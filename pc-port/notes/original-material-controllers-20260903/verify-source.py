#!/usr/bin/env python3
"""Original-compiler/DOL proof of the material projection controller group."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import struct
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-material-controllers-20260903"
spec = importlib.util.spec_from_file_location("maker", HERE.parent / "original-model-manager-owner-20260903/verify-source.py")
maker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(maker)
reader, proof = maker.reader, maker.proof
DOL = maker.DOL
FUNCTIONS = (
    "__ct__12MaterialCtrlFP12J3DModelDataPCc",
    "update__12MaterialCtrlFv",
    "__ct__12MatColorCtrlFP12J3DModelDataPCcUlPC10J3DGXColor",
    "updateMaterial__12MatColorCtrlFP11J3DMaterial",
    "__ct__26ViewProjmapEffectMtxSetterFP12J3DModelData",
    "update__26ViewProjmapEffectMtxSetterFv",
    "__ct__22ProjmapEffectMtxSetterFP8J3DModelPC14ResourceHolder",
    "update__22ProjmapEffectMtxSetterFv",
    "getBaseTrans__22ProjmapEffectMtxSetterCFPQ29JGeometry8TVec3<f>",
    "updateMtxUseBaseMtx__22ProjmapEffectMtxSetterFv",
    "updateMtxUseBaseMtxWithLocalOffset__22ProjmapEffectMtxSetterFRCQ29JGeometry8TVec3<f>",
    "__ct__27MarioShadowProjmapMtxSetterFP8J3DModelPC14ResourceHolder",
    "update__27MarioShadowProjmapMtxSetterFv",
    "updateMaterial__12MaterialCtrlFP11J3DMaterial",
    "__ct__Q222ProjmapEffectMtxSetter19UpdateEffectMtxInfoFv",
)
NEW = {FUNCTIONS[4], FUNCTIONS[5], FUNCTIONS[11], FUNCTIONS[12], FUNCTIONS[13]}


def refs(elf, function):
    result = elf.references(function)
    for ref in result:
        # Vtables and constructor-function addresses are real symbols, not
        # strings beginning with zero or four-byte anonymous constants.
        symbol = next((s for s in elf.symbols if s[0] == ref["symbol"]), None)
        if ref["symbol"].startswith("__vt__") or (symbol is not None and symbol[3] and elf.sections[symbol[3]][2] & 4):
            ref.pop("value_hex", None)
    return result


def canonicalize(sides, objects, name):
    normalized = [proof.normalize(side, refs(obj, name), "projection", bool(i))
                  for i, (side, obj) in enumerate(zip(sides, objects))]
    changes = []
    if name == FUNCTIONS[10]:
        mapping = {0: 3, 1: 4, 2: 2, 3: 1, 4: 0}
        normalized[1] = [re.sub(r"\bf([0-4])\b", lambda m: "f" + str(mapping[int(m[1])]), row)
                         for row in normalized[1]]
        loads = ["lfs f2, 0x0(r31)", "lfs f1, 0x4(r31)", "lfs f0, 0x8(r31)"]
        start = normalized[0].index(loads[0])
        assert normalized[0][start:start+3] == loads
        assert normalized[1][start:start+3] == list(reversed(loads))
        normalized[1][start:start+3] = loads
        changes.append("Bijective f0/f3 and f1/f4 temporary-register swaps; three independent local-offset scalar loads reversed. All stores, matrix operations, operands and call order otherwise identical.")
    assert normalized[0] == normalized[1], (name, [(i,a,b) for i,(a,b) in enumerate(zip(*normalized)) if a != b])
    return normalized[0], changes


def verify_vtables(compiled, dol, symbols):
    records = []
    for cls, expected in (
        ("12MaterialCtrl", ["update__12MaterialCtrlFv", "updateMaterial__12MaterialCtrlFP11J3DMaterial"]),
        ("26ViewProjmapEffectMtxSetter", ["update__26ViewProjmapEffectMtxSetterFv", "updateMaterial__12MaterialCtrlFP11J3DMaterial"]),
        ("22ProjmapEffectMtxSetter", ["update__22ProjmapEffectMtxSetterFv", "updateMaterial__12MaterialCtrlFP11J3DMaterial"]),
        ("27MarioShadowProjmapMtxSetter", ["update__27MarioShadowProjmapMtxSetterFv", "updateMaterial__12MaterialCtrlFP11J3DMaterial"]),
    ):
        name = "__vt__" + cls
        address, size = symbols[name]
        assert size == 16
        retail = reader.dol_bytes(dol, address, size)
        assert retail[:8] == bytes(8)
        assert list(struct.unpack(">II", retail[8:])) == [symbols[x][0] for x in expected]
        actual_refs = sorted(compiled.references(name), key=lambda r: int(r["offset"], 16))
        assert [(r["offset"], r["kind"], r["symbol"]) for r in actual_refs] == [
            ("0x8", 1, expected[0]), ("0xc", 1, expected[1])]
        records.append({"name": name, "retail_address": hex(address), "size": size, "slots": expected})
    return records


def main():
    BUILD.mkdir(exist_ok=True, parents=True)
    maker.BUILD = BUILD
    maker.refs = refs
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    config_text = (ROOT / "config/RMGK01/symbols.txt").read_text()
    symbols = {name: (int(address, 16), int(size, 16)) for name,address,size in re.findall(
        r"^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);[^\n]*size:(0x[0-9A-Fa-f]+)", config_text, re.M)}
    for name,address in re.findall(r"^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);", config_text, re.M):
        symbols.setdefault(name, (int(address, 16), 0))
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Configured Game compiler flags absent")
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command += ["-c", "src/Game/LiveActor/MaterialCtrl.cpp", "-o", str(BUILD / "MaterialCtrl.o")]
    (BUILD / "MaterialCtrl.command.json").write_text(json.dumps(command, indent=2) + "\n")
    maker.run(command, "MaterialCtrl.compile.log")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    maker.run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    target_path = BUILD / "retail/obj/Game/LiveActor/MaterialCtrl.o"
    maker.run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / "MaterialCtrl.o",
               "-o", BUILD / "MaterialCtrl.diff.json", "--format", "json-pretty"], "MaterialCtrl.objdiff.log")
    objects = reader.Elf(target_path), reader.Elf(BUILD / "MaterialCtrl.o")
    diff = json.loads((BUILD / "MaterialCtrl.diff.json").read_text())
    records = []
    for name in FUNCTIONS:
        sides = [next(s for s in diff[side]["symbols"] if s["name"] == name) for side in ("left", "right")]
        for side in sides:
            for row in side["instructions"]:
                if "instruction" in row:
                    row["instruction"].setdefault("address", "0")
        canonical, changes = canonicalize(sides, objects, name)
        address, size = symbols[name]
        code, relocations = maker.relocate(objects[1], objects[0], name, address, size, dol, symbols)
        equal = code == reader.dol_bytes(dol, address, size)
        if not changes:
            assert equal, name
        records.append({"name": name, "new_body": name in NEW, "address": hex(address), "retail_size": size,
                        "compiled_size": len(code), "objdiff_match_percent": sides[0]["match_percent"],
                        "relocated_bytes_equal": equal, "normalization": changes,
                        "all_canonical_instructions_equal": True, "canonical_instructions": canonical,
                        "relocations": relocations})
        print(name, sides[0]["match_percent"], "verified")
    evidence = {"compiler": "GC3.0a3 with configured Game flags and Shift-JIS wrapper", "dol_sha1": hashlib.sha1(dol).hexdigest(),
                "functions": records, "vtables": verify_vtables(objects[1], dol, symbols),
                "source_sha256": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in (
                    "src/Game/LiveActor/MaterialCtrl.cpp", "include/Game/LiveActor/MaterialCtrl.hpp")},
                "scope": "Root projection-controller source/layout recovery only; no native activation or replacement texture."}
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
