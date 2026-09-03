#!/usr/bin/env python3
"""Verify J3D model dispatchers and the existing original addMesh body."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import shlex
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-sdk-model-dispatch-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("collision_proof", HERE.parent / "original-collision-owner-20260903/verify-source.py")
previous = importlib.util.module_from_spec(spec)
spec.loader.exec_module(previous)
proof, reader = previous.proof, previous.reader

FUNCTIONS = (
    ("loadMaterialTable__22J3DModelLoaderDataBaseFPCv", 0x8043DDBC, 0x80),
    ("loadBinaryDisplayList__22J3DModelLoaderDataBaseFPCvUl", 0x8043DE3C, 0x94),
)
HIERARCHY = "makeHierarchy__12J3DJointTreeFP8J3DJointPPC17J3DModelHierarchyP16J3DMaterialTableP13J3DShapeTable"
ADD_MESH = "addMesh__8J3DJointFP11J3DMaterial"


def run(command, log):
    result = subprocess.run([str(item) for item in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def symbol(diff, side, name):
    result = next(entry for entry in diff[side]["symbols"] if entry["name"] == name)
    for row in result["instructions"]:
        if "instruction" in row:
            row["instruction"].setdefault("address", "0")
    return result


def main():
    BUILD.mkdir(exist_ok=True, parents=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == "cflags_jsys" for target in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("JSystem compiler flags not found")
    base = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        base.extend(shlex.split(flag))
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")

    diffs, objects = {}, {}
    for folder, unit in (("J3DGraphLoader", "J3DModelLoader"), ("J3DGraphAnimator", "J3DJointTree")):
        source = f"src/JSystem/{folder}/{unit}.cpp"
        command = base + ["-c", source, "-o", str(BUILD / f"{unit}.o")]
        (BUILD / f"{unit}.command.json").write_text(json.dumps(command, indent=2) + "\n")
        run(command, f"{unit}.compile.log")
        target_path = BUILD / f"retail/obj/JSystem/{folder}/{unit}.o"
        run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / f"{unit}.o",
             "-o", BUILD / f"{unit}.diff.json", "--format", "json-pretty"], f"{unit}.objdiff.log")
        diffs[unit] = json.loads((BUILD / f"{unit}.diff.json").read_text())
        objects[unit] = reader.Elf(target_path), reader.Elf(BUILD / f"{unit}.o")

    records = []
    target, compiled = objects["J3DModelLoader"]
    for name, address, size in FUNCTIONS:
        sides = [symbol(diffs["J3DModelLoader"], side, name) for side in ("left", "right")]
        normalized = [proof.normalize(side, obj.references(name), "lifecycle", bool(i))
                      for i, (side, obj) in enumerate(zip(sides, (target, compiled)))]
        assert normalized[0] == normalized[1], name
        code, relocations = proof.relocated(compiled, target, name, address, size, dol)
        assert code == reader.dol_bytes(dol, address, size), name
        records.append({"name": name, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                        "objdiff_match_percent": sides[0]["match_percent"], "relocated_bytes_equal": True,
                        "all_canonical_instructions_equal": True, "relocations": relocations,
                        "canonical_instructions": normalized[0]})
        print(name.split("__")[0], sides[0]["match_percent"], "relocated bytes equal")

    # addMesh was already an inline root definition. Its five-instruction
    # retail expansion uses r23=this and r28=material in makeHierarchy.
    # The current original compiler chooses an out-of-line weak body with
    # the ordinary ABI arguments r3/r4; verify all five operations and blr.
    diff = diffs["J3DJointTree"]
    target, compiled = objects["J3DJointTree"]
    body = symbol(diff, "right", ADD_MESH)
    normalized = proof.normalize(body, compiled.references(ADD_MESH), "lifecycle", True)
    expected = ["lwz r0, 0x58(r3)", "cmpwi r0, 0x0", "beq instruction_4",
                "stw r0, 0x4(r4)", "stw r4, 0x58(r3)", "blr"]
    assert normalized == expected, normalized
    retail_bytes = bytes.fromhex("801700582c00000041820008901c000493970058")
    assert reader.dol_bytes(dol, 0x804317DC, len(retail_bytes)) == retail_bytes
    caller_sides = [symbol(diff, side, HIERARCHY) for side in ("left", "right")]
    callers = [proof.normalize(side, obj.references(HIERARCHY), "lifecycle", bool(i))
               for i, (side, obj) in enumerate(zip(caller_sides, (target, compiled)))]
    assert callers[0][70:76] == ["lwz r0, 0x58(r23)", "cmpwi r0, 0x0", "beq instruction_74",
                                "stw r0, 0x4(r28)", "stw r28, 0x58(r23)", "stw r23, 0xc(r28)"]
    assert callers[1][70:74] == ["mr r3, r23", "mr r4, r28", "bl " + ADD_MESH, "stw r23, 0xc(r28)"]
    add_mesh = {"name": ADD_MESH, "root_definition_already_present": True,
                "retail_inline_address": "0x804317dc", "retail_inline_size": len(retail_bytes),
                "retail_inline_bytes": retail_bytes.hex(), "compiled_size": int(body["size"]),
                "all_inline_operations_verified": True, "compiled_canonical_instructions": normalized,
                "caller_argument_identity_verified": True,
                "makeHierarchy_context_objdiff_percent": caller_sides[0]["match_percent"],
                "scope": "Verifies the existing addMesh body and caller argument identity, not every unrelated makeHierarchy instruction."}
    print("addMesh", "all five retail inline operations verified")
    paths = ("src/JSystem/J3DGraphLoader/J3DModelLoader.cpp",
             "libs/JSystem/include/JSystem/J3DGraphLoader/J3DModelLoader.hpp",
             "src/JSystem/J3DGraphAnimator/J3DJointTree.cpp",
             "libs/JSystem/include/JSystem/J3DGraphAnimator/J3DJoint.hpp",
             "libs/JSystem/include/JSystem/J3DGraphBase/J3DMaterial.hpp")
    evidence = {"compiler": "GC3.0a3 with configure.py JSystem flags and Shift-JIS wrapper",
                "dol_sha1": hashlib.sha1(dol).hexdigest(), "functions": records, "existing_addMesh": add_mesh,
                "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in paths},
                "scope": "Two root SDK dispatchers recovered; existing root addMesh verified. No native changes."}
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
