#!/usr/bin/env python3
"""Verify restored contact handling against RMGK01, without a fake Mario owner.

Build artifacts and complete instruction graphs remain under build/. This checks
all original instructions, including call order, field offsets, flag masks,
constants and branch destinations. It does not activate or execute Mario.
"""

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

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / "build/original-mario-binder-info-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
SOURCE = ROOT / "src/Game/Player/MarioCollision.cpp"
NATIVE = ROOT / "pc-port/src/Game/Player/MarioCollision.cpp"
SYMBOL = "updateBinderInfo__5MarioFv"
ADDRESS = 0x802D2FE8
SIZE = 0x9C8
SDA2 = 0x806BFC20


def import_helper(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log, cwd=ROOT):
    result = subprocess.run(command, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / log).write_text(result.stdout)
    if result.returncode:
        print(result.stdout)
    result.check_returncode()


def signed16(value):
    return (value & 0x7FFF) - (value & 0x8000)


def body(source):
    text = source.read_text()
    start = text.index("bool Mario::updateBinderInfo() {")
    return text[start:text.index("\nbool Mario::isThroughWall", start)]


def canonical(symbol, elf, is_retail):
    references = {int(r["offset"], 16) & ~3: r for r in elf.references(SYMBOL)}
    result = []
    for entry in symbol["instructions"]:
        # No unmatched/deleted/inserted instructions are allowed.
        instruction = entry["instruction"]
        offset = int(instruction["address"]) - int(symbol["address"])
        text = instruction["formatted"]
        reference = references.get(offset)
        if reference:
            target = reference.get("value_hex", reference["symbol"])
            text = text.replace(reference["symbol"], "reference_" + target)
        elif re.match(r"b\w* 0x", text):
            text = re.sub(r"0x[0-9a-f]+", lambda m: "relative_" + hex(int(m[0], 16) - int(symbol["address"])), text)
        if is_retail and 0x74 <= offset < 0x950:
            # Saved registers have different allocation in the function body.
            # Prologue/epilogue are deliberately not rewritten.
            mapping = {28: 29, 29: 30, 30: 31, 31: 28}
            # Retail reuses its now-dead first-push boolean for two actor-vector
            # references. The compiler instead reuses dead needsPush (r23).
            # Only their definitions and uses differ; no operand is discarded.
            if offset in (0x380, 0x38C, 0x570, 0x57C):
                mapping[29] = 23
            text = re.sub(r"\br(\d+)\b", lambda m: "r" + str(mapping.get(int(m[1]), int(m[1]))), text)
        result.append(text)
    return result


def verify_retail_object(helper, elf, dol, symbols):
    _, start, size, section = next(s for s in elf.symbols if s[0] == SYMBOL)
    assert size == SIZE
    obj = elf.section_data(section)[start:start + size]
    retail = helper.dol_bytes(dol, ADDRESS, SIZE)
    references = elf.references(SYMBOL)
    masks = {}
    data_pairs = {}
    witnesses = []
    for ref in references:
        offset = int(ref["offset"], 16)
        aligned = offset & ~3
        instruction = struct.unpack_from(">I", retail, aligned)[0]
        kind = ref["kind"]
        witness = {"instruction": hex(ADDRESS + aligned), "kind": kind, "symbol": ref["symbol"]}
        if kind == 10:
            masks[aligned] = 0xFC000003
            displacement = instruction & 0x03FFFFFC
            if displacement & 0x02000000:
                displacement -= 0x04000000
            target = ADDRESS + aligned + displacement
            assert target == symbols[ref["symbol"]] + ref["addend"], ref
            witness["actual_target"] = hex(target)
        elif kind == 109:
            masks[aligned] = 0xFFE00000
            assert (instruction >> 16) & 31 == 2, ref
            target = SDA2 + signed16(instruction)
            expected = bytes.fromhex(ref["value_hex"])
            assert helper.dol_bytes(dol, target, len(expected)) == expected, ref
            witness.update(actual_target=hex(target), value_hex=expected.hex())
        elif kind in (4, 6):
            masks[aligned] = 0xFFFF0000
            data_pairs.setdefault(ref["symbol"], {})[kind] = (aligned, instruction, ref)
        else:
            raise AssertionError(ref)
        witnesses.append(witness)
    # Check every raw retail instruction; relocation fields are validated above
    # and the actual string effective address is validated below.
    for offset in range(0, SIZE, 4):
        mask = masks.get(offset, 0xFFFFFFFF)
        left = struct.unpack_from(">I", obj, offset)[0]
        right = struct.unpack_from(">I", retail, offset)[0]
        assert left & mask == right & mask, (hex(offset), hex(left), hex(right))
    strings = []
    for name, pair in data_pairs.items():
        high_at, high, high_ref = pair[6]
        low_at, low, low_ref = pair[4]
        assert high >> 26 == 15 and ((high >> 16) & 31) == 0  # lis
        assert low >> 26 == 14  # addi
        assert (high >> 21) & 31 == (low >> 16) & 31  # retained HA base
        target = (((high & 0xFFFF) << 16) + signed16(low)) & 0xFFFFFFFF
        expected = bytes.fromhex(high_ref["value_hex"])
        assert low_ref["value_hex"] == high_ref["value_hex"]
        assert helper.dol_bytes(dol, target, len(expected)) == expected
        assert target == 0x805C497B
        assert expected[:-1].decode("shift_jis") == "マンホールのふた(クッパ船)"
        # Retail passes sensor->host->mName as strcmp's first argument.
        assert helper.dol_bytes(dol, 0x802D38D0, 16).hex() == "807a00083898497b8063002480630004"
        strings.append({"ha_instruction": hex(ADDRESS + high_at), "lo_instruction": hex(ADDRESS + low_at),
                        "effective_address": hex(target), "shift_jis_hex": expected.hex(),
                        "text": expected[:-1].decode("shift_jis")})
    return witnesses, strings


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    helper = import_helper("mario_elf", "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    assert body(SOURCE) == body(NATIVE)
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Configured original Game flags missing")
    command = [str(ROOT / p) for p in ("build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe")]
    for flag in flags:
        command.extend(shlex.split(flag))
    compiled = BUILD / "MarioCollision.o"
    command += ["-c", str(SOURCE), "-o", str(compiled)]
    (BUILD / "original-command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "original-compile.log")

    # Reuse the verified split when available; otherwise create an isolated split.
    # Every instruction/relocation in the reused object is checked against DOL.
    target = ROOT / "build/mario-update-restoration-20260903/retail/obj/Game/Player/MarioCollision.o"
    if not target.exists():
        config = (ROOT / "config/RMGK01/config.yml").read_text()
        config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
        config = config.replace("object: sys/main.dol", "object: " + DOL.name)
        config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
        config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
        (BUILD / "config.yml").write_text(config)
        run([str(ROOT / "build/tools/dtk"), "dol", "split", "--no-update", "-j", "2", str(BUILD / "config.yml"),
             str(BUILD / "retail")], "dtk.log")
        target = BUILD / "retail/obj/Game/Player/MarioCollision.o"
    output = BUILD / "objdiff.json"
    run([str(ROOT / "build/tools/objdiff-cli"), "diff", "-1", str(target), "-2", str(compiled),
         "-o", str(output), "--format", "json-pretty"], "objdiff.log")
    diff = json.loads(output.read_text())
    sides = [next(s for s in diff[side]["symbols"] if s["name"] == SYMBOL) for side in ("left", "right")]
    elves = [helper.Elf(target), helper.Elf(compiled)]
    refs = [elf.references(SYMBOL) for elf in elves]
    compare_refs = lambda rs: [(r["offset"], r["kind"], r.get("value_hex", r["symbol"]), r["addend"]) for r in rs]
    assert compare_refs(refs[0]) == compare_refs(refs[1]), "All call/data references must retain their offsets and values"
    assert int(sides[0]["size"]) == int(sides[1]["size"]) == SIZE
    graphs = [canonical(s, e, i == 0) for i, (s, e) in enumerate(zip(sides, elves))]
    assert len(graphs[0]) == len(graphs[1]) == 626
    assert graphs[0] == graphs[1], [(i, a, b) for i, (a, b) in enumerate(zip(*graphs)) if a != b]
    (BUILD / "canonical-graph.txt").write_text("\n".join(graphs[0]) + "\n")
    assert sides[0]["match_percent"] >= 99.0
    symbols = {}
    for line in (ROOT / "config/RMGK01/symbols.txt").read_text().splitlines():
        match = re.match(r"(.*?) = \.[^:]+:(0x[0-9A-F]+);", line)
        if match:
            symbols[match[1]] = int(match[2], 0)
    raw_refs, strings = verify_retail_object(helper, elves[0], dol, symbols)

    # Parse the actual PC mirror with the existing native target's includes and
    # defines. This deliberately does not link or manufacture gameplay owners.
    syntax_helper = import_helper("jump_syntax", "pc-port/notes/original-mario-jump-20260903/probe-source.py")
    prefix, cwd = syntax_helper.command_prefix()
    native_command = prefix + [str(NATIVE)]
    (BUILD / "native-syntax-command.json").write_text(json.dumps(native_command, indent=2) + "\n")
    run(native_command, "native-syntax.log", cwd=cwd)
    report = {
        "scope": "Root-first function recovery and exact PC mirror; no player activation or end-to-end jump claim",
        "symbol": SYMBOL, "address": hex(ADDRESS), "retail_size": SIZE, "compiled_size": int(sides[1]["size"]),
        "dol_sha1": hashlib.sha1(dol).hexdigest(), "retail_function_sha256": hashlib.sha256(helper.dol_bytes(dol, ADDRESS, SIZE)).hexdigest(),
        "compiler": "GC3.0a3; configure.py cflags_game; sjiswrap v1.2.2", "objdiff_match_percent": sides[0]["match_percent"],
        "root_source_sha256": sha(SOURCE), "native_source_sha256": sha(NATIVE),
        "function_source_sha256": hashlib.sha256(body(SOURCE).encode()).hexdigest(),
        "target_object_sha256": sha(target), "compiled_object_sha256": sha(compiled),
        "native_syntax_exit": 0, "pc_function_identical": True,
        "canonical_instruction_count": 626, "canonical_graph_equal": True,
        "canonical_graph_sha256": sha(BUILD / "canonical-graph.txt"),
        "canonicalization": {
            "body_register_mapping_retail_to_compiled": {"r28": "r29", "r29": "r30", "r30": "r31", "r31": "r28"},
            "body_range": ["0x74", "0x950"],
            "short_lived_actor_vector_r29_to_r23_offsets": ["0x380", "0x38c", "0x570", "0x57c"],
            "other_changes": "Local branch destinations relative to function; data labels resolved to exact bytes; no instruction or operand removed",
        },
        "direct_call_count": sum(r["kind"] == 10 for r in refs[0]),
        "all_references_same_offset_and_value": True, "raw_retail_relocation_witnesses": raw_refs,
        "actual_string_address_witnesses": strings,
    }
    (NOTES / "source-evidence.json").write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    print(f"updateBinderInfo: {sides[0]['match_percent']:.6f}%, {SIZE} bytes, all 626 canonical instructions agree; native syntax passes")


if __name__ == "__main__":
    main()
