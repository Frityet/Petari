#!/usr/bin/env python3
"""Compile with GC/3.0a3 and compare signed rotation output dataflow to RMGK01."""

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
BUILD = ROOT / "build/original-camera-targets-20260903"
SYMBOL = "makeMtxRotate__2MRFPA4_fsss"
ADDRESS = 0x803EB2C4
SIZE = 0xBC
TABLE = 0x8060FC80
SDA2 = 0x806BFC20
ZERO = 0x806C17E4


def c_string(data, offset):
    return data[offset:data.index(0, offset)].decode()


class Elf:
    def __init__(self, path):
        self.data = path.read_bytes()
        assert self.data[:6] == b"\x7fELF\x01\x02"
        shoff = struct.unpack_from(">I", self.data, 0x20)[0]
        shsize, shnum = struct.unpack_from(">HH", self.data, 0x2E)
        self.sections = [struct.unpack_from(">10I", self.data, shoff + i * shsize) for i in range(shnum)]
        symbols = next(section for section in self.sections if section[1] == 2)
        names = self.section_data(symbols[6])
        self.symbols = []
        for offset in range(symbols[4], symbols[4] + symbols[5], symbols[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", self.data, offset)
            self.symbols.append((c_string(names, name), value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4] + section[5]]


def dol_bytes(dol, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from(">I", dol, field + index * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return dol[start:start + size]
    raise AssertionError(f"DOL range absent: {address:#x}")


def compile_unit():
    config = types.SimpleNamespace(version="RMGK01")
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"), {"config": config, "version_num": 0})
            break
    else:
        raise AssertionError("Configured Game flags absent")
    command = ["build/tools/wibo", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command.extend(["-c", "src/Game/Util/MtxUtil.cpp", "-o", str(BUILD / "MtxUtil.o")])
    (BUILD / "compile-command.txt").write_text(shlex.join(command) + "\n")
    (BUILD / "compile-command.json").write_text(json.dumps(command, indent=2) + "\n")
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / "compile.log").write_text(result.stdout)
    print(result.stdout, end="")
    result.check_returncode()


def relocate(elf, dol):
    _, start, size, section_index = next(symbol for symbol in elf.symbols if symbol[0] == SYMBOL)
    code = bytearray(elf.section_data(section_index)[start:start + size])
    records = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != section_index:
            continue
        for offset in range(section[4], section[4] + section[5], section[9]):
            address, info, addend = struct.unpack_from(">IIi", elf.data, offset)
            relative = address - start
            if not 0 <= relative < size:
                continue
            name, value, symbol_size, symbol_section = elf.symbols[info >> 8]
            kind = info & 0xFF
            if kind in (4, 6):  # R_PPC_ADDR16_LO / R_PPC_ADDR16_HA
                assert name == "sSinCosTable__5JMath" and addend == 0
                target = TABLE
                half = target & 0xFFFF if kind == 4 else ((target + 0x8000) >> 16) & 0xFFFF
                struct.pack_into(">H", code, relative, half)
            elif kind == 109:  # R_PPC_EMB_SDA21
                assert symbol_size == 4 and addend == 0
                constant = elf.section_data(symbol_section)[value:value + 4]
                assert constant == dol_bytes(dol, ZERO, 4) == b"\0\0\0\0"
                target = ZERO
                displacement = target - SDA2
                assert -0x8000 <= displacement < 0x8000
                word = struct.unpack_from(">I", code, relative)[0]
                word = (word & 0xFFE00000) | (2 << 16) | (displacement & 0xFFFF)
                struct.pack_into(">I", code, relative, word)
            else:
                raise AssertionError(f"Unexpected relocation {kind}: {name}")
            records.append({"offset": hex(relative), "kind": kind, "symbol": name, "target": hex(target)})
    assert len(records) == 3
    return bytes(code), records


def signed16(value):
    return (value & 0x7FFF) - (value & 0x8000)


def pointer_add(left, right):
    if isinstance(left, int) and isinstance(right, int):
        return (left + right) & 0xFFFFFFFF
    if left == TABLE and isinstance(right, tuple) and right[0] == "index":
        return ("table", right[1], 0)
    if isinstance(left, tuple) and left[0] in ("table", "stack") and isinstance(right, int):
        return (*left[:-1], left[-1] + right)
    raise AssertionError(("pointer add", left, right))


def load_float(pointer):
    if pointer == ZERO:
        return ("constant", "00000000")
    assert isinstance(pointer, tuple) and pointer[0] == "table" and pointer[2] in (0, 4), pointer
    return ("input", ("sin" if pointer[2] == 0 else "cos") + pointer[1])


def arithmetic(kind, left, right):
    # Only commute operands of one finite f32 add/multiply. Never reassociate
    # operations: every tree node retains its original rounding boundary.
    if kind in ("fmuls", "fadds"):
        left, right = sorted((left, right), key=repr)
    return (kind, left, right)


def output_expressions(code):
    gpr = {1: ("stack", 0), 2: SDA2, 3: ("output", 0),
           4: ("angle", "X"), 5: ("angle", "Y"), 6: ("angle", "Z")}
    fpr = {}
    outputs = {}
    counts = Counter()
    for position in range(0, len(code), 4):
        word = struct.unpack_from(">I", code, position)[0]
        op = word >> 26
        target, left, right, third = (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31, (word >> 6) & 31
        immediate = signed16(word)
        if op == 15:  # addis / lis
            assert left == 0
            gpr[target] = (immediate << 16) & 0xFFFFFFFF
            counts["lis"] += 1
        elif op == 14:  # addi
            gpr[target] = pointer_add(gpr[left] if left else 0, immediate)
            counts["stack_restore" if target == left == 1 else "addi"] += 1
        elif op == 21:  # Exact short-angle table index: (u16(angle) >> 2) * 8.
            assert (word & 0xFFFF) == 0x0BF8 and gpr[target][0] == "angle"
            gpr[left] = ("index", gpr[target][1])
            counts["angle_index"] += 1
        elif op == 31 and ((word >> 1) & 1023) == 266:  # add
            gpr[target] = pointer_add(gpr[left], gpr[right])
            counts["add"] += 1
        elif op == 31 and ((word >> 1) & 1023) == 535:  # lfsx
            fpr[target] = load_float(pointer_add(gpr[left], gpr[right]))
            counts["lfsx"] += 1
        elif op == 48:  # lfs
            fpr[target] = load_float(pointer_add(gpr[left] if left else 0, immediate))
            counts["lfs"] += 1
        elif op == 52:  # stfs
            assert left == 3 and immediate in range(0, 48, 4)
            assert immediate not in outputs
            outputs[immediate] = fpr[target]
            counts["stfs"] += 1
        elif op == 59:
            kind = {25: "fmuls", 21: "fadds", 20: "fsubs"}[(word >> 1) & 31]
            fpr[target] = arithmetic(kind, fpr[left], fpr[third if kind == "fmuls" else right])
            counts[kind] += 1
        elif op == 63 and ((word >> 1) & 1023) == 40:  # fneg
            fpr[target] = ("fneg", fpr[right])
            counts["fneg"] += 1
        elif word == 0x4E800020:
            assert position + 4 == len(code)
            counts["blr"] += 1
        elif op == 37:  # stwu r1,-32(r1)
            assert target == left == 1 and immediate == -32
            gpr[1] = pointer_add(gpr[1], immediate)
            counts["stack_save"] += 1
        elif op in (50, 54, 56, 60):
            # Exact compiler prologue/epilogue preserving caller f31, including
            # paired-single state. These never contribute to a matrix output.
            assert word in (0xDBE10010, 0xF3E10018, 0xE3E10018, 0xCBE10010), hex(word)
            counts["f31_preserve"] += 1
        else:
            raise AssertionError(f"Unsupported instruction at {position:#x}: {word:08x}")
    assert set(outputs) == set(range(0, 48, 4))
    return outputs, counts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile", action="store_true")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    if args.compile:
        compile_unit()
    dol = (ROOT / "build/compat-math-oracle/main.dol").read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    # Retail __init_registers establishes the original SDA2 base.
    assert dol_bytes(dol, 0x80004224, 8) == bytes.fromhex("3c40806b6042fc20")
    symbol_text = (ROOT / "config/RMGK01/symbols.txt").read_text()
    assert re.search(r"^sSinCosTable__5JMath = \.[^:]+:0x8060FC80;", symbol_text, re.MULTILINE)
    compiled, relocations = relocate(Elf(BUILD / "MtxUtil.o"), dol)
    retail = dol_bytes(dol, ADDRESS, SIZE)
    (BUILD / "compiled_signed_relocated.bin").write_bytes(compiled)
    original_outputs, original_counts = output_expressions(retail)
    compiled_outputs, compiled_counts = output_expressions(compiled)
    assert original_outputs == compiled_outputs
    assert compiled_counts - original_counts == Counter({"f31_preserve": 4, "stack_save": 1, "stack_restore": 1})
    assert not original_counts - compiled_counts
    result = {
        "function": SYMBOL,
        "compiled_size": hex(len(compiled)),
        "retail_size": hex(len(retail)),
        "compiled_instructions": len(compiled) // 4,
        "retail_instructions": len(retail) // 4,
        "relocations": relocations,
        "matching_output_dataflows": len(original_outputs),
        "compiled_instruction_counts": compiled_counts,
        "retail_instruction_counts": original_counts,
        "outputs": {f"matrix[{offset // 16}][{(offset % 16) // 4}]": value for offset, value in sorted(original_outputs.items())},
        "compiled_object_sha256": hashlib.sha256((BUILD / "MtxUtil.o").read_bytes()).hexdigest(),
        "compiler_sha256": hashlib.sha256((ROOT / "build/compilers/GC/3.0a3/mwcceppc.exe").read_bytes()).hexdigest(),
    }
    (BUILD / "rotation-verification.json").write_text(json.dumps(result, indent=2) + "\n")
    report = (
        "GC/3.0a3 signed makeMtxRotate: 53 emitted instructions vs 47 retail.\n"
        "Resolved 3 relocations (table HA/LO and verified positive-zero SDA2).\n"
        "All 12 matrix output expression trees match, preserving f32 operation grouping.\n"
        "Differences: register allocation/scheduling and 6 stack/f31 preservation instructions.\n"
        "This is functional compiler evidence; emitted instructions are not byte-identical.\n"
    )
    (BUILD / "rotation-verification.txt").write_text(report)
    print(report, end="")


if __name__ == "__main__":
    main()
