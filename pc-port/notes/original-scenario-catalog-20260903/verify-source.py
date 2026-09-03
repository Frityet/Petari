#!/usr/bin/env python3
"""Verify original ScenarioData accessors and their retail JMap lookup."""
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
BUILD = ROOT / "build/original-scenario-catalog-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("collision_proof", HERE.parent / "original-collision-owner-20260903/verify-source.py")
previous = importlib.util.module_from_spec(spec)
spec.loader.exec_module(previous)
proof, reader = previous.proof, previous.reader

FUNCTIONS = (
    ("getScenarioNum__12ScenarioDataCFv", 0x803A8CE8, 0x98),
    ("getPowerStarNum__12ScenarioDataCFv", 0x803A8D80, 0x98),
    ("getScenarioDataIter__12ScenarioDataCFl", 0x803A8ED0, 0xDC),
    ("getValueBool__12ScenarioDataCFPCclPb", 0x803A9468, 0xA4),
)
STRINGS = {"lbl_805DBF63": "IsHidden", "lbl_805DBF6C": "PowerStarId", "lbl_805DBF81": "ScenarioNo"}



def run(command, log):
    result = subprocess.run([str(item) for item in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def canonicalize(sides, objects, name, dol):
    sides = copy.deepcopy(sides)
    changes = []
    if name.startswith("getScenarioDataIter__"):
        rows = [[row for row in side["instructions"] if "instruction" in row] for side in sides]
        if len(rows[0]) != len(rows[1]):
            normalized = [proof.normalize(side, obj.references(name), "lifecycle", bool(i))
                          for i, (side, obj) in enumerate(zip(sides, objects))]
            assert normalized[0][11:16] == ["mr r3, r31", "mr r4, r29", "mr r5, r30", "addi r6, r1, 0x8",
                                            "bl getValue<l>__8JMapInfoCFiPCcPl_Cb"]
            assert normalized[1][11:21] == ["mr r3, r31", "mr r4, r30", "bl searchItemInfo__8JMapInfoCFPCc",
                                            "cmpwi r3, 0x0", "mr r5, r3", "blt instruction_21", "mr r3, r31",
                                            "mr r4, r29", "addi r6, r1, 0x8", "bl getValueFast__8JMapInfoCFiiPl"]
            # The complete retail getValue<s32> body saves this/row/output,
            # searches the key, returns false without writing when absent,
            # and otherwise calls getValueFast(this,row,index,output). This
            # caller ignores the bool return; the inlined false-return store
            # is consequently dead. Its two original call destinations are
            # searchItemInfo 0x80406154 and getValueFast(s32) 0x80406344.
            assert reader.dol_bytes(dol, 0x800B8BE8, 0x64).hex() == (
                "9421ffe07c0802a690010024396100204845fe117c9e23787c7d1b787ca42b787cdf3378"
                "4834d5492c0300007c651b784080000c38600000480000147fa3eb787fc4f3787fe6fb78"
                "4834d715396100204845fe1d800100247c0803a6382100204e800020")
            start = int(rows[1][11]["instruction"]["address"])
            replacement = copy.deepcopy(rows[0][11:16])
            for index, row in enumerate(replacement):
                row["instruction"]["address"] = str(start + index * 4)
            rows[1][11:21] = replacement
            sides[1]["instructions"] = rows[1]
            changes.append("One inlined JMapInfo::getValue<s32>: verified the complete 100-byte retail callee, the same lookup/negative-index guard/read and argument identities; its unused bool result and private ABI save/restore are eliminated. Every remaining caller instruction is retained.")
    normalized = [proof.normalize(side, obj.references(name), "lifecycle", bool(i))
                  for i, (side, obj) in enumerate(zip(sides, objects))]
    assert normalized[0] == normalized[1], (name, [(i,a,b) for i,(a,b) in enumerate(zip(*normalized)) if a != b])
    return normalized[0], changes


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
    command += ["-c", "src/Game/System/ScenarioDataParser.cpp", "-o", str(BUILD / "ScenarioDataParser.o")]
    (BUILD / "ScenarioDataParser.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "ScenarioDataParser.compile.log")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    target_path = BUILD / "retail/obj/Game/System/ScenarioDataParser.o"
    run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / "ScenarioDataParser.o",
         "-o", BUILD / "ScenarioDataParser.diff.json", "--format", "json-pretty"], "ScenarioDataParser.objdiff.log")
    target, compiled = reader.Elf(target_path), reader.Elf(BUILD / "ScenarioDataParser.o")
    diff = json.loads((BUILD / "ScenarioDataParser.diff.json").read_text())
    records = []
    for symbol, value in STRINGS.items():
        assert reader.dol_bytes(dol, int(symbol[4:], 16), len(value) + 1) == value.encode() + b"\0"
    def with_strings(symbol):
        return [{**ref, "value_hex": (STRINGS[ref["symbol"]].encode() + b"\0").hex()}
                if ref["symbol"] in STRINGS else ref for ref in target.references(symbol)]
    target_with_strings = types.SimpleNamespace(references=with_strings)
    for name, address, size in FUNCTIONS:
        sides = [next(symbol for symbol in diff[side]["symbols"] if symbol["name"] == name) for side in ("left", "right")]
        for side in sides:
            for row in side["instructions"]:
                if "instruction" in row:
                    row["instruction"].setdefault("address", "0")
        canonical, changes = canonicalize(sides, (target_with_strings, compiled), name, dol)
        code, relocations = proof.relocated(compiled, target_with_strings, name, address, size, dol)
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
                          for path in ("src/Game/System/ScenarioDataParser.cpp", "include/Game/System/ScenarioDataParser.hpp")},
        "scope": "Root ScenarioData accessor recovery only. No native catalog/scene construction or collision query activation."
    }
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
