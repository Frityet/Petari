#!/usr/bin/env python3
"""Check the isolated owner's original sources and save reproducible inputs.

This writes evidence only. It does not activate native Game sources or run a
shared build. ModelX preparation is verified by the preceding ModelX notes.
"""
from pathlib import Path
import hashlib
import json
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-model-manager-native-20260903"
DRAW = ROOT / "build/original-model-manager-render-20260903/staged/Game/System"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, signature):
    start = text.index(signature)
    at = text.index("{", start)
    depth, end = 1, at + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def tokens(text):
    return re.sub(r"\s+", "", re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S))


def main():
    identical = []
    for name in ("ModelManager", "DisplayListMaker", "MaterialCtrl"):
        for prefix, extension in (("src", ".cpp"), ("include", ".hpp")):
            original = ROOT / prefix / "Game/LiveActor" / (name + extension)
            native = BUILD / (name + extension) if extension == ".cpp" else BUILD / "Game/LiveActor" / (name + extension)
            assert original.read_bytes() == native.read_bytes(), (original, native)
            identical.append({"original": str(original.relative_to(ROOT)), "staged": str(native.relative_to(ROOT)),
                              "sha256": digest(original), "byte_identical": True})
    for name in ("DrawBuffer", "DrawBufferExecuter", "DrawBufferGroup", "DrawBufferHolder"):
        for prefix, extension in (("src", ".cpp"), ("include", ".hpp")):
            original = ROOT / prefix / "Game/System" / (name + extension)
            native = DRAW / (name + extension)
            assert original.read_bytes() == native.read_bytes(), (original, native)
            identical.append({"original": str(original.relative_to(ROOT)), "staged": str(native.relative_to(ROOT)),
                              "sha256": digest(original), "byte_identical": True})

    model = "src/Game/Util/ModelUtil.cpp"
    live = "src/Game/Util/LiveActorUtil.cpp"
    helpers = [
        (model, "u16 getMaterialNo(J3DModelData* pModelData, const char* pMaterialName)"),
        (model, "J3DMaterial* getMaterial(J3DModelData* pModelData, const char* pMaterialName)"),
        (model, "bool isUseTex(J3DMaterial* pMaterial, u16 a2)"),
        (model, "bool isNormalTexMtx(J3DMaterial* pMaterial)"),
        ("src/Game/Util/PlayerUtil.cpp", "TVec3f* getPlayerShadowRotate()"),
        (live, "ProjmapEffectMtxSetter* initDLMakerProjmapEffectMtxSetter(LiveActor* pActor)"),
        (model, "J3DModel* getJ3DModel(const LiveActor* pActor)"),
        (model, "J3DMaterial* getMaterial(J3DModel* pModel, int idx)"),
        (model, "s32 getMaterialNum(J3DModel* pModel)"),
        (model, "const char* getMaterialName(const J3DModelData* pModelData, int idx)"),
        (live, "ResourceHolder* getModelResourceHolder(const LiveActor* pActor)"),
        (live, "const char* getModelResName(const LiveActor* pActor)"),
    ]
    extracted = []
    for original, signature in helpers:
        lhs = tokens(body((ROOT / original).read_text(), signature))
        rhs = tokens(body((BUILD / "Helpers.cpp").read_text(), signature))
        normalization = None
        if signature.startswith("const char* getModelResName"):
            # Select exactly the original unsigned-32 overload on LP64.
            lhs = lhs.replace("static_cast<u32>(0)", "0U").replace("0UL", "0U")
            normalization = "The original u32 zero index is spelled 0U in the native extract."
        assert lhs == rhs, signature
        extracted.append({"signature": signature, "original": original, "tokens_equal": True,
                          "normalization": normalization})
    for signature in ("const JUTTexture* getMarioShadowTex()", "const JUTTexture* getMarioShadowTexForLoad()",
                      "const TVec3f& getMarioShadowVec()", "void setMarioShadowTex(const JUTTexture* pShadowTex)",
                      "void setMarioShadowVec(const TVec3f& rVec)"):
        original = "src/Game/Util/DrawUtil.cpp"
        assert tokens(body((ROOT / original).read_text(), signature)) == tokens(body((BUILD / "ShadowPublish.cpp").read_text(), signature))
        extracted.append({"signature": signature, "original": original, "tokens_equal": True, "normalization": None})
    for declaration in ("const JUTTexture* mShadowTex;", "TVec3f mShadowVec;"):
        assert declaration in (ROOT / "src/Game/Util/DrawUtil.cpp").read_text()
        assert declaration in (BUILD / "ShadowPublish.cpp").read_text()

    for name in ("ModelManagerOwner.cpp", "ModelManagerOwner.hpp", "Helpers.cpp", "ShadowPublish.cpp"):
        assert (HERE / name).read_bytes() == (BUILD / name).read_bytes(), name
    assert (HERE / "ModelManagerDrawLifetimeProbe.cpp").read_bytes() == (BUILD / "draw-live.cpp").read_bytes()
    current_header = ROOT / "pc-port/src/runtime/RuntimeContext.hpp"
    overlay_header = ROOT / "build/original-modelx-native-20260903/staged/include/runtime/RuntimeContext.hpp"
    assert current_header.read_bytes() == overlay_header.read_bytes(), "RuntimeContext overlay is stale"
    tracked = [BUILD / name for name in ("ModelManagerOwner.cpp", "ModelManagerOwner.hpp", "Helpers.cpp", "ShadowPublish.cpp",
               "ActorRuntimeRegistry.cpp", "LodCtrlRuntimeCompat.cpp", "LiveActor.cpp", "draw-live.cpp")]
    tracked += [current_header, ROOT / "pc-port/src/runtime/RuntimeContext.cpp"]
    tracked += [ROOT / "pc-port/build/macosx/arm64/debug" / name for name in (
        "libsmg-pc-game.a", "libsmg-pc-app.a", "libsmg-pc-common.a", "libsmg-pc-render.a", "libaurora-core.a", "libaurora-mtx.a")]
    result = {
        "scope": "Original ModelManager/ModelX/DrawBuffer construction and actual packet lifetime probe; no actor or MarioAnimator activation.",
        "root_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "aurora_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT / "pc-port/aurora", text=True).strip(),
        "original_sources": identical,
        "extracted_helpers": extracted,
        "probe_and_link_inputs": {str(path.relative_to(ROOT)): digest(path) for path in tracked},
        "runtime_overlay_matches_current_header": True,
        "warning": "The registry/LOD/LiveActor overlay is probe-only coexistence scaffolding, not an approved production activation patch.",
    }
    (HERE / "source-evidence.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"Verified {len(identical)} byte-identical original source/header files and {len(extracted)} original helper bodies.")
    print("Verified captured-owner/probe snapshots and current RuntimeContext header identity.")


if __name__ == "__main__":
    main()
