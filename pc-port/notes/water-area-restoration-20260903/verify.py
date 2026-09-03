#!/usr/bin/env python3
"""Rebuild the root water query and verify every instruction against RMGK01."""

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
BUILD = ROOT / "build/water-area-restoration-20260903"
SYMBOL = "getWaterAreaObj__2MRFP9WaterInfoRCQ29JGeometry8TVec3<f>"
ADDRESS = 0x80400964
SIZE = 0x70


def main():
    spec = importlib.util.spec_from_file_location(
        "only_camera_oracle", ROOT / "pc-port/notes/original-only-camera-20260903/verify-object.py")
    oracle = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(oracle)
    BUILD.mkdir(parents=True, exist_ok=True)
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    command = ["build/tools/wibo", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    command += [part for flag in flags for part in shlex.split(flag)]
    command += ["-c", "src/Game/Util/AreaObjUtil.cpp", "-o", str(BUILD / "AreaObjUtil.o")]
    subprocess.run(command, cwd=ROOT, check=True)
    dol = (ROOT / "build/compat-math-oracle/main.dol").read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    symbols = {}
    for line in (ROOT / "config/RMGK01/symbols.txt").read_text().splitlines():
        match = re.match(r"(.*?) = \.[^:]+:(0x[0-9A-F]+);", line)
        if match:
            symbols[match[1]] = int(match[2], 0)
    elf = oracle.Elf(BUILD / "AreaObjUtil.o")
    _, start, size, index = next(s for s in elf.symbols if s[0] == SYMBOL)
    assert size == SIZE
    result = bytearray(elf.section_data(index)[start:start + size])
    relocations = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != index:
            continue
        for offset in range(section[4], section[4] + section[5], section[9]):
            location, info, addend = struct.unpack_from(">IIi", elf.data, offset)
            relative = location - start
            if not 0 <= relative < size:
                continue
            name, value, target_size, target_section = elf.symbols[info >> 8]
            kind = info & 255
            assert addend == 0
            if name.startswith("@"):
                constant = elf.section_data(target_section)[value:value + target_size]
                assert constant == b"Water\0"
                target = 0x805E244C
                assert oracle.dol_bytes(dol, target, target_size) == constant
            else:
                target = symbols[name]
            if kind in (4, 6):
                immediate = target if kind == 4 else (target + 0x8000) >> 16
                struct.pack_into(">H", result, relative, immediate & 0xFFFF)
            elif kind == 10:
                word = struct.unpack_from(">I", result, relative)[0]
                displacement = target - ADDRESS - relative
                assert -0x02000000 <= displacement < 0x02000000 and displacement % 4 == 0
                struct.pack_into(">I", result, relative, (word & 0xFC000003) | (displacement & 0x03FFFFFC))
            else:
                raise AssertionError((name, kind))
            relocations.append({"offset": hex(relative), "kind": kind, "symbol": name, "target": hex(target)})
    expected = oracle.dol_bytes(dol, ADDRESS, SIZE)
    assert bytes(result) == expected
    evidence = {
        "function": SYMBOL, "address": hex(ADDRESS), "size": hex(SIZE),
        "instructions": SIZE // 4, "relocated_bytes_equal": True,
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "retail_function_sha256": hashlib.sha256(expected).hexdigest(),
        "source_sha256": hashlib.sha256((ROOT / "src/Game/Util/AreaObjUtil.cpp").read_bytes()).hexdigest(),
        "compiler": "GC/3.0a3; configure.py cflags_game; RMGK01 VERSION=0",
        "compiler_sha256": hashlib.sha256((ROOT / "build/compilers/GC/3.0a3/mwcceppc.exe").read_bytes()).hexdigest(),
        "command": command, "relocations": relocations,
    }
    (BUILD / "compiler-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"getWaterAreaObj: {SIZE // 4} matching instructions, {len(relocations)} checked relocations")


if __name__ == "__main__":
    main()
