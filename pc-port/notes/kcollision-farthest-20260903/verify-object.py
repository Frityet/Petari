#!/usr/bin/env python3
"""Compare the restored function's PPC object with RMGK01 rev0, including relocations."""

import argparse
import hashlib
from pathlib import Path
import re
import struct


ROOT = Path(__file__).resolve().parents[3]
FUNCTION = "calcFarthestVertexDistance__16KCollisionServerFv"
ADDRESS = 0x80183208
R2 = 0x806BFC20


def dol_bytes(data, address, size):
    for index in range(18):
        offset = struct.unpack_from(">I", data, index * 4)[0]
        base = struct.unpack_from(">I", data, 0x48 + index * 4)[0]
        length = struct.unpack_from(">I", data, 0x90 + index * 4)[0]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start : start + size]
    raise AssertionError(f"DOL address is absent: {address:#x}")


def string_at(table, offset):
    return table[offset : table.index(0, offset)].decode()


def verify(dol_path, object_path):
    dol = dol_path.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578", "Expected RMGK01 rev0"
    elf = object_path.read_bytes()
    assert elf[:6] == b"\x7fELF\x01\x02", "Expected a big-endian ELF32 object"
    shoff = struct.unpack_from(">I", elf, 0x20)[0]
    shsize, shnum = struct.unpack_from(">HH", elf, 0x2E)
    sections = [
        struct.unpack_from(">10I", elf, shoff + index * shsize)
        for index in range(shnum)
    ]

    def section_data(index):
        section = sections[index]
        return elf[section[4] : section[4] + section[5]]

    symbols = None
    for section in sections:
        if section[1] != 2:
            continue
        names = section_data(section[6])
        symbols = []
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", elf, offset)
            symbols.append((string_at(names, name), value, size, index))
        break
    assert symbols is not None
    name, value, size, section_index = next(symbol for symbol in symbols if symbol[0] == FUNCTION)
    compiled = bytearray(section_data(section_index)[value : value + size])
    retail = dol_bytes(dol, ADDRESS, 0x188)
    assert size == 0x1B8, f"Expected only the documented sqrt inlining difference: {size:#x}"

    addresses = dict(
        (name, int(address, 16))
        for name, address in re.findall(
            r"^(.+?) = [^:]+:(0x[0-9a-fA-F]+);",
            (ROOT / "config/RMGK01/symbols.txt").read_text(),
            re.MULTILINE,
        )
    )
    relocated = 0
    for section in sections:
        if section[1] != 4 or section[7] != section_index:
            continue
        for offset in range(section[4], section[4] + section[5], section[9]):
            address, info, addend = struct.unpack_from(">IIi", elf, offset)
            relative = address - value
            if not 0 <= relative < size:
                continue
            # The inlined sqrt replaces retail offsets 0x158..0x163 with 0x158..0x193.
            if 0x158 <= relative < 0x194:
                continue
            original_offset = relative if relative < 0x158 else relative - 0x30
            symbol_name, symbol_value, _, symbol_section = symbols[info >> 8]
            kind = info & 0xFF
            word = struct.unpack_from(">I", compiled, relative)[0]
            if kind == 10:  # R_PPC_REL24
                target = addresses[symbol_name] + addend
                word = (word & 0xFC000003) | ((target - ADDRESS - original_offset) & 0x03FFFFFC)
            elif kind == 109:  # R_PPC_EMB_SDA21: the initial +0.0f constant
                assert relative == 0x24
                constant = section_data(symbol_section)[symbol_value + addend : symbol_value + addend + 4]
                assert constant == dol_bytes(dol, 0x806BC04C, 4) == b"\0\0\0\0"
                word = (word & 0xFFE00000) | (2 << 16) | ((0x806BC04C - R2) & 0xFFFF)
            else:
                raise AssertionError(f"Unexpected relocation kind {kind}")
            struct.pack_into(">I", compiled, relative, word)
            relocated += 1

    assert compiled[:0x158] == retail[:0x158], "Pre-sqrt instructions differ"
    assert compiled[0x194:] == retail[0x164:], "Post-sqrt instructions differ"
    matched = (0x158 + 0x188 - 0x164) // 4
    print(f"{matched}/98 retail instructions match exactly after {relocated} verified relocations.")
    print("The other 3 retail instructions call/store MR::sqrt; current MathUtil.hpp inlines it.")
    print("Retail size: 0x188. Current original-compiler object size: 0x1b8.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", type=Path, default=ROOT / "build/compat-math-oracle/main.dol")
    parser.add_argument("--object", type=Path, default=ROOT / "build/compat-kcollision-farthest/KCollision.o")
    args = parser.parse_args()
    verify(args.dol, args.object)
