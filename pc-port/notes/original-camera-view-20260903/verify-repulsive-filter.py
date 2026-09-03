#!/usr/bin/env python3
"""Verify original repulsion, triangle filter, and cylinder helpers against retail."""

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
BUILD = ROOT / "build/compat-camera-repulsive-filter"
FUNCTIONS = {
    "CameraRepulsiveArea": {
        "getRepulsion__21CameraRepulsiveSphereFRCQ29JGeometry8TVec3<f>": (0x80020228, 0x14),
        "getRepulsion__23CameraRepulsiveCylinderFRCQ29JGeometry8TVec3<f>": (0x8002023C, 0xFC),
    },
    "AreaObjUtil": {
        "calcCylinderUpVec__2MRFPQ29JGeometry8TVec3<f>PC7AreaObj": (0x804008F4, 0x10),
        "getCylinderRadius__2MRFPC7AreaObj": (0x80400904, 0xC),
    },
    "TriangleFilter": {
        "createTriangleFilterFunc__2MRFPFPC8Triangle_b": (0x804081EC, 0x48),
        "isInvalidTriangle__18TriangleFilterFuncCFPC8Triangle": (0x8009DA5C, 0x14),
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
        "CameraRepulsiveArea": "src/Game/AreaObj/CameraRepulsiveArea.cpp",
        "AreaObjUtil": "src/Game/Util/AreaObjUtil.cpp",
        "TriangleFilter": "src/Game/Util/TriangleFilter.cpp",
    }.items():
        command = ["build/tools/wibo", "build/compilers/GC/3.0a3/mwcceppc.exe"]
        for flag in flags:
            command.extend(shlex.split(flag))
        command.extend(["-c", source, "-o", str(BUILD / f"{unit}.o")])
        (BUILD / f"{unit}.command.txt").write_text(shlex.join(command) + "\n")
        subprocess.run(command, cwd=ROOT, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile", action="store_true", help="Compile the full root units with the configured GC/3.0a3 Game flags")
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
    # __init_registers loads r2 with these exact lis/ori instructions.
    assert dol_bytes(dol, 0x80004224, 8) == bytes.fromhex("3c40806b6042fc20")
    sda2_base = 0x806BFC20
    float_addresses = {
        bytes.fromhex("00000000"): 0x806B7DE4,
        bytes.fromhex("3f800000"): 0x806B7DE0,
        bytes.fromhex("40000000"): 0x806B7DE8,
    }
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
                    symbol_name, value, _, symbol_section = elf.symbols[info >> 8]
                    kind = info & 0xFF
                    if kind == 10:  # R_PPC_REL24
                        target = addresses[symbol_name] + addend
                        word = struct.unpack_from(">I", code, relative)[0]
                        word = (word & 0xFC000003) | ((target - base - relative) & 0x03FFFFFC)
                        struct.pack_into(">I", code, relative, word)
                    elif kind in (4, 6):  # R_PPC_ADDR16_LO / R_PPC_ADDR16_HA
                        assert symbol_name == "__vt__18TriangleFilterFunc"
                        target = addresses[symbol_name] + addend
                        assert target == 0x80574B48
                        halfword = target if kind == 4 else (target + 0x8000) >> 16
                        struct.pack_into(">H", code, relative, halfword & 0xFFFF)
                    elif kind == 109:  # R_PPC_EMB_SDA21
                        literal = elf.section_data(symbol_section)[value + addend : value + addend + 4]
                        target = float_addresses[literal]
                        assert dol_bytes(dol, target, 4) == literal
                        displacement = target - sda2_base
                        assert -0x8000 <= displacement < 0x8000
                        word = struct.unpack_from(">I", code, relative)[0]
                        word = (word & 0xFFE00000) | (2 << 16) | (displacement & 0xFFFF)
                        struct.pack_into(">I", code, relative, word)
                    else:
                        raise AssertionError(f"Unexpected relocation {kind} in {function}")
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
