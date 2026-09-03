#!/usr/bin/env python3
"""Compile unchanged root OnlyCamera and compare every retail method and vtable."""

import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types


ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/original-only-camera-20260903"
SDA2 = 0x806BFC20
CONSTANTS = {
    "3f800000": 0x806B9BD0,
    "00000000": 0x806B9BD4,
    "3f000000": 0x806B9BD8,
    "bf800000": 0x806B9BDC,
    "43960000": 0x806B9BE0,
    "3a83126f": 0x806B9BE4,
    "3f7ae148": 0x806B9BE8,
    "42c80000": 0x806B9BEC,
    "4330000080000000": 0x80531990,
}


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
    command = ["build/tools/wibo", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command.extend(["-c", "src/Game/Camera/OnlyCamera.cpp", "-o", str(BUILD / "OnlyCamera.o")])
    (BUILD / "OnlyCamera.command.txt").write_text(shlex.join(command) + "\n")
    (BUILD / "OnlyCamera.command.json").write_text(json.dumps(command, indent=2) + "\n")
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / "OnlyCamera.compile.log").write_text(result.stdout)
    print(result.stdout, end="")
    result.check_returncode()


def relocate(elf, symbol, address, retail_symbols, dol):
    name, start, size, index = symbol
    data = bytearray(elf.section_data(index)[start:start + size])
    records = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != index:
            continue
        for offset in range(section[4], section[4] + section[5], section[9]):
            location, info, addend = struct.unpack_from(">IIi", elf.data, offset)
            relative = location - start
            if not 0 <= relative < size:
                continue
            target_name, value, target_size, target_section = elf.symbols[info >> 8]
            kind = info & 255
            assert addend == 0, (name, target_name, addend)
            if target_name.startswith("@"):
                assert target_size in (4, 8)
                constant = elf.section_data(target_section)[value:value + target_size]
                target = CONSTANTS[constant.hex()]
                assert dol_bytes(dol, target, target_size) == constant
            else:
                target = retail_symbols[target_name][0]
            if kind == 1:  # R_PPC_ADDR32, vtable entry.
                struct.pack_into(">I", data, relative, target)
            elif kind in (4, 6):  # R_PPC_ADDR16_LO / R_PPC_ADDR16_HA.
                immediate = target if kind == 4 else (target + 0x8000) >> 16
                struct.pack_into(">H", data, relative, immediate & 0xFFFF)
            elif kind == 10:  # R_PPC_REL24, actual retail function or thunk.
                word = struct.unpack_from(">I", data, relative)[0]
                displacement = target - address - relative
                assert -0x02000000 <= displacement < 0x02000000 and displacement % 4 == 0
                word = (word & 0xFC000003) | (displacement & 0x03FFFFFC)
                struct.pack_into(">I", data, relative, word)
            elif kind == 109:  # R_PPC_EMB_SDA21, actual verified float bits.
                word = struct.unpack_from(">I", data, relative)[0]
                displacement = target - SDA2
                assert -0x8000 <= displacement < 0x8000
                word = (word & 0xFFE00000) | (2 << 16) | (displacement & 0xFFFF)
                struct.pack_into(">I", data, relative, word)
            else:
                raise AssertionError((name, kind, target_name))
            records.append({"offset": hex(relative), "kind": kind, "symbol": target_name, "target": hex(target)})
    return bytes(data), records


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
    retail_symbols = {}
    for line in (ROOT / "config/RMGK01/symbols.txt").read_text().splitlines():
        match = re.match(r"(.*?) = \.[^:]+:(0x[0-9A-F]+);(.*)", line)
        if match:
            name, address, metadata = match.groups()
            size = re.search(r"size:(0x[0-9A-F]+)", metadata)
            retail_symbols[name] = (int(address, 0), int(size[1], 0) if size else None)
    elf = Elf(BUILD / "OnlyCamera.o")
    methods = []
    for symbol in elf.symbols:
        name = symbol[0]
        if "10OnlyCamera" not in name or name.startswith("."):
            continue
        address, size = retail_symbols[name]
        assert symbol[2] == size, (name, symbol[2], size)
        compiled, relocations = relocate(elf, symbol, address, retail_symbols, dol)
        retail = dol_bytes(dol, address, size)
        mismatches = [hex(address + offset) for offset in range(0, size, 4)
                      if compiled[offset:offset + 4] != retail[offset:offset + 4]]
        assert compiled == retail, (name, mismatches)
        kind = "vtable" if name.startswith("__vt__") else "function"
        (BUILD / f"{name}.relocated.bin").write_bytes(compiled)
        record = {"name": name, "kind": kind, "address": hex(address), "size": hex(size),
                  "sha256": hashlib.sha256(retail).hexdigest(), "relocations": relocations,
                  "relocated_bytes_equal": True}
        methods.append(record)
        print(f"{name}: {size // 4} matching {'words' if kind == 'vtable' else 'instructions'}, "
              f"{len(relocations)} verified relocations")
    assert len(methods) == 7
    result = {
        "verification": "All six retail methods (including generated destructor) and the full vtable are byte-identical after relocation.",
        "source": "src/Game/Camera/OnlyCamera.cpp",
        "source_sha256": hashlib.sha256((ROOT / "src/Game/Camera/OnlyCamera.cpp").read_bytes()).hexdigest(),
        "header": "include/Game/Camera/OnlyCamera.hpp",
        "header_sha256": hashlib.sha256((ROOT / "include/Game/Camera/OnlyCamera.hpp").read_bytes()).hexdigest(),
        "compiler": "GC/3.0a3, configure.py cflags_game, RMGK01, VERSION=0",
        "compiler_sha256": hashlib.sha256((ROOT / "build/compilers/GC/3.0a3/mwcceppc.exe").read_bytes()).hexdigest(),
        "object_sha256": hashlib.sha256((BUILD / "OnlyCamera.o").read_bytes()).hexdigest(),
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "functions_and_vtable": methods,
    }
    (BUILD / "compiler-evidence.json").write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
