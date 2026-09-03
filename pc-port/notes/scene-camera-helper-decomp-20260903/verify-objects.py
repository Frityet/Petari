#!/usr/bin/env python3
"""Verify six recovered/corrected functions against RMGK01 rev0 after relocation."""

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
BUILD = ROOT / "build/compat-scene-camera-helpers"
FUNCTIONS = {
    "SceneUtil": {
        "getCurrentStartZoneId__2MRFv": (0x803F75A0, 0x24),
        "getCurrentStartCameraId__2MRFv": (0x803F7A88, 0x24),
        "getStartCameraIdInfoFromStartDataIndex__2MRFP10JMapIdInfoi": (0x803F7AAC, 0x44),
        "getStageCameraData__2MRFPPvPll": (0x803F7C14, 0x8C),
    },
    "StageDataHolder": {
        "getStageDataHolderFromZoneId__15StageDataHolderFi": (0x80347B08, 0x4),
        "isPlacedZone__15StageDataHolderCFi": (0x80347B0C, 0x50),
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile", action="store_true", help="Compile both root units first with the configured GC/3.0a3 Game flags")
    args = parser.parse_args()
    if args.compile:
        config = types.SimpleNamespace(version="RMGK01")
        for node in ast.parse((ROOT / "configure.py").read_text()).body:
            if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
                flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"), {"config": config, "version_num": 0})
                break
        BUILD.mkdir(parents=True, exist_ok=True)
        for unit, source in {
            "SceneUtil": "src/Game/Util/SceneUtil.cpp",
            "StageDataHolder": "src/Game/Scene/StageDataHolder.cpp",
        }.items():
            command = ["build/tools/wibo", "build/compilers/GC/3.0a3/mwcceppc.exe"]
            for flag in flags:
                command.extend(shlex.split(flag))
            command.extend(["-c", source, "-o", str(BUILD / f"{unit}.o")])
            (BUILD / f"{unit}.command.txt").write_text(shlex.join(command) + "\n")
            subprocess.run(command, cwd=ROOT, check=True)
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
    instructions = 0
    relocation_count = 0
    for unit, functions in FUNCTIONS.items():
        elf = Elf(BUILD / f"{unit}.o")
        for function, (base, expected_size) in functions.items():
            name, start, size, section_index = next(s for s in elf.symbols if s[0] == function)
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
                        literal = c_string(elf.section_data(symbol_section), value + addend)
                        assert literal == b"CameraParam.bcam"
                        assert dol_bytes(dol, 0x805E1FCB, len(literal) + 1) == literal + b"\0"
                        target = 0x805E1FCB
                        halfword = target if kind == 4 else (target + 0x8000) >> 16
                        struct.pack_into(">H", code, relative, halfword & 0xFFFF)
                    else:
                        raise AssertionError(f"Unexpected relocation {kind} in {function}")
                    relocation_count += 1
            assert code == dol_bytes(dol, base, size), f"Instruction difference in {function}"
            instructions += size // 4
            print(f"{function}: {size // 4}/{size // 4} instructions match")
    print(f"Total: {instructions}/{instructions} instructions match after {relocation_count} verified relocations.")


if __name__ == "__main__":
    main()
