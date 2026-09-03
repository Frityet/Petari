#!/usr/bin/env python3
"""Verify the recovered CollisionParts sphere/line query group against RMGK01."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-collision-parts-owner-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("collision_proof", HERE.parent / "original-collision-owner-20260903/verify-source.py")
previous = importlib.util.module_from_spec(spec)
spec.loader.exec_module(previous)
proof, reader = previous.proof, previous.reader

FUNCTIONS = (
    ("checkStrikeBall__14CollisionPartsFP7HitInfoUlRCQ29JGeometry8TVec3<f>fbPC18TriangleFilterBase", 0x8017677C, 0x2CC),
    ("checkStrikeBallCore__14CollisionPartsFP7HitInfoUlRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>fffPP12KC_PrismDataPfPUcPC18TriangleFilterBasePCQ29JGeometry8TVec3<f>", 0x80176A48, 0x1A8),
    ("checkStrikeBallWithThickness__14CollisionPartsFP7HitInfoUlRCQ29JGeometry8TVec3<f>ffPC18TriangleFilterBase", 0x80176BF0, 0x1B8),
    ("calcCollidePosition__14CollisionPartsFPQ29JGeometry8TVec3<f>RC12KC_PrismDataUc", 0x80176DA8, 0x348),
    ("checkStrikeLine__14CollisionPartsFP7HitInfoUlRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PC18TriangleFilterBase", 0x801771A4, 0x1EC),
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
        # The incoming position/boolean use r30/r29 until this point. Rename
        # only the subsequent disjoint sweep-loop live ranges.
        left = rename_registers(left, 74, 144, {29: 30, 30: 31, 31: 29})
        changes.append("Loop-only GPR allocation: retail r29/r30/r31 -> compiled r30/r31/r29, instruction indices [74,144).")
        assert left[92:95] == ["stfs f0, 0x48(r1)", "stfs f0, 0x44(r1)", "stfs f0, 0x40(r1)"]
        assert right[92:95] == ["stfs f0, 0x40(r1)", "stfs f0, 0x44(r1)", "stfs f0, 0x48(r1)"]
        left[92:95] = reversed(left[92:95])
        changes.append("Three independent stores of the same loaded zero initialize offset.z/y/x versus offset.x/y/z.")
    elif name.startswith("checkStrikeBallCore__"):
        left = rename_registers(left, 0, len(left), {25: 26, 26: 27, 27: 28, 28: 25})
        changes.append("GPR allocation: retail r25/r26/r27/r28 -> compiled r26/r27/r28/r25.")
    elif name.startswith("checkStrikeBallWithThickness__"):
        assert left[42:44] == ["fmuls f1, f30, f31", "fmr f2, f31"]
        assert right[42:44] == ["fmr f2, f31", "fmuls f1, f30, f31"]
        left[42:44] = reversed(left[42:44])
        changes.append("Independent radius multiplication and scale argument copy are scheduled in reverse order.")
    assert left == right, (name, [(index, a, b) for index, (a, b) in enumerate(zip(left, right)) if a != b])
    return left, changes


def relocate_collide_position(compiled, target, name, address, dol):
    # A relocated jump table is not a scalar constant: verify its eight
    # function-relative relocations and their actual loaded retail addresses.
    tables = [next(ref["symbol"] for ref in obj.references(name) if ref["kind"] == 6)
              for obj in (target, compiled)]
    table_refs = [obj.references(table) for obj, table in zip((target, compiled), tables)]
    assert table_refs[0] == table_refs[1]
    assert len(table_refs[0]) == 8
    for index, ref in enumerate(table_refs[0]):
        assert ref["symbol"] == name and ref["kind"] == 1 and int(ref["offset"], 16) == index * 4
        assert reader.dol_bytes(dol, 0x80588F00 + index * 4, 4) == struct.pack(">I", address + ref["addend"])
    symbols = {name: int(value, 16) for name, value in re.findall(
        r"^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);", (ROOT / "config/RMGK01/symbols.txt").read_text(), re.M)}
    _, start, size, section = next(symbol for symbol in compiled.symbols if symbol[0] == name)
    code = bytearray(compiled.section_data(section)[start:start + size])
    records = []
    for ref in compiled.references(name):
        offset, kind = int(ref["offset"], 16), ref["kind"]
        if kind in (4, 6):
            assert ref["symbol"] == tables[1] and ref["addend"] == 0
            effective = 0x80588F00
            value = effective if kind == 4 else (effective + 0x8000) >> 16
            struct.pack_into(">H", code, offset, value & 0xFFFF)
        else:
            assert kind == 10
            effective = symbols[ref["symbol"]] + ref["addend"]
            word = struct.unpack_from(">I", code, offset)[0]
            word = (word & 0xFC000003) | ((effective - address - offset) & 0x3FFFFFC)
            struct.pack_into(">I", code, offset, word)
        records.append({**ref, "effective_retail_target": hex(effective)})
    return bytes(code), records


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
    command += ["-c", "src/Game/Map/CollisionParts.cpp", "-o", str(BUILD / "CollisionParts.o")]
    (BUILD / "CollisionParts.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "CollisionParts.compile.log")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    target_path = BUILD / "retail/obj/Game/Map/CollisionParts.o"
    run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / "CollisionParts.o",
         "-o", BUILD / "CollisionParts.diff.json", "--format", "json-pretty"], "CollisionParts.objdiff.log")
    target, compiled = reader.Elf(target_path), reader.Elf(BUILD / "CollisionParts.o")
    diff = json.loads((BUILD / "CollisionParts.diff.json").read_text())
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
        if name.startswith("calcCollidePosition__"):
            code, relocations = relocate_collide_position(compiled, target, name, address, dol)
        else:
            code, relocations = proof.relocated(compiled, target, name, address, size, dol)
        retail = reader.dol_bytes(dol, address, size)
        if not changes:
            assert code == retail, name
        records.append({"name": name, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                        "objdiff_match_percent": sides[0]["match_percent"], "relocated_bytes_equal": code == retail,
                        "all_canonical_instructions_equal": True, "normalization": changes, "relocations": relocations,
                        "canonical_instructions": canonical})
        print(name.split("__")[0], sides[0]["match_percent"], "all instructions verified")
    assert reader.dol_bytes(dol, 0x806BBE84, 4).hex() == "3cea0ea1"
    assert reader.dol_bytes(dol, 0x80588F00, 32).hex() == (
        "801770d480176dec80176e2c80176ef080176fb48017707880177098801770b8")
    evidence = {
        "compiler": "GC3.0a3 with configure.py Game flags and Shift-JIS wrapper",
        "dol_sha1": hashlib.sha1(dol).hexdigest(), "functions": records,
        "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
                          for path in ("src/Game/Map/CollisionParts.cpp", "include/Game/Map/CollisionParts.hpp")},
        "constants": {"sphere_motion_step_reciprocal": {"address": "0x806bbe84", "bits": "3cea0ea1", "expression": "1.0f / 35.0f"}},
        "jump_table": {"address": "0x80588f00", "entries": ["no-op", "face", "edge0", "edge1", "edge2", "vertex0", "vertex1", "vertex2"]},
        "scope": "Root source recovery only. No native CollisionParts or collision query activation; KCL narrow phase and keeper queries remain missing."
    }
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
