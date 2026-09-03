#!/usr/bin/env python3
"""Compile four root XanimeCore entry/scale methods with the original compiler.

This does not build the PC port. Generated objects and complete diffs stay in build/.
"""

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
BUILD = ROOT / "build/xanime-core-matrix-calculation-20260903/entrypoints"
DOL = ROOT / "build/compat-math-oracle/main.dol"
SOURCE = ROOT / "src/Game/Animation/XanimeCore.cpp"
HEADER = ROOT / "include/Game/Animation/XanimeCore.hpp"
FUNCTIONS = (
    ("calcScaleBlendBasic", 0x8001AB58, 0x104),
    ("calcScaleBlendMayaNoTransform", 0x8001A4BC, 0x1CC),
    ("calc", 0x8001AFA0, 0x124),
    ("init", 0x8001B0C4, 0x24),
)

# Only reuse the documented ELF/DOL reader and normalized event comparison.
spec = importlib.util.spec_from_file_location(
    "pose_verifier", ROOT / "pc-port/notes/xanime-core-pose-blending-restoration-20260903/verify-object.py")
helpers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helpers)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(source, name):
    start = source.index("void XanimeCore::" + name + "(")
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def run(command, log):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (BUILD / log).write_text(result.stdout)
    if result.stdout:
        print(result.stdout, end="")
    result.check_returncode()


def normalized_instructions(symbol, references):
    """Retain operands, register allocation and memory offsets; normalize relocations."""
    result = []
    constants = {ref["symbol"]: ref["value_hex"] for ref in references if "value_hex" in ref}
    for instruction in helpers.instructions(symbol):
        text = instruction["formatted"].strip()
        for name, value in constants.items():
            text = text.replace(name + "@", "constant_" + value + "@")
        if re.match(r"b\w* 0x", text):
            opcode, target = text.split()
            text = opcode + " relative_" + hex(int(target, 16) - int(symbol["address"]))
        result.append(text)
    return result


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
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    compiled = BUILD / "XanimeCore.o"
    command.extend(["-c", "src/Game/Animation/XanimeCore.cpp", "-o", str(compiled)])
    (BUILD / "XanimeCore.command.json").write_text(json.dumps(command, indent=2) + "\n")
    source = SOURCE.read_text()
    source_sha = sha(SOURCE)
    header_sha = sha(HEADER)
    run(command, "XanimeCore.compile.log")
    assert sha(SOURCE) == source_sha and sha(HEADER) == header_sha, "Sources changed during compilation; rerun."

    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", str(BUILD / "config.yml"),
         str(BUILD / "retail")], "dtk.log")
    target = BUILD / "retail/obj/Game/Animation/XanimeCore.o"
    run(["build/tools/objdiff-cli", "diff", "-1", str(target), "-2", str(compiled),
         "-o", str(BUILD / "objdiff.json"), "--format", "json-pretty"], "objdiff.log")
    diff = json.loads((BUILD / "objdiff.json").read_text())
    elves = [helpers.reader.Elf(path) for path in (target, compiled)]
    evidence = {
        "scope": "Four root XanimeCore methods; original compiler and retail comparison, no PC activation.",
        "source_sha256_at_compile": source_sha, "header_sha256_at_compile": header_sha,
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "compiler": "GC/3.0a3 with configure.py cflags_game, RMGK01 VERSION=0",
        "tool_versions": {"dtk": "v1.8.3", "objdiff-cli": "v3.6.1", "sjiswrap": "v1.2.2"},
        "tool_hashes": {path: sha(ROOT / path) for path in (
            "build/compilers/GC/3.0a3/mwcceppc.exe", "build/tools/sjiswrap.exe",
            "build/tools/dtk", "build/tools/objdiff-cli")},
        "target_object_sha256": sha(target), "compiled_object_sha256": sha(compiled),
        "normalization": "Control events retain branch destinations, arithmetic opcodes and call order; erase register names. Full instruction check for calc/init/MayaNoTransform only normalizes relocation constant names and relative branch addresses, preserving operands/registers.",
        "functions": [],
    }
    aligned = []
    for short_name, address, size in FUNCTIONS:
        sides = [next(s for s in diff[key]["symbols"] if s["name"].startswith(short_name + "__10XanimeCore"))
                 for key in ("left", "right")]
        name = sides[0]["name"]
        assert int(sides[0]["size"]) == size
        refs = [elf.references(name) for elf in elves]
        calls = [[r["symbol"] for r in side if r["kind"] == 10] for side in refs]
        assert calls[0] == calls[1], (short_name, "Direct calls differ")
        constants = [set(r["value_hex"] for r in side if "value_hex" in r) for side in refs]
        assert constants[0] == constants[1], (short_name, "Constants differ")
        globals_used = [set((r["symbol"], r["addend"]) for r in side if r["kind"] != 10 and "value_hex" not in r)
                        for side in refs]
        assert globals_used[0] == globals_used[1], (short_name, "External globals differ")
        events = [helpers.normalized_events(side) for side in sides]
        assert events[0] == events[1], (short_name, "Arithmetic/helper/control topology differs")
        full_match = normalized_instructions(sides[0], refs[0]) == normalized_instructions(sides[1], refs[1])
        if short_name != "calcScaleBlendBasic":
            assert full_match, (short_name, "Relocation-normalized instructions differ")
            assert int(sides[1]["size"]) == size
        else:
            assert int(sides[1]["size"]) == size + 4
            assert sides[0]["match_percent"] >= 84.0
            float_operations = [[i["formatted"].strip() for i in helpers.instructions(side)
                                 if i["formatted"].strip().split()[0] in ("fmuls", "fcmpu")] for side in sides]
            assert float_operations[0] == float_operations[1]
        entry = {
            "method": short_name, "symbol": name, "address": hex(address), "retail_size": size,
            "compiled_size": int(sides[1]["size"]), "objdiff_match_percent": sides[0]["match_percent"],
            "source_body_sha256": hashlib.sha256(body(source, short_name).encode()).hexdigest(),
            "retail_function_sha256": hashlib.sha256(helpers.reader.dol_bytes(dol, address, size)).hexdigest(),
            "direct_calls": calls[0], "retail_references": refs[0], "compiled_references": refs[1],
            "identical_normalized_event_count": len(events[0]), "identical_normalized_events": events[0],
            "full_relocation_normalized_instruction_match": full_match,
        }
        evidence["functions"].append(entry)
        print(f"{short_name}: {sides[0]['match_percent']:.6f}%, {size}/{sides[1]['size']} bytes, "
              f"{len(events[0])} identical arithmetic/helper/control events, full instruction match={full_match}")
        aligned.append(f"\n{name}: {address:#x}, {sides[0]['match_percent']}%")
        for left, right in zip(sides[0]["instructions"], sides[1]["instructions"]):
            cells = [f"{side.get('diff_kind', ''):20} {side.get('instruction', {}).get('formatted', '')}"
                     for side in (left, right)]
            aligned.append(f"{cells[0]:120} | {cells[1]}")
    (BUILD / "entrypoints-comparison.txt").write_text("\n".join(aligned) + "\n")
    (BUILD / "entrypoints-compiler-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
