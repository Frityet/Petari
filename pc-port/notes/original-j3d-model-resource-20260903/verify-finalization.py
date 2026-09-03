#!/usr/bin/env python3
"""Check exact native source adaptations and original hierarchy instructions."""
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
BUILD = ROOT / "build/original-j3d-model-finalization-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("sdk_dispatch", HERE.parent / "original-sdk-model-dispatch-20260903/verify-source.py")
sdk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sdk)

TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/|\s+|[A-Za-z_][A-Za-z_0-9]*|0[xX][0-9A-Fa-f]+[uUlL]*|[0-9]+(?:\.[0-9]*)?[fFuUlL]*|::|->|\+\+|--|==|!=|<=|>=|&&|\|\||.', re.S)


def tokens(text):
    return [value for value in TOKEN.findall(text)
            if not value.isspace() and not value.startswith(("//", "/*"))]


def find(sequence, needle, start=0):
    for i in range(start, len(sequence) - len(needle) + 1):
        if sequence[i:i + len(needle)] == needle:
            return i
    raise AssertionError(("Token sequence missing", needle))


def block(sequence, start):
    assert sequence[start] == "{"
    depth = 1
    end = start + 1
    while depth:
        depth += (sequence[end] == "{") - (sequence[end] == "}")
        end += 1
    return sequence[start:end], end


def body(path, declaration):
    source = tokens((ROOT / path).read_text())
    start = find(source, tokens(declaration))
    return block(source, source.index("{", start))[0]


def replace(sequence, before, after):
    before, after = tokens(before), tokens(after)
    result, i = [], 0
    while i < len(sequence):
        if sequence[i:i + len(before)] == before:
            result.extend(after)
            i += len(before)
        else:
            result.append(sequence[i])
            i += 1
    return result


def digest(sequence):
    return hashlib.sha256(" ".join(sequence).encode()).hexdigest()


def verify_sources():
    root_tree = "src/JSystem/J3DGraphAnimator/J3DJointTree.cpp"
    native_tree = "pc-port/src/compat/J3DHierarchyCompat.cpp"
    root_loader = "src/JSystem/J3DGraphLoader/J3DModelLoader.cpp"
    native_loader = "pc-port/src/compat/J3DModelLoaderCompat.cpp"
    records = []
    for label, a, b, da, db in (
        ("makeHierarchy", root_tree, native_tree, "void J3DJointTree::makeHierarchy", "void J3DJointTree::makeHierarchy"),
        ("hierarchy command enum", root_tree, native_tree, "enum", "enum"),
        ("setupBBoardInfo", root_loader, native_loader, "void J3DModelLoader::setupBBoardInfo", "void setup_bboard_info"),
        ("addMesh", "libs/JSystem/include/JSystem/J3DGraphAnimator/J3DJoint.hpp",
         "pc-port/src/JSystem/J3DGraphAnimator/J3DJoint.hpp", "inline void addMesh", "inline void addMesh"),
    ):
        left, right = body(a, da), body(b, db)
        assert left == right, label
        records.append({"name": label, "tokens": len(left), "normalized_token_sha256": digest(left),
                        "root": a, "native": b, "body_tokens_equal": True})

    # The explicit-input billboard helper retains the original field names.
    # Verify its binding signature, then compare the complete body above.
    loader_tokens = tokens((ROOT / native_loader).read_text())
    find(loader_tokens, tokens("void setup_bboard_info(J3DModelData* mpModelData, const J3DShapeBlock* mpShapeBlock)"))
    find(loader_tokens, tokens("void finalize_j3d_model(J3DModelData& model, const J3DShapeBlock& shapes, bool binary_display_list)"))
    combined = body(native_loader, "void finalize_j3d_model")[1:-1]
    gate = find(combined, tokens("if (binary_display_list)"))
    common = combined[:gate]
    yes, end = block(combined, gate + len(tokens("if (binary_display_list)")))
    assert combined[end] == "else"
    no = combined[end + 1:]
    for name, declaration, suffix in (
        ("ordinary model finalization", "J3DModelData* J3DModelLoader::load(", no),
        ("binary model finalization", "J3DModelData* J3DModelLoader::loadBinaryDisplayList(", yes[1:-1]),
    ):
        original = body(root_loader, declaration)
        start = find(original, tokens("J3DModelHierarchy const* hierarchy"))
        finish = find(original, tokens("return mpModelData;"), start)
        original = original[start:finish]
        native = replace(common + suffix, "model .", "mpModelData ->")
        native = replace(native, "setup_bboard_info(&model, &shapes)", "setupBBoardInfo()")
        assert original == native, name
        records.append({"name": name, "tokens": len(original), "normalized_token_sha256": digest(original),
                        "root": root_loader, "native": native_loader, "body_tokens_equal": True,
                        "explicit_adaptations": ["model reference replaces mpModelData member access",
                                                 "model and shape block supplied to original billboard body",
                                                 "binary selector chooses the corresponding original suffix",
                                                 "resource owner handles the original final model return"]})
    return records


def run(command, log):
    result = subprocess.run([str(item) for item in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def verify_hierarchy():
    BUILD.mkdir(exist_ok=True, parents=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_jsys" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("JSystem compiler flags missing")
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command += ["-c", "src/JSystem/J3DGraphAnimator/J3DJointTree.cpp", "-o", str(BUILD / "J3DJointTree.o")]
    (BUILD / "J3DJointTree.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "J3DJointTree.compile.log")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    target_path = BUILD / "retail/obj/JSystem/J3DGraphAnimator/J3DJointTree.o"
    run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / "J3DJointTree.o",
         "-o", BUILD / "J3DJointTree.diff.json", "--format", "json-pretty"], "J3DJointTree.objdiff.log")
    diff = json.loads((BUILD / "J3DJointTree.diff.json").read_text())
    objects = sdk.reader.Elf(target_path), sdk.reader.Elf(BUILD / "J3DJointTree.o")
    sides = [sdk.symbol(diff, side, sdk.HIERARCHY) for side in ("left", "right")]
    normalized = [sdk.proof.normalize(side, obj.references(sdk.HIERARCHY), "lifecycle", bool(i))
                  for i, (side, obj) in enumerate(zip(sides, objects))]
    helper = sdk.symbol(diff, "right", sdk.ADD_MESH)
    helper_body = sdk.proof.normalize(helper, objects[1].references(sdk.ADD_MESH), "lifecycle", True)
    assert helper_body == ["lwz r0, 0x58(r3)", "cmpwi r0, 0x0", "beq instruction_4",
                           "stw r0, 0x4(r4)", "stw r4, 0x58(r3)", "blr"]
    assert sdk.reader.dol_bytes(dol, 0x804317DC, 20).hex() == "801700582c00000041820008901c000493970058"
    assert normalized[1][70:73] == ["mr r3, r23", "mr r4, r28", "bl " + sdk.ADD_MESH]
    assert normalized[0][70:75] == ["lwz r0, 0x58(r23)", "cmpwi r0, 0x0", "beq instruction_74",
                                    "stw r0, 0x4(r28)", "stw r28, 0x58(r23)"]
    # The caller differs only by the verified method's out-of-line emission.
    # Expand that exact call and rebase branch indices beyond its three rows.
    expanded = [re.sub(r"instruction_(\d+)", lambda m: "instruction_" + str(int(m[1]) + (2 if int(m[1]) >= 73 else 0)), row)
                for row in normalized[1]]
    expanded[70:73] = normalized[0][70:75]
    assert normalized[0] == expanded

    # Unlike a string constant, a jump table must retain every relocation.
    # Compare all 19 case targets and the complete absolute retail table.
    tables = []
    for obj in objects:
        name = next(ref["symbol"] for ref in obj.references(sdk.HIERARCHY) if ref["kind"] == 6)
        _, _, size, _ = next(entry for entry in obj.symbols if entry[0] == name)
        assert size == 19 * 4
        refs = obj.references(name)
        assert len(refs) == 19
        for index, ref in enumerate(refs):
            assert ref["offset"] == hex(index * 4) and ref["kind"] == 1 and ref["symbol"] == sdk.HIERARCHY
        tables.append([ref["addend"] for ref in refs])
    assert tables[0] == tables[1]
    table_bytes = b"".join(struct.pack(">I", 0x804316C4 + offset) for offset in tables[0])
    assert sdk.reader.dol_bytes(dol, 0x805E99E8, len(table_bytes)) == table_bytes
    return {"name": sdk.HIERARCHY, "address": "0x804316c4", "retail_size": int(sides[0]["size"]),
            "compiled_size": int(sides[1]["size"]), "objdiff_percent": sides[0]["match_percent"],
            "all_89_canonical_instructions_equal_after_verified_addMesh_expansion": True,
            "canonical_instructions": normalized[0], "jump_table_address": "0x805e99e8",
            "all_19_jump_targets_equal": True, "jump_target_offsets": tables[0],
            "normalization": "Only the complete proven addMesh call is expanded; argument identity and each branch target retained."}


def main():
    correspondence = verify_sources()
    hierarchy = verify_hierarchy()
    paths = sorted({record[side] for record in correspondence for side in ("root", "native")})
    evidence = {"source_correspondence": correspondence, "hierarchy_compiler_evidence": hierarchy,
                "dol_sha1": hashlib.sha1(DOL.read_bytes()).hexdigest(),
                "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in paths},
                "scope": "Source identity/adaptations and original hierarchy proof. Does not certify native owner validation, OS policy, or complete loader execution."}
    (HERE / "finalization-source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    for record in correspondence:
        print(record["name"], record["tokens"], "source tokens match")
    print("makeHierarchy", hierarchy["objdiff_percent"], "all 89 instructions and 19 jump targets verified")


if __name__ == "__main__":
    main()
