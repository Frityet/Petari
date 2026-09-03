#!/usr/bin/env python3
"""Verify original KCL octree traversal and spatial helpers against RMGK01."""
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
BUILD = ROOT / "build/original-kcollision-traversal-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("narrow_proof", HERE.parent / "original-kcollision-query-20260903/verify-source.py")
previous = importlib.util.module_from_spec(spec)
spec.loader.exec_module(previous)
proof, reader = previous.proof, previous.reader
FUNCTIONS = {
    "KCollision": (
        ("checkSphere__16KCollisionServerFP4FxyzffUlPP12KC_PrismDataPfPUc", 0x80183978, 0x2D4),
        ("checkSphereWithThickness__16KCollisionServerFP4FxyzffUlPP12KC_PrismDataPfPUcf", 0x80183C4C, 0x2EC),
        ("checkArrow__16KCollisionServerCFRCQ29JGeometry8TVec3<f>RCQ29JGeometry8TVec3<f>PfPUcPUlPP12KC_PrismDataUl", 0x80183F38, 0x708),
        ("setUsingCast__Q216KCollisionServer3V3uFRCQ29JGeometry8TVec3<f>", 0x80185AF4, 0x48),
        ("find<PP12KC_PrismData,P12KC_PrismData>__3stdFPP12KC_PrismDataPP12KC_PrismDataRCP12KC_PrismData_PP12KC_PrismData", 0x80185B3C, 0x24),
    ),
    "KCollisionPlus": (
        ("isInsideMinMaxInLocalSpace__16KCollisionServerCFRCQ216KCollisionServer3V3u", 0x80185B7C, 0x50),
        ("outCheck__16KCollisionServerCFPCQ29JGeometry8TVec3<f>PCQ29JGeometry8TVec3<f>PQ216KCollisionServer3V3uPQ216KCollisionServer3V3u", 0x80185BCC, 0x128),
        ("objectSpaceToLocalSpace__16KCollisionServerCFPQ216KCollisionServer3V3uRCQ29JGeometry8TVec3<f>", 0x80185CF4, 0x64),
    ),
}


def run(command, log):
    result = subprocess.run([str(item) for item in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def allocation(lines, registers, stack):
    """Rename complete disjoint register and local-stack live ranges."""
    result = []
    for line in lines:
        line = re.sub(r"\br(\d+)\b", lambda match: "r" + str(registers.get(int(match[1]), int(match[1]))), line)
        line = re.sub(r"0x([0-9a-f]+)(?=\(r1\))", lambda match: hex(stack.get(int(match[1], 16), int(match[1], 16))), line)
        if line.startswith("addi ") and ", r1, " in line:
            head, value = line.rsplit(", ", 1)
            offset = int(value, 16)
            line = head + ", " + hex(stack.get(offset, offset))
        result.append(line)
    return result


def leaf_stride_dataflow(lines):
    """Decode every operation in the one block with scratch-register reuse.

    Input registers are independent symbols. Every produced value, load,
    comparison and branch is retained in the trace, not merely the final
    result. r0/r5/r6/r7/r8/r9 are dead at the block boundary; all other GPRs
    and the comparison state are checked separately.
    """
    registers = {"r" + str(index): ("input", index) for index in range(32)}
    trace = []
    comparison = None
    for line in lines:
        operation, operands = line.split(" ", 1)
        args = operands.split(", ")
        if operation == "lwz":
            offset, base = re.fullmatch(r"(0x[0-9a-f]+)\((r\d+)\)", args[1]).groups()
            value = ("load_u32", registers[base], int(offset, 16))
        elif operation == "li":
            value = ("literal", int(args[1], 16))
        elif operation == "mr":
            value = registers[args[1]]
        elif operation in ("slw", "and"):
            value = (operation, registers[args[1]], registers[args[2]])
        elif operation == "subi":
            value = ("subtract", registers[args[1]], ("literal", int(args[2], 16)))
        elif operation == "subf":
            value = ("subtract", registers[args[2]], registers[args[1]])
        elif operation == "cmpw":
            comparison = ("compare_signed", registers[args[0]], registers[args[1]])
            trace.append(comparison)
            continue
        elif operation == "bge":
            assert comparison is not None
            trace.append(("branch_ge", operands, comparison))
            continue
        else:
            raise AssertionError((operation, line))
        registers[args[0]] = value
        trace.append((operation, value))
    live = {name: value for name, value in registers.items()
            if name not in {"r0", "r5", "r6", "r7", "r8", "r9"}}
    return trace, live, comparison


def canonicalize(name, left, right, dol):
    changes = []
    if name.startswith("checkSphere"):
        right = allocation(right, {28: 23, 27: 22, 26: 21, 25: 26, 24: 27, 23: 25, 22: 24, 21: 28},
                           {0x48: 0x30, 0x4C: 0x34, 0x50: 0x38, 0x3C: 0x24, 0x40: 0x28, 0x44: 0x2C,
                            0x30: 0x48, 0x34: 0x4C, 0x38: 0x50, 0x24: 0x3C, 0x28: 0x40, 0x2C: 0x44,
                            0x20: 0x10, 0x1C: 0x18, 0x18: 0x1C, 0x14: 0x20, 0x10: 0xC, 0xC: 0x14})
        start = 71 if name.startswith("checkSphereWithThickness") else 68
        end = start + 17
        expected, actual = leaf_stride_dataflow(left[start:end]), leaf_stride_dataflow(right[start:end])
        assert expected == actual, (name, expected, actual)
        right[start:end] = left[start:end]
        changes += ["Complete local-stack/GPR allocation renaming; pointers and every stack address remain distinct.",
                    "Leaf-stride block: decoded all 17 instructions into symbolic load/arithmetic/comparison/branch values and checked every live GPR at its exit. Only dead scratch register reuse differs."]
    elif name.startswith("checkArrow__"):
        right = allocation(right, {30: 29, 29: 28, 28: 27, 27: 26, 26: 25, 25: 24, 24: 23, 23: 22, 22: 30},
                           {0x2C: 0x28, 0x28: 0x24, 0x24: 0x20, 0x20: 0x1C, 0x1C: 0x18, 0x18: 0x14, 0x14: 0x2C})
        right = [re.sub(r"\bf(27|28)\b", lambda match: "f" + str(55 - int(match[1])), line)
                 if 356 <= index < 434 else line for index, line in enumerate(right)]
        for address, bits in ((0x80532B20, "4330000000000000"), (0x80532B18, "4330000080000000")):
            assert reader.dol_bytes(dol, address, 8).hex() == bits
            left = [line.replace("lbl_" + hex(address)[2:].upper(), "constant_" + bits) for line in left]
        changes += ["Complete local-stack/GPR allocation renaming and f27/f28 swap only during the later per-axis traversal-distance live range [356,434); preserved all 450 instructions.",
                    "Resolved both external integer-to-double conversion constants directly from the verified retail DOL."]
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
    records = []
    for unit, functions in FUNCTIONS.items():
        command = base + ["-c", f"src/Game/Map/{unit}.cpp", "-o", str(BUILD / f"{unit}.o")]
        (BUILD / f"{unit}.command.json").write_text(json.dumps(command, indent=2) + "\n")
        run(command, f"{unit}.compile.log")
        target_path = BUILD / f"retail/obj/Game/Map/{unit}.o"
        run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / f"{unit}.o",
             "-o", BUILD / f"{unit}.diff.json", "--format", "json-pretty"], f"{unit}.objdiff.log")
        target, compiled = reader.Elf(target_path), reader.Elf(BUILD / f"{unit}.o")
        diff = json.loads((BUILD / f"{unit}.diff.json").read_text())
        for name, address, size in functions:
            sides = [next(symbol for symbol in diff[side]["symbols"] if symbol["name"] == name) for side in ("left", "right")]
            for side in sides:
                for row in side["instructions"]:
                    if "instruction" in row:
                        row["instruction"].setdefault("address", "0")
            canonical = [proof.normalize(sides[i], obj.references(name), "lifecycle", bool(i))
                         for i, obj in enumerate((target, compiled))]
            canonical, changes = canonicalize(name, *canonical, dol)
            external_constants = {"lbl_80532B20": "4330000000000000", "lbl_80532B18": "4330000080000000"}
            target_with_constants = types.SimpleNamespace(references=lambda symbol: [
                {**ref, "value_hex": external_constants[ref["symbol"]]} if ref["symbol"] in external_constants else ref
                for ref in target.references(symbol)])
            code, relocations = proof.relocated(compiled, target_with_constants, name, address, size, dol)
            retail = reader.dol_bytes(dol, address, size)
            if not changes:
                assert code == retail, name
            records.append({"name": name, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                            "objdiff_match_percent": sides[0]["match_percent"], "relocated_bytes_equal": code == retail,
                            "all_canonical_instructions_equal": True, "normalization": changes, "relocations": relocations,
                            "canonical_instructions": canonical})
            print(name.split("__")[0], sides[0]["match_percent"], "all instructions verified")
    sources = ("src/Game/Map/KCollision.cpp", "src/Game/Map/KCollisionPlus.cpp", "include/Game/Map/KCollision.hpp")
    evidence = {"compiler": "GC3.0a3 with configure.py Game flags and Shift-JIS wrapper",
                "dol_sha1": hashlib.sha1(dol).hexdigest(), "functions": records,
                "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in sources},
                "scope": "Root query traversal recovery and original compiler/retail verification only; no native scene-owner or query activation."}
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
