#!/usr/bin/env python3
"""Original-compiler and retail proof for the KCL prism narrow phase."""
import ast
import copy
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
BUILD = ROOT / "build/original-kcollision-query-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("parts_proof", HERE.parent / "original-collision-parts-owner-20260903/verify-source.py")
parts = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parts)
proof, reader = parts.proof, parts.reader
FUNCTIONS = (
    ("KCHitSphere__16KCollisionServerFP12KC_PrismDataP4FxyzffPfPUc", 0x80184640, 0x6B0),
    ("KCHitSphereWithThickness__16KCollisionServerFP12KC_PrismDataP4FxyzffPfPUcf", 0x80184CF0, 0x67C),
    ("KCHitArrow__16KCollisionServerCFP12KC_PrismDataRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PfPUc", 0x80185448, 0x2C0),
)


def run(command, log):
    result = subprocess.run([str(item) for item in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def canonicalize(sides, objects, name, dol):
    sides = copy.deepcopy(sides)
    changes = []
    if name.startswith("KCHitArrow__"):
        rows = [[row for row in side["instructions"] if "instruction" in row] for side in sides]
        expected = ["lfs f2, 0x8(r1)", "addi r3, r1, 0x8", "lfs f1, 0xc(r1)", "addi r4, r1, 0x14",
                    "lfs f0, 0x10(r1)", "fmuls f2, f2, f31", "fmuls f1, f1, f31", "fmuls f0, f0, f31",
                    "stfs f2, 0x8(r1)", "stfs f1, 0xc(r1)", "stfs f0, 0x10(r1)"]
        assert [row["instruction"]["formatted"] for row in rows[1][52:63]] == expected
        assert [row["instruction"]["formatted"] for row in rows[0][52:57]] == [
            "fmr f1, f31", "addi r3, r1, 0x8", "bl scale__Q29JGeometry8TVec3<f>Ff", "addi r3, r1, 0x8", "addi r4, r1, 0x14"]
        # The actual callee loads all XYZ first, multiplies each by f1 with
        # fmuls, then stores all three. This is the expanded block above with
        # the caller's f31 scalar and stack-vector address substituted.
        assert reader.dol_bytes(dol, 0x800200D0, 0x28).hex() == (
            "c0630000c0430004c0030008ec630072ec420072ec000072d0630000d0430004d00300084e800020")
        start = int(rows[1][52]["instruction"]["address"])
        replacement = copy.deepcopy(rows[0][52:57])
        for index, row in enumerate(replacement):
            row["instruction"]["address"] = str(start + index * 4)
        rows[1][52:63] = replacement
        sides[1]["instructions"] = rows[1]
        changes.append("One inlined TVec3f::scale: validated every XYZ load/multiply/store against the raw retail callee at 0x800200d0. Caller argument setup and dead scratch FPRs differ.")
    canonical = [proof.normalize(side, obj.references(name), "lifecycle", bool(index))
                 for index, (side, obj) in enumerate(zip(sides, objects))]
    if name.startswith("KCHitSphere"):
        canonical[0] = [re.sub(r"\bf(30|31)\b", lambda match: "f" + str(61 - int(match[1])), line)
                        if not line.startswith(("stfd ", "psq_st ", "lfd ", "psq_l ")) else line
                        for line in canonical[0]]
        changes.append("Computational f30/f31 allocation is swapped; ABI save/restore instructions remain unchanged.")
    assert canonical[0] == canonical[1], (name, [(i, a, b) for i, (a, b) in enumerate(zip(*canonical)) if a != b])
    return canonical[0], changes


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
    command += ["-c", "src/Game/Map/KCollision.cpp", "-o", str(BUILD / "KCollision.o")]
    (BUILD / "KCollision.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "KCollision.compile.log")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    target_path = BUILD / "retail/obj/Game/Map/KCollision.o"
    run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / "KCollision.o",
         "-o", BUILD / "KCollision.diff.json", "--format", "json-pretty"], "KCollision.objdiff.log")
    objects = (reader.Elf(target_path), reader.Elf(BUILD / "KCollision.o"))
    diff = json.loads((BUILD / "KCollision.diff.json").read_text())
    records = []
    for name, address, size in FUNCTIONS:
        sides = [next(symbol for symbol in diff[side]["symbols"] if symbol["name"] == name) for side in ("left", "right")]
        for side in sides:
            for row in side["instructions"]:
                if "instruction" in row:
                    row["instruction"].setdefault("address", "0")
        canonical, changes = canonicalize(sides, objects, name, dol)
        code, relocations = proof.relocated(objects[1], objects[0], name, address, size, dol)
        records.append({"name": name, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                        "objdiff_match_percent": sides[0]["match_percent"], "all_canonical_instructions_equal": True,
                        "normalization": changes, "relocations": relocations, "canonical_instructions": canonical})
        print(name.split("__")[0], sides[0]["match_percent"], "all instructions verified")
    evidence = {"compiler": "GC3.0a3 with configure.py Game flags and Shift-JIS wrapper",
                "dol_sha1": hashlib.sha1(dol).hexdigest(), "functions": records,
                "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
                                  for path in ("src/Game/Map/KCollision.cpp", "include/Game/Map/KCollision.hpp")},
                "scope": "Root prism narrow-phase recovery only. No native owner/query activation or runtime test claim."}
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
