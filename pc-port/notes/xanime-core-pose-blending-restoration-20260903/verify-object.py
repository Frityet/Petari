#!/usr/bin/env python3
"""Compile root XanimeCore and compare the three pose methods with retail.

Generated original-toolchain output stays under build/. This is not a native build.
"""

import ast
import bisect
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
BUILD = ROOT / "build/xanime-core-pose-blending-restoration-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
NAMES = (
    "calcBlend__10XanimeCoreFPQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f>",
    "calcSingle__10XanimeCoreFPQ29JGeometry8TVec3<f>PQ29JGeometry8TVec3<f>",
    "calcBlendSpecial__10XanimeCoreFv",
)
VEC_CTOR = "__ct__Q29JGeometry8TVec3<f>FRC3Vec"
VEC_ZERO = "zero__Q29JGeometry8TVec3<f>Fv"
VEC_SET = "set__Q29JGeometry8TVec3<f>FRC3Vec"
VEC_ASSIGN = "__as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>"

# Reuse the documented ELF/DOL readers only; do not run Mario's task driver.
spec = importlib.util.spec_from_file_location(
    "object_reader", ROOT / "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (BUILD / log).write_text(result.stdout)
    if result.stdout:
        print(result.stdout, end="")
    result.check_returncode()


def instructions(symbol):
    return [entry["instruction"] for entry in symbol["instructions"] if "instruction" in entry]


def normalized_events(symbol):
    """Compare control-flow topology and ordered arithmetic/helper events.

    Register names are erased. Branch destinations are indexes in the event stream,
    not raw addresses. The only ignored calls are separately checked Vec copies and
    outlined zeroing. This supplements, and does not replace, operand/source review.
    """
    events = []
    for instruction in instructions(symbol):
        text = instruction["formatted"].strip()
        opcode = text.split()[0]
        if opcode == "bl":
            target = text[3:]
            if target in (VEC_CTOR, VEC_ZERO):
                continue
            if target == VEC_SET:
                target = VEC_ASSIGN
            event = "call " + target
        elif opcode in ("fadds", "fsubs", "fmuls", "fdivs", "fcmpo", "fcmpu") or opcode.startswith(("cmp", "cmpl")):
            event = re.sub(r"\b[rf]\d+\b", "_", text)
        elif opcode.startswith("b") and opcode != "blr":
            event = text
        else:
            continue
        events.append([int(instruction["address"]), event])
    addresses = [event[0] for event in events]
    for event in events:
        if re.match(r"b\w* 0x", event[1]):
            opcode, target = event[1].split()
            event[1] = opcode + " event" + str(bisect.bisect_left(addresses, int(target, 16)))
    return [event[1] for event in events]


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

    checks = {
        "sizeof(XanimeCore)": 0x2C, "sizeof(XanimeTrack)": 0x10,
        "sizeof(XjointInfo)": 0x64, "sizeof(XtransformInfo)": 0x28,
        "sizeof(J3DTransformInfo)": 0x20,
        "offsetof(J3DTransformInfo, mScale)": 0,
        "offsetof(J3DTransformInfo, mRotation)": 0xC,
        "offsetof(J3DTransformInfo, mTranslate)": 0x14,
        "offsetof(J3DMtxBuffer, mpAnmMtx)": 0xC,
        "offsetof(XjointInfo, _0)": 0, "offsetof(XjointInfo, _28)": 0x28,
        "offsetof(XjointInfo, _50)": 0x50, "offsetof(XjointInfo, _5C)": 0x5C,
        "offsetof(XjointInfo, _60)": 0x60, "offsetof(XtransformInfo, mRotation)": 0x18,
        "offsetof(XanimeCore, mTrackCount)": 5, "offsetof(XanimeCore, mJointList)": 0x10,
        "offsetof(XanimeCore, mTrackList)": 0x18, "offsetof(XanimeCore, _1C)": 0x1C,
        "offsetof(XanimeCore, _20)": 0x20, "offsetof(XanimeCore, _29)": 0x29,
    }
    probe = '#include "Game/Animation/XanimeCore.hpp"\n#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"\n#include <stddef.h>\n'
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
    elves = [reader.Elf(path) for path in (target, compiled)]
    retail_symbols = {}
    for line in (ROOT / "config/RMGK01/symbols.txt").read_text().splitlines():
        match = re.match(r"(.*?) = \.[^:]+:(0x[0-9A-F]+);.*size:(0x[0-9A-F]+)", line)
        if match:
            retail_symbols[match[1]] = (int(match[2], 0), int(match[3], 0))

    evidence = {
        "scope": "Three root pose blending methods; original-toolchain comparison, not native/runtime animation activation.",
        "source_sha256": sha(ROOT / "src/Game/Animation/XanimeCore.cpp"),
        "header_sha256": sha(ROOT / "include/Game/Animation/XanimeCore.hpp"),
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "compiler": "GC/3.0a3, configure.py cflags_game, RMGK01 VERSION=0; sjiswrap v1.2.2",
        "tool_versions": {"dtk": "v1.8.3", "objdiff-cli": "v3.6.1", "sjiswrap": "v1.2.2"},
        "tool_hashes": {path: sha(ROOT / path) for path in (
            "build/compilers/GC/3.0a3/mwcceppc.exe", "build/tools/sjiswrap.exe",
            "build/tools/dtk", "build/tools/objdiff-cli")},
        "target_object_sha256": sha(target), "compiled_object_sha256": sha(compiled),
        "verified_ppc_layout": checks,
        "normalization": "Ignore separately inspected Vec copy constructors and outlined zero helper; map set(Vec) to TVec assignment. Erase register allocation in the event stream, retain comparison and arithmetic opcodes, calls, and branch destinations.",
        "functions": [],
    }
    helpers = {}
    for name in (VEC_CTOR, VEC_ZERO, VEC_SET):
        symbol = next(s for s in diff["right"]["symbols"] if s["name"] == name)
        helpers[name] = [i["formatted"].strip() for i in instructions(symbol)]
    assert helpers[VEC_CTOR] == ["psq_l f1, 0x0(r4), 0, qr0", "lfs f0, 0x8(r4)",
                                 "psq_st f1, 0x0(r3), 0, qr0", "stfs f0, 0x8(r3)", "blr"]
    assert helpers[VEC_SET] == ["lfs f2, 0x0(r4)", "lfs f1, 0x4(r4)", "lfs f0, 0x8(r4)",
                                "stfs f2, 0x0(r3)", "stfs f1, 0x4(r3)", "stfs f0, 0x8(r3)", "blr"]
    assert helpers[VEC_ZERO][1:] == ["stfs f0, 0x8(r3)", "stfs f0, 0x4(r3)", "stfs f0, 0x0(r3)", "blr"]
    zero_refs = elves[1].references(VEC_ZERO)
    assert len(zero_refs) == 1 and zero_refs[0]["value_hex"] == "00000000"
    evidence["checked_adaptation_helpers"] = helpers
    aligned = []
    for name in NAMES:
        sides = [next(s for s in diff[key]["symbols"] if s["name"] == name) for key in ("left", "right")]
        refs = [elf.references(name) for elf in elves]
        calls = [[r["symbol"] for r in references if r["kind"] == 10] for references in refs]
        normalized_calls = [[VEC_ASSIGN if call == VEC_SET else call for call in sequence
                             if call not in (VEC_CTOR, VEC_ZERO)] for sequence in calls]
        assert normalized_calls[0] == normalized_calls[1], (name, "Original call sequence changed")
        assert calls[1].count(VEC_CTOR) == (0 if "calcSingle" in name else 2)
        assert calls[1].count(VEC_ZERO) == (2 if "calcBlend__" in name else 0)
        assert calls[1].count(VEC_SET) == (2 if "calcSingle" in name else 0)
        values = [{r["value_hex"] for r in references if "value_hex" in r} for references in refs]
        assert values[0] == values[1], (name, "Constant values differ")
        external = [Counter((r["kind"], r["symbol"], r["addend"]) for r in references
                            if r["kind"] != 10 and "value_hex" not in r) for references in refs]
        assert external[0] == external[1], (name, "External globals differ")
        events = [normalized_events(side) for side in sides]
        assert events[0] == events[1], (name, "Arithmetic/call/control topology changed")
        assert sides[0]["match_percent"] >= 90.0
        address, size = retail_symbols[name]
        record = {
            "symbol": name, "address": hex(address), "retail_size": size,
            "compiled_size": int(sides[1]["size"]), "objdiff_match_percent": sides[0]["match_percent"],
            "retail_function_sha256": hashlib.sha256(reader.dol_bytes(dol, address, size)).hexdigest(),
            "retail_direct_calls": calls[0], "compiled_direct_calls": calls[1],
            "retail_references": refs[0], "compiled_references": refs[1],
            "identical_normalized_event_count": len(events[0]),
            "identical_normalized_events": events[0],
        }
        evidence["functions"].append(record)
        print(f"{name}: {sides[0]['match_percent']:.6f}%, {size}/{sides[1]['size']} bytes; "
              f"{len(events[0])} identical normalized arithmetic/call/control events")
        aligned.append(f"\n{name}: {address:#x}, {sides[0]['match_percent']}%")
        for left, right in zip(sides[0]["instructions"], sides[1]["instructions"]):
            texts = []
            for side, entry in enumerate((left, right)):
                instruction = entry.get("instruction", {})
                relative = int(instruction["address"]) - int(sides[side]["address"]) if instruction else None
                location = f"{address + relative:08x}" if instruction and side == 0 else f"+{relative:04x}" if instruction else ""
                texts.append(f"{entry.get('diff_kind', ''):18} {location:9} {instruction.get('formatted', '')}")
            aligned.append(f"{texts[0]:120} | {texts[1]}")
    (BUILD / "three-function-comparison.txt").write_text("\n".join(aligned) + "\n")
    (BUILD / "compiler-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
