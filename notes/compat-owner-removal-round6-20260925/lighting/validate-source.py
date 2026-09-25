#!/usr/bin/env python3
"""Confirm complete lighting donor owners and explicit native boundaries."""
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("source_parser", ROOT / "notes/compat-owner-removal-round5-20260925/draw/source_parser.py")
parser = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parser)
checks = []


def check(label, result):
    checks.append({"name": label, "passed": bool(result)})


allow = {
    "LightDirector::LightDirector": "CP932 literal encoding",
    "LightDirector::init": "Exception-safe ownership transfer after all three allocations",
    "LightFunction::registerPlayerLightCtrl": "Native two-way borrowed controller lifetime",
    "LightFunction::registerLightAreaHolder": "Native two-way borrowed area-manager lifetime",
    "LightFunction::loadPointLightInfo": "Existing native PointLightInfo field names and enum type",
}
owners = ["Map/LightDirector.cpp", "Map/LightFunction.cpp", "Map/LightDataHolder.cpp", "Map/LightZoneDataHolder.cpp",
          "Util/LightUtil.cpp", "LiveActor/ActorLightCtrl.cpp"]
definitions = []
for owner in owners:
    original = parser.functions((ROOT / "decomp/src/Game" / owner).read_text())
    native = parser.functions((ROOT / "src/Game" / owner).read_text())
    for function in original:
        symbol = function["symbol"]
        matches = [f for f in native if f["symbol"] == symbol]
        check(owner + " contains one " + symbol, len(matches) == 1)
        if len(matches) != 1:
            continue
        if symbol not in allow:
            check(symbol + " has complete unchanged donor body", matches[0]["normalized"] == function["normalized"])
        definitions.append({"symbol": symbol, "owner": "src/Game/" + owner, "native_change": allow.get(symbol)})

point = ROOT / "src/Game/Map/LightPointCtrl.cpp"
check("PointLightCtrl numeric, generation and ownership implementation unchanged", point.read_bytes() == (NOTES / "before/src/Game/Map/LightPointCtrl.cpp").read_bytes())
check("PointLightCtrl native record and generation schema unchanged", (ROOT / "src/Game/Map/LightPointCtrl.hpp").read_bytes() == (NOTES / "before/src/Game/Map/LightPointCtrl.hpp").read_bytes())
deleted = ["src/compat/LightDirectorCompat.cpp", "src/compat/LightFunctionCompat.cpp", "src/compat/LightFunctionCompat.hpp", "src/compat/LightUtilCompat.cpp",
           "src/render/light/LightData.cpp", "src/render/light/LightData.hpp", "src/scene/StageLightSceneBinding.cpp", "src/scene/StageLightSceneBinding.hpp"]
for path in deleted:
    check(path + " removed after snapshot", not (ROOT / path).exists() and (NOTES / "before" / path).exists())
retired = ["StageLightData", "StageLightSceneBinding", "LightFunctionCompat", "registered_player_light_controller", "unregister_player_light_controller", "initialize_original_scene_lights"]
for name in retired:
    references = [str(p.relative_to(ROOT)) for base in ("src", "tests") for p in (ROOT / base).rglob("*") if p.suffix in (".cpp", ".hpp") and name in p.read_text(errors="replace")]
    check(name + " has no remaining source/test callers", not references)

function = (ROOT / "src/Game/Map/LightFunction.cpp").read_text()
for name in ("RuntimeContext", "SceneLightService", "GXLightState", "std::clamp"):
    check("LightFunction no longer routes through " + name, name not in function)
check("Original coordinate-space conversion retained", "MR::getCameraInvViewMtx().mult(pos, pos);" in function and "MR::getCameraViewMtx().mult(pos, pos);" in function)
check("Canonical sorting is called rather than a no-op adapter", (ROOT / "src/Game/LiveActor/ActorLightCtrl.cpp").read_text().count("_8->resetLightSort(this);") == 3)
test = (ROOT / "tests/PointLightRuntimeTests.cpp").read_text()
check("Default light test uses the real original process", "run_stage_resource_process(" in test and "auto runtime = smgpc::runtime::RuntimeContext(" not in test)
check("Independent existing ownership and GX modes retained", '"--original-owner-only"' in test and '"--original-gx-only"' in test)

output = {"all_passed": all(c["passed"] for c in checks), "checks": checks, "check_count": len(checks), "donor_definitions": definitions,
          "source_sha256": {"src/Game/" + path: hashlib.sha256((ROOT / "src/Game" / path).read_bytes()).hexdigest() for path in owners},
          "validation_boundary": "Source validation only; parent owns compile and runtime results"}
(NOTES / "source-validation.json").write_text(json.dumps(output, indent=2) + "\n")
print(json.dumps({"all_passed": output["all_passed"], "check_count": len(checks), "donor_definition_count": len(definitions), "failed": [c for c in checks if not c["passed"]]}))
raise SystemExit(0 if output["all_passed"] else 1)
