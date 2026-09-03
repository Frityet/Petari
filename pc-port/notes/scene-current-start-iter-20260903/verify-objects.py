#!/usr/bin/env python3
"""Verify the two recovered current-Mario-start helpers against RMGK01 rev0."""

import argparse
import ast
import hashlib
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types


ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/compat-scene-current-start-iter"
FUNCTIONS = {
    "StageDataHolder": {
        "makeCurrentMarioJMapInfoIter__15StageDataHolderCFv": (0x80347D84, 0xD4),
    },
    "SceneUtil": {
        "getCurrentMarioStartIdInfo__2MRFv": (0x803F756C, 0x10),
    },
}


def c_string(data, offset):
    return data[offset : data.index(0, offset)]


class Elf:
    def __init__(self, path):
        self.data = path.read_bytes()
        assert self.data[:6] == b"\x7fELF\x01\x02"
        shoff = struct.unpack_from(">I", self.data, 0x20)[0]
        shsize, shnum = struct.unpack_from(">HH", self.data, 0x2E)
        self.sections = [
            struct.unpack_from(">10I", self.data, shoff + index * shsize)
            for index in range(shnum)
        ]
        table = next(section for section in self.sections if section[1] == 2)
        names = self.section_data(table[6])
        self.symbols = []
        for offset in range(table[4], table[4] + table[5], table[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", self.data, offset)
            self.symbols.append((c_string(names, name).decode(), value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4] : section[4] + section[5]]


def dol_bytes(data, address, size):
    for index in range(18):
        offset = struct.unpack_from(">I", data, index * 4)[0]
        base = struct.unpack_from(">I", data, 0x48 + index * 4)[0]
        length = struct.unpack_from(">I", data, 0x90 + index * 4)[0]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start : start + size]
    raise AssertionError(f"DOL range not found: {address:#x}")


def compile_units():
    config = types.SimpleNamespace(version="RMGK01")
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"), {"config": config, "version_num": 0})
            break
    else:
        raise AssertionError("Configured Game compiler flags were not found")
    for unit, source in {
        "StageDataHolder": "src/Game/Scene/StageDataHolder.cpp",
        "SceneUtil": "src/Game/Util/SceneUtil.cpp",
    }.items():
        command = ["build/tools/wibo", "build/compilers/GC/3.0a3/mwcceppc.exe"]
        for flag in flags:
            command.extend(shlex.split(flag))
        command.extend(["-c", source, "-o", str(BUILD / f"{unit}.o")])
        (BUILD / f"{unit}.command.txt").write_text(shlex.join(command) + "\n")
        subprocess.run(command, cwd=ROOT, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile", action="store_true", help="Compile both full root units with the configured GC/3.0a3 Game flags")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    if args.compile:
        compile_units()
    dol = (ROOT / "build/compat-math-oracle/main.dol").read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    addresses = {
        name: int(address, 16)
        for name, address in re.findall(
            r"^(.+?) = [^:]+:(0x[0-9a-fA-F]+);",
            (ROOT / "config/RMGK01/symbols.txt").read_text(),
            re.MULTILINE,
        )
    }
    # __init_registers loads r13 with these exact lis/ori instructions.
    assert dol_bytes(dol, 0x8000422C, 8) == bytes.fromhex("3da0806b61ad9620")
    sda_base = 0x806B9620
    instructions = 0
    relocation_count = 0
    report = []
    for unit, functions in FUNCTIONS.items():
        elf = Elf(BUILD / f"{unit}.o")
        for function, (base, expected_size) in functions.items():
            name, start, size, section_index = next(s for s in elf.symbols if s[0] == function)
            assert addresses[function] == base
            assert size == expected_size, (function, size, expected_size)
            code = bytearray(elf.section_data(section_index)[start : start + size])
            for section in elf.sections:
                if section[1] != 4 or section[7] != section_index:
                    continue
                for offset in range(section[4], section[4] + section[5], section[9]):
                    address, info, addend = struct.unpack_from(">IIi", elf.data, offset)
                    relative = address - start
                    if not 0 <= relative < size:
                        continue
                    symbol_name, _, _, _ = elf.symbols[info >> 8]
                    kind = info & 0xFF
                    target = addresses[symbol_name] + addend
                    word = struct.unpack_from(">I", code, relative)[0]
                    if kind == 10:  # R_PPC_REL24
                        word = (word & 0xFC000003) | ((target - base - relative) & 0x03FFFFFC)
                    elif kind == 109:  # R_PPC_EMB_SDA21
                        assert symbol_name == "sInstance__29SingletonHolder<10GameSystem>"
                        assert target == 0x806B5BE8
                        displacement = target - sda_base
                        assert -0x8000 <= displacement < 0x8000
                        word = (word & 0xFFE00000) | (13 << 16) | (displacement & 0xFFFF)
                    else:
                        raise AssertionError(f"Unexpected relocation {kind} in {function}")
                    struct.pack_into(">I", code, relative, word)
                    relocation_count += 1
            assert code == dol_bytes(dol, base, size), f"Instruction difference in {function}"
            instructions += size // 4
            report.append(f"{function}: {size // 4}/{size // 4} instructions match")
    report.append(f"Total: {instructions}/{instructions} instructions match after {relocation_count} verified relocations.")
    result = "\n".join(report) + "\n"
    (BUILD / "verification.txt").write_text(result)
    print(result, end="")


if __name__ == "__main__":
    main()
