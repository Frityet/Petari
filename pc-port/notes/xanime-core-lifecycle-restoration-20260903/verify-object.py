#!/usr/bin/env python3
"""Compile the actual root core, check PPC layout, and compare nine retail methods."""

import ast
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess
import types


ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/xanime-core-lifecycle-restoration-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
NAMES = (
    "__as__14XtransformInfoFRC14XtransformInfo",
    "__ct__15XjointTransformFv",
    "enableJointTransform__10XanimeCoreFP12J3DModelData",
    "reconfigJointTransform__10XanimeCoreFP12J3DModelData",
    "setBck__10XanimeCoreFUlP15J3DAnmTransform",
    "updateFrame__10XanimeCoreFv",
    "freezeCopy__10XanimeCoreFP12J3DModelDataP10XanimeCoreUlUl",
    "initT__10XanimeCoreFP12J3DModelData",
    "fixT__10XanimeCoreFPQ29JGeometry8TVec3<f>",
)
VEC_SET = "set__Q29JGeometry8TVec3<f>FRC3Vec"
VEC_ASSIGN = "__as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>"

# Reuse only the independently documented ELF/DOL readers, not Mario's task driver.
spec = importlib.util.spec_from_file_location("object_reader", ROOT / "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / log).write_text(result.stdout)
    if result.stdout:
        print(result.stdout, end="")
    result.check_returncode()


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Configured Game flags absent")
    base = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        base.extend(shlex.split(flag))
    command = base + ["-c", "src/Game/Animation/XanimeCore.cpp", "-o", str(BUILD / "XanimeCore.o")]
    (BUILD / "XanimeCore.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "XanimeCore.compile.log")

    # Test production declarations directly. No generated include overlays or dummy classes.
    checks = {
        "sizeof(XtransformInfo)": 0x28, "sizeof(XjointInfo)": 0x64,
        "sizeof(XjointTransform)": 0x70, "sizeof(XanimeTrack)": 0x10,
        "sizeof(XanimeCore)": 0x2C,
        "offsetof(XtransformInfo, mRotation)": 0x18,
        "offsetof(XjointTransform, _0)": 0,
        "offsetof(XjointTransform, _4)": 4,
        "offsetof(XjointTransform, mTransformInfo)": 0x44,
        "offsetof(XjointTransform, _56)": 0x56,
        "offsetof(XjointTransform, _64)": 0x64,
        "offsetof(XjointTransform, _6C)": 0x6C,
        "offsetof(XanimeCore, mJointList)": 0x10,
        "offsetof(XanimeCore, mTransformList)": 0x14,
        "offsetof(XanimeCore, mTrackList)": 0x18,
    }
    probe = '#include "Game/Animation/XanimeCore.hpp"\n#include <stddef.h>\n'
    probe += "\n".join(f"typedef char Check{i}[({expr} == {value}) ? 1 : -1];"
                       for i, (expr, value) in enumerate(checks.items())) + "\n"
    (BUILD / "layout.cpp").write_text(probe)
    run(base + ["-c", str(BUILD / "layout.cpp"), "-o", str(BUILD / "layout.o")], "layout.compile.log")

    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", str(BUILD / "config.yml"),
         str(BUILD / "retail")], "dtk.log")
    target = BUILD / "retail/obj/Game/Animation/XanimeCore.o"
    compiled = BUILD / "XanimeCore.o"
    run(["build/tools/objdiff-cli", "diff", "-1", str(target), "-2", str(compiled),
         "-o", str(BUILD / "objdiff.json"), "--format", "json-pretty"], "objdiff.log")
    diff = json.loads((BUILD / "objdiff.json").read_text())
    elves = [reader.Elf(p) for p in (target, compiled)]
    symbols = {}
    for line in (ROOT / "config/RMGK01/symbols.txt").read_text().splitlines():
        m = re.match(r"(.*?) = \.[^:]+:(0x[0-9A-F]+);.*size:(0x[0-9A-F]+)", line)
        if m:
            symbols[m[1]] = (int(m[2], 0), int(m[3], 0))

    evidence = {
        "scope": "Nine root lifecycle methods; live fuzzy comparison and source/layout checks. Large blend/calc methods remain absent.",
        "source_sha256": sha(ROOT / "src/Game/Animation/XanimeCore.cpp"),
        "header_sha256": sha(ROOT / "include/Game/Animation/XanimeCore.hpp"),
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "compiler": "GC/3.0a3, configure.py cflags_game, RMGK01 VERSION=0, sjiswrap v1.2.2",
        "tool_versions": {"dtk": "v1.8.3", "objdiff-cli": "v3.6.1", "sjiswrap": "v1.2.2"},
        "tool_hashes": {p: sha(ROOT / p) for p in ("build/compilers/GC/3.0a3/mwcceppc.exe",
                        "build/tools/sjiswrap.exe", "build/tools/dtk", "build/tools/objdiff-cli")},
        "target_object_sha256": sha(target), "compiled_object_sha256": sha(compiled),
        "verified_ppc_layout": checks,
        "typed_copy_adaptation": "initT and fixT use TVec3::set(const Vec&) for SDK Vec fields. This copies the same x/y/z with scalar loads/stores; no invalid downcast of a Vec into a TVec3 object.",
        "functions": [],
    }
    set_symbol = next(s for s in diff["right"]["symbols"] if s["name"] == VEC_SET)
    set_instructions = [i["instruction"]["formatted"].strip() for i in set_symbol["instructions"]]
    assert set_instructions == ["lfs f2, 0x0(r4)", "lfs f1, 0x4(r4)", "lfs f0, 0x8(r4)",
                                "stfs f2, 0x0(r3)", "stfs f1, 0x4(r3)", "stfs f0, 0x8(r3)", "blr"]
    evidence["typed_vec_copy_instructions"] = set_instructions
    aligned = []
    for name in NAMES:
        sides = [next(s for s in diff[k]["symbols"] if s["name"] == name) for k in ("left", "right")]
        address, size = symbols[name]
        refs = [elf.references(name) for elf in elves]
        for references in refs:
            for reference in references:
                if reference["symbol"] == "__ct__15XjointTransformFv":
                    # This is an actual constructor function pointer, not a string pool.
                    reference.pop("value_hex", None)
                elif reference["kind"] != 10 and "value_hex" not in reference and reference["symbol"].startswith("lbl_"):
                    # dtk places the shared integer-conversion doubles in another TU.
                    constant_address, constant_size = symbols[reference["symbol"]]
                    assert constant_size == 8 and reference["addend"] == 0
                    reference["value_hex"] = reader.dol_bytes(dol, constant_address, constant_size).hex()
                    reference["retail_data_address"] = hex(constant_address)
        calls = [[r["symbol"] for r in rs if r["kind"] == 10] for rs in refs]
        normalized_calls = [[VEC_ASSIGN if c == VEC_SET else c for c in cs] for cs in calls]
        assert normalized_calls[0] == normalized_calls[1], (name, "Call sequence differs beyond typed Vec copy")
        values = [{r["value_hex"] for r in rs if "value_hex" in r} for rs in refs]
        assert values[0] == values[1], (name, "Constant mismatch")
        external = [Counter((r["kind"], r["symbol"], r["addend"]) for r in rs
                            if r["kind"] != 10 and "value_hex" not in r) for rs in refs]
        assert external[0] == external[1], (name, "External data reference mismatch")
        assert sides[0]["match_percent"] >= 90
        evidence["functions"].append({
            "symbol": name, "address": hex(address), "retail_size": size,
            "compiled_size": int(sides[1]["size"]), "objdiff_match_percent": sides[0]["match_percent"],
            "retail_function_sha256": hashlib.sha256(reader.dol_bytes(dol, address, size)).hexdigest(),
            "retail_direct_calls": calls[0], "compiled_direct_calls": calls[1],
            "retail_references": refs[0], "compiled_references": refs[1],
        })
        print(f"{name}: {sides[0]['match_percent']:.6f}%, {size}/{sides[1]['size']} bytes")
        aligned.append(f"\n{name}: {address:#x}, {sides[0]['match_percent']}%")
        for left, right in zip(sides[0]["instructions"], sides[1]["instructions"]):
            texts = []
            for side, entry in enumerate((left, right)):
                inst = entry.get("instruction", {})
                relative = int(inst["address"]) - int(sides[side]["address"]) if inst else None
                location = f"{address + relative:08x}" if inst and side == 0 else f"+{relative:04x}" if inst else ""
                texts.append(f"{entry.get('diff_kind', ''):18} {location:9} {inst.get('formatted', '')}")
            aligned.append(f"{texts[0]:120} | {texts[1]}")
    (BUILD / "nine-function-comparison.txt").write_text("\n".join(aligned) + "\n")
    (BUILD / "compiler-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
