#!/usr/bin/env python3
"""Compile root MarioActorGravity and retain reproducible retail comparison inputs."""

import argparse
import ast
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/mario-gravity-restoration-20260903"
SOURCE = ROOT / "src/Game/Player/MarioActorGravity.cpp"
ADDRESS = 0x802B9A9C
SIZE = 0x930
SYMBOL = "updateGravityVec__10MarioActorFbb"
SDA2 = 0x806BFC20
LLVM = Path("/opt/homebrew/opt/llvm/bin")


class Elf:
    def __init__(self, path):
        self.data = path.read_bytes()
        assert self.data[:6] == b"\x7fELF\x01\x02"
        offset = struct.unpack_from(">I", self.data, 0x20)[0]
        size, count = struct.unpack_from(">HH", self.data, 0x2E)
        self.sections = [struct.unpack_from(">10I", self.data, offset + i * size) for i in range(count)]
        section = next(s for s in self.sections if s[1] == 2)
        names = self.section_data(section[6])
        self.symbols = []
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", self.data, offset)
            self.symbols.append((names[name:names.index(0, name)].decode(), value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4] + section[5]]

    def references(self, function):
        _, start, size, index = next(s for s in self.symbols if s[0] == function)
        result = []
        for section in self.sections:
            if section[1] != 4 or section[7] != index:
                continue
            for offset in range(section[4], section[4] + section[5], section[9]):
                at, info, addend = struct.unpack_from(">IIi", self.data, offset)
                if not start <= at < start + size:
                    continue
                name, value, length, target_section = self.symbols[info >> 8]
                kind = info & 255
                record = {"offset": hex(at - start), "kind": kind, "symbol": name, "addend": addend}
                if kind != 10 and target_section and length:
                    payload = self.section_data(target_section)[value + addend:value + length]
                    if length not in (4, 8):
                        payload = payload[:payload.index(0) + 1]
                    record["value_hex"] = payload.hex()
                result.append(record)
        return result


def dol_bytes(data, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from(">I", data, field + index * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start:start + size]
    raise AssertionError(f"DOL range absent: {address:#x}+{size:#x}")


def compile_unit():
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
    command.extend(["-c", str(SOURCE), "-o", str(BUILD / "MarioActorGravity.o")])
    (BUILD / "compile-command.json").write_text(json.dumps(command, indent=2) + "\n")
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / "compile.log").write_text(result.stdout)
    print(result.stdout, end="")
    result.check_returncode()


def disassemble(data, stem, address):
    binary = BUILD / f"{stem}.bin"
    obj = BUILD / f"{stem}.o"
    binary.write_bytes(data)
    subprocess.run([str(LLVM / "llvm-objcopy"), "-I", "binary", "-O", "elf32-powerpc", str(binary), str(obj)], check=True)
    output = subprocess.check_output([str(LLVM / "llvm-objdump"), "-D", "--triple=powerpc",
                                      f"--adjust-vma={address:#x}", "--section=.data", str(obj)], text=True)
    return re.sub(r" <[^>]+>", "", output)


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def control_graph(data, names):
    words = list(struct.unpack(">" + str(len(data) // 4) + "I", data))
    calls = {}
    for index, word in enumerate(words):
        if word >> 26 == 18 and word & 1:
            target = ADDRESS + index * 4 + signed(word & 0x3FFFFFC, 26)
            calls[index] = names[target]
    axis_call = next(i for i, name in calls.items() if name.startswith("makeAxisAndCosignVecToVec"))
    rotate_call = next(i for i, name in calls.items() if name == "PSMTXRotAxisRad")
    # In this recovered TU the compiler expands JMAAcosRadian. Preserve the
    # preceding makeAxis result test and the following rotation call, and
    # compare the intervening math at its actual original library boundary.
    acos_begin = next(i for i in range(axis_call + 1, rotate_call) if words[i] >> 26 == 16) + 1
    events = []
    vector_intrinsics = ("_save", "_rest", "__as__", "__ct__", "__mi__", "__apl__", "__amu__", "scale__")
    for index, word in enumerate(words):
        if acos_begin <= index < rotate_call:
            if index == acos_begin:
                events.append((index, ("call", "JMAAcosRadian__Ff")))
            continue
        opcode = word >> 26
        if opcode == 18 and word & 1:
            name = calls[index]
            if not name.startswith(vector_intrinsics):
                events.append((index, ("call", name)))
        elif opcode == 16:
            target = index + signed(word & 0xFFFC, 16) // 4
            events.append((index, ("conditional_branch", (word >> 21) & 31, (word >> 16) & 31, target)))
        elif opcode == 18:
            events.append((index, ("branch", index + signed(word & 0x3FFFFFC, 26) // 4)))
        elif opcode in (10, 11):
            events.append((index, ("integer_compare", hex(word))))
        elif opcode == 63 and (word >> 1) & 1023 == 32:
            events.append((index, ("float_compare", hex(word))))
        elif opcode == 19 and (word >> 1) & 1023 == 449:
            events.append((index, ("condition_or", hex(word))))
        elif opcode == 21 and word & 1:
            events.append((index, ("bit_mask_test", hex(word))))
    def next_event(target):
        return next((i for i, (instruction, _) in enumerate(events) if instruction >= target), len(events))
    graph = []
    for _, event in events:
        if event[0] in ("conditional_branch", "branch"):
            event = (*event[:-1], next_event(event[-1]))
        graph.append(event)
    return graph, {"begin": hex(ADDRESS + acos_begin * 4), "end": hex(ADDRESS + rotate_call * 4)}


def verify_animation_argument(dol):
    high_address, low_address = 0x802B9D24, 0x802B9D2C
    high, low = [struct.unpack(">I", dol_bytes(dol, at, 4))[0] for at in (high_address, low_address)]
    assert high >> 26 == 15 and (high >> 21) & 31 == 4 and (high >> 16) & 31 == 0
    assert low >> 26 == 14 and (low >> 21) & 31 == 4 and (low >> 16) & 31 == 4
    effective = (((high & 0xFFFF) << 16) + signed(low & 0xFFFF, 16)) & 0xFFFFFFFF
    assert effective == 0x805B86D8
    expected = "ショートジャンプ".encode("shift_jis") + b"\0"
    assert dol_bytes(dol, effective, len(expected)) == expected
    return {"lis_address": hex(high_address), "lis_word": hex(high), "addi_address": hex(low_address),
            "addi_word": hex(low), "effective_address": hex(effective), "shift_jis_hex": expected.hex(),
            "text": "ショートジャンプ", "argument_register": "r4"}


def verify_ratio_writers(dol, names):
    assert dol_bytes(dol, 0x802B9670, 8) == bytes.fromhex("c02303744e800020")
    records = []
    for load_address, store_address, base_register, expected_bits in (
        (0x802AF4E0, 0x802AF4F0, 29, 0x3F800000),
        (0x802BB1E4, 0x802BB1F4, 31, 0x00000000),
    ):
        load, store = [struct.unpack(">I", dol_bytes(dol, at, 4))[0] for at in (load_address, store_address)]
        assert load >> 26 == 48 and (load >> 21) & 31 == 0 and (load >> 16) & 31 == 2
        assert store >> 26 == 52 and (store >> 21) & 31 == 0 and (store >> 16) & 31 == base_register
        assert store & 0xFFFF == 0x374
        constant_address = SDA2 + signed(load & 0xFFFF, 16)
        bits = struct.unpack(">I", dol_bytes(dol, constant_address, 4))[0]
        assert bits == expected_bits
        records.append({"load_address": hex(load_address), "load_word": hex(load), "store_address": hex(store_address),
                        "store_word": hex(store), "actor_register": f"r{base_register}", "field_offset": "0x374",
                        "constant_address": hex(constant_address), "constant_bits": hex(bits),
                        "constant_value": struct.unpack(">f", struct.pack(">I", bits))[0]})
    # Bounded direct-store inventory, not a claim about indirect/indexed writes.
    import bisect
    addresses = sorted(names)
    stores = []
    for section in range(7):
        offset, base, size = [struct.unpack_from(">I", dol, field + section * 4)[0] for field in (0, 0x48, 0x90)]
        for relative in range(0, size, 4):
            word = struct.unpack_from(">I", dol, offset + relative)[0]
            if word >> 26 != 52 or word & 0xFFFF != 0x374:
                continue
            address = base + relative
            function = names[addresses[bisect.bisect_right(addresses, address) - 1]]
            if "Mario" in function:
                stores.append({"address": hex(address), "word": hex(word), "containing_symbol": function})
    assert [int(record["address"], 16) for record in stores] == [0x802AF4F0, 0x802BB1F4]
    return {"getter_address": "0x802b9670", "getter_words": "c0230374 4e800020", "writers": records,
            "mario_named_direct_stfs_inventory": stores}


def compare_objdiff(compiled_elf, dol):
    target = BUILD / "retail/obj/Game/Player/MarioActorGravity.o"
    if not target.exists():
        config = (ROOT / "config/RMGK01/config.yml").read_text()
        dol = ROOT / "build/compat-math-oracle/main.dol"
        config = config.replace("object_base: orig/RMGK01", "object_base: " + str(dol.parent))
        config = config.replace("object: sys/main.dol", "object: " + dol.name)
        config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
        config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
        (BUILD / "config.yml").write_text(config)
        subprocess.run([str(ROOT / "build/tools/dtk"), "dol", "split", "--no-update", "-j", "2", str(BUILD / "config.yml"),
                        str(BUILD / "retail")], cwd=ROOT, check=True, stdout=(BUILD / "dtk.log").open("w"),
                       stderr=subprocess.STDOUT)
    subprocess.run([str(ROOT / "build/tools/objdiff-cli"), "diff", "-1", str(target), "-2", str(BUILD / "MarioActorGravity.o"),
                    "-o", str(BUILD / "objdiff.json"), "--format", "json-pretty"], cwd=ROOT, check=True,
                   stdout=(BUILD / "objdiff.log").open("w"), stderr=subprocess.STDOUT)
    diff = json.loads((BUILD / "objdiff.json").read_text())
    sides = [next(s for s in diff[side]["symbols"] if s["name"] == SYMBOL) for side in ("left", "right")]
    references = [elf.references(SYMBOL) for elf in (Elf(target), compiled_elf)]
    # This conversion constant belongs to an external DOL data split, so
    # the target ELF intentionally has no local payload for its label.
    for reference in references[0]:
        if reference["symbol"] == "lbl_80539A60":
            reference["value_hex"] = dol_bytes(dol, 0x80539A60, 8).hex()
    values = [{r["value_hex"] for r in side if "value_hex" in r} for side in references]
    assert values[0] <= values[1], (values[0] - values[1], "A retail constant or string is missing")
    extra = values[1] - values[0]
    assert extra == {"bf800000", "40490fdb", "447fe000", "3fc90fdb"}, extra
    aligned = []
    for left, right in zip(sides[0]["instructions"], sides[1]["instructions"]):
        def text(entry):
            instruction = entry.get("instruction")
            return "" if instruction is None else str(instruction["address"]) + " " + instruction["formatted"]
        aligned.append((str(left.get("diff_kind", "")) + " " + text(left)).ljust(105) + text(right))
    (BUILD / "aligned-instructions.txt").write_text("\n".join(aligned) + "\n")
    return {"match_percent": sides[0]["match_percent"], "retail_size": sides[0]["size"], "compiled_size": sides[1]["size"],
            "retail_object_sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
            "retail_constant_string_values": sorted(values[0]), "additional_inline_acos_constants": sorted(extra)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile", action="store_true")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    if args.compile:
        compile_unit()
    dol = (ROOT / "build/compat-math-oracle/main.dol").read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    assert dol_bytes(dol, 0x80004224, 8) == bytes.fromhex("3c40806b6042fc20")
    symbols = {}
    for line in (ROOT / "config/RMGK01/symbols.txt").read_text().splitlines():
        match = re.match(r"(.*?) = \.[^:]+:(0x[0-9A-F]+);", line)
        if match:
            symbols[match[1]] = int(match[2], 0)
    names = {address: name for name, address in symbols.items()}
    # Prefer the function's own constant pool, then verified equivalent
    # constants in the existing SDA2 section for inlined library math.
    constants = {}
    for address in range(SDA2 - 0x8000, SDA2 + 0x8000, 4):
        try:
            value = dol_bytes(dol, address, 4)
        except AssertionError:
            continue
        constants.setdefault(value, address)
    for address in range(0x806BF698, 0x806BF6E0, 4):
        constants[dol_bytes(dol, address, 4)] = address
    constants[bytes.fromhex("4330000000000000")] = 0x80539A60
    animation_argument = verify_animation_argument(dol)
    constants[bytes.fromhex(animation_argument["shift_jis_hex"])] = int(animation_argument["effective_address"], 16)
    elf = Elf(BUILD / "MarioActorGravity.o")
    symbol = next(s for s in elf.symbols if s[0] == SYMBOL)
    _, start, size, index = symbol
    compiled = bytearray(elf.section_data(index)[start:start + size])
    relocations = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != index:
            continue
        for location in range(section[4], section[4] + section[5], section[9]):
            offset, info, addend = struct.unpack_from(">IIi", elf.data, location)
            relative = offset - start
            if not 0 <= relative < size:
                continue
            name, value, length, target_section = elf.symbols[info >> 8]
            kind = info & 255
            record = {"offset": hex(relative), "kind": kind, "name": name, "addend": addend}
            if target_section:
                local = elf.section_data(target_section)[value:value + length]
                record["local_bytes"] = local.hex()
            if target_section and name.startswith("@"):
                assert addend == 0
                target = constants[local]
                assert dol_bytes(dol, target, len(local)) == local
            elif name in symbols:
                target = symbols[name] + addend
            else:
                raise AssertionError((name, kind, addend))
            record["target"] = hex(target)
            if kind == 10:
                word = struct.unpack_from(">I", compiled, relative)[0]
                displacement = target - ADDRESS - relative
                word = (word & 0xFC000003) | (displacement & 0x03FFFFFC)
                struct.pack_into(">I", compiled, relative, word)
            elif kind in (4, 6):
                immediate = target if kind == 4 else (target + 0x8000) >> 16
                struct.pack_into(">H", compiled, relative, immediate & 0xFFFF)
            elif kind == 109:
                word = struct.unpack_from(">I", compiled, relative)[0]
                displacement = target - SDA2
                assert -0x8000 <= displacement < 0x8000, (name, hex(target), kind, record)
                word = (word & 0xFFE00000) | (2 << 16) | (displacement & 0xFFFF)
                struct.pack_into(">I", compiled, relative, word)
            else:
                raise AssertionError((name, kind, addend))
            relocations.append(record)
    retail = dol_bytes(dol, ADDRESS, SIZE)
    asm = disassemble(retail, "retail", ADDRESS)
    asm = re.sub(r"\bbl (0x[0-9a-f]+)", lambda m: m[0] + " // " + names.get(int(m[1], 16), "?"), asm)
    def constant(m):
        address = SDA2 + int(m[1])
        data = dol_bytes(dol, address, 4)
        return m[0] + f" // {address:#x} = {struct.unpack('>f', data)[0]!r} ({data.hex()})"
    asm = re.sub(r"lfs \d+, (-?\d+)\(2\)", constant, asm)
    (BUILD / "retail.annotated.asm").write_text(asm)
    (BUILD / "compiled.asm").write_text(disassemble(compiled, "compiled", ADDRESS))
    subprocess.run([str(LLVM / "llvm-objdump"), "-dr", str(BUILD / "MarioActorGravity.o")], check=True,
                   stdout=(BUILD / "object.asm").open("w"))
    retail_graph, retail_acos = control_graph(retail, names)
    compiled_graph, compiled_acos = control_graph(compiled, names)
    assert retail_graph == compiled_graph, "Critical control-flow graph differs"
    graph_data = json.dumps(retail_graph, indent=2) + "\n"
    (BUILD / "critical-control-graph.json").write_text(graph_data)
    objdiff = compare_objdiff(elf, dol)
    evidence = {
        "scope": "Root-only nonmatching functional recovery; compiler, control-flow, constants and source review, not native gameplay execution",
        "symbol": SYMBOL, "retail_address": hex(ADDRESS), "retail_size": hex(SIZE), "compiled_size": hex(size),
        "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        "header_sha256": {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                          for path in (ROOT / "include/Game/Player/MarioActor.hpp", ROOT / "include/Game/Player/Mario.hpp")},
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "compiler": "GC/3.0a3, configure.py cflags_game, RMGK01 / VERSION=0",
        "tools_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
                         for path in ("build/compilers/GC/3.0a3/mwcceppc.exe", "build/tools/sjiswrap.exe",
                                      "build/tools/dtk", "build/tools/objdiff-cli")},
        "compiler_input_encoding": "Configured sjiswrap v1.2.2, compiling the real root source and include hierarchy",
        "object_sha256": hashlib.sha256(elf.data).hexdigest(),
        "retail_function_sha256": hashlib.sha256(retail).hexdigest(), "relocations": relocations,
        "objdiff": objdiff,
        "critical_control_graph": {"equal": True, "event_count": len(retail_graph),
                                   "event_types": dict(Counter(event[0] for event in retail_graph)),
                                   "graph_sha256": hashlib.sha256(graph_data.encode()).hexdigest(),
                                   "retail_acos_boundary": retail_acos, "compiled_inline_acos_boundary": compiled_acos},
        "animation_argument": animation_argument,
        "gravity_ratio_writers": verify_ratio_writers(dol, names),
    }
    (BUILD / "comparison-inputs.json").write_text(json.dumps(evidence, indent=2) + "\n")
    compact = dict(evidence)
    compact["relocation_count"] = len(compact.pop("relocations"))
    Path(__file__).with_name("compiler-evidence.json").write_text(json.dumps(compact, indent=2, ensure_ascii=False) + "\n")
    print(f"Compiled {SYMBOL}: {size:#x} bytes; retail {SIZE:#x}; {len(relocations)} relocations retained.")
    print(f"objdiff {objdiff['match_percent']:.6f}%; all {len(retail_graph)} critical control-flow events agree.")
    print("Actual animation HA/LO argument and gravity-ratio SDA load/store pairs verified.")


if __name__ == "__main__":
    main()
