#!/usr/bin/env python3
"""Verify the recovered original keeper broad-phase queries against RMGK01."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-collision-keeper-query-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("collision_proof", HERE.parent / "original-collision-owner-20260903/verify-source.py")
previous = importlib.util.module_from_spec(spec)
spec.loader.exec_module(previous)
proof, reader = previous.proof, previous.reader

FUNCTIONS = (
    ("checkStrikeBall__26CollisionCategorizedKeeperFRCQ29JGeometry8TVec3<f>fbPC24CollisionPartsFilterBasePC18TriangleFilterBase", 0x80173FC4, 0x24C),
    ("checkStrikeBallWithThickness__26CollisionCategorizedKeeperFRCQ29JGeometry8TVec3<f>ffPC24CollisionPartsFilterBasePC18TriangleFilterBase", 0x80174210, 0x25C),
    ("checkStrikeLine__26CollisionCategorizedKeeperFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>lPC24CollisionPartsFilterBasePC18TriangleFilterBase", 0x8017446C, 0x2BC),
    ("isSphereOverlappingWithBox__26CollisionCategorizedKeeperFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>f", 0x80174A40, 0x8C),
)


def run(command, log):
    result = subprocess.run([str(item) for item in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def rename_registers(lines, first, last, mapping):
    return [re.sub(r"\br(\d+)\b", lambda match: "r" + str(mapping.get(int(match[1]), int(match[1]))), line)
            if first <= index < last else line for index, line in enumerate(lines)]


def normalize_all(name, left, right):
    changes = []
    if name.startswith("checkStrikeBall__"):
        left = rename_registers(left, 0, len(left), {21: 22, 22: 23, 23: 24, 24: 25, 25: 26, 26: 27, 27: 28, 28: 21})
        changes.append("Complete GPR allocation: retail r21/r22/r23/r24/r25/r26/r27/r28 -> compiled r22/r23/r24/r25/r26/r27/r28/r21. All instructions retained.")
    elif name.startswith("checkStrikeBallWithThickness__"):
        left = rename_registers(left, 0, len(left), {21: 22, 22: 21})
        changes.append("Complete GPR allocation: retail r21/r22 -> compiled r22/r21. All instructions retained.")
    assert left == right, (name, [(index, a, b) for index, (a, b) in enumerate(zip(left, right)) if a != b])
    return left, changes


def main():
    BUILD.mkdir(exist_ok=True, parents=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == "cflags_game" for target in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Game compiler flags not found")
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command += ["-c", "src/Game/Map/CollisionCategorizedKeeper.cpp", "-o", str(BUILD / "CollisionCategorizedKeeper.o")]
    (BUILD / "CollisionCategorizedKeeper.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "CollisionCategorizedKeeper.compile.log")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    target_path = BUILD / "retail/obj/Game/Map/CollisionCategorizedKeeper.o"
    run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / "CollisionCategorizedKeeper.o",
         "-o", BUILD / "CollisionCategorizedKeeper.diff.json", "--format", "json-pretty"], "CollisionCategorizedKeeper.objdiff.log")
    target, compiled = reader.Elf(target_path), reader.Elf(BUILD / "CollisionCategorizedKeeper.o")
    diff = json.loads((BUILD / "CollisionCategorizedKeeper.diff.json").read_text())
    records = []
    for name, address, size in FUNCTIONS:
        sides = [next(symbol for symbol in diff[side]["symbols"] if symbol["name"] == name) for side in ("left", "right")]
        for side in sides:
            for row in side["instructions"]:
                if "instruction" in row:
                    row["instruction"].setdefault("address", "0")
        canonical = [proof.normalize(sides[i], obj.references(name), "lifecycle", bool(i))
                     for i, obj in enumerate((target, compiled))]
        canonical, changes = normalize_all(name, *canonical)
        code, relocations = proof.relocated(compiled, target, name, address, size, dol)
        retail = reader.dol_bytes(dol, address, size)
        if not changes:
            assert code == retail, name
        records.append({"name": name, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                        "objdiff_match_percent": sides[0]["match_percent"], "relocated_bytes_equal": code == retail,
                        "all_canonical_instructions_equal": True, "normalization": changes, "relocations": relocations,
                        "canonical_instructions": canonical})
        print(name.split("__")[0], sides[0]["match_percent"], "all instructions verified")
    evidence = {
        "compiler": "GC3.0a3 with configure.py Game flags and Shift-JIS wrapper",
        "dol_sha1": hashlib.sha1(dol).hexdigest(), "functions": records,
        "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
                          for path in ("src/Game/Map/CollisionCategorizedKeeper.cpp", "include/Game/Map/CollisionCategorizedKeeper.hpp")},
        "scope": "Root sphere/thickness/line broad-phase recovery and original-compiler/retail proof only. No native keeper, scene owner, or production query activation."
    }
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
