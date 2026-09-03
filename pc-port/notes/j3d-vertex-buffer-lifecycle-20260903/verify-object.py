#!/usr/bin/env python3
"""Compile actual root SDK/Game sources and compare five small retail methods.

No native build and no generated replacement headers. Generated binary artifacts
remain under build/j3d-vertex-buffer-lifecycle-20260903.
"""

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
BUILD = ROOT / "build/j3d-vertex-buffer-lifecycle-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
FUNCTIONS = (
    ("J3DVertex", "setVertexData__15J3DVertexBufferFP13J3DVertexData", 0x80423874, 0x48),
    ("J3DVertex", "init__15J3DVertexBufferFv", 0x804238BC, 0x40),
    ("J3DVertex", "__dt__15J3DVertexBufferFv", 0x804238FC, 0x40),
    ("J3DVertex", "frameInit__15J3DVertexBufferFv", 0x804239B4, 0x1C),
    ("XanimeCore", "getJointTransform__10XanimeCoreFUl", 0x802AEC3C, 0x20),
)
SOURCES = {
    "J3DVertex": ("src/JSystem/J3DGraphBase/J3DVertex.cpp", "cflags_jsys",
                  "JSystem/J3DGraphBase/J3DVertex.o"),
    "XanimeCore": ("src/Game/Animation/XanimeCore.cpp", "cflags_game", "Game/Player/Mario.o"),
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (BUILD / log).write_text(result.stdout)
    if result.stdout:
        print(result.stdout, end="")
    result.check_returncode()


def dol_bytes(dol, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from(">I", dol, field + index * 4)[0]
                                for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            return dol[offset + address - base:offset + address - base + size]
    raise AssertionError((hex(address), size))


class Elf:
    def __init__(self, path):
        self.data = path.read_bytes()
        assert self.data[:6] == b"\x7fELF\x01\x02"
        offset = struct.unpack_from(">I", self.data, 0x20)[0]
        stride, count = struct.unpack_from(">HH", self.data, 0x2E)
        self.sections = [struct.unpack_from(">10I", self.data, offset + i * stride) for i in range(count)]
        section = next(s for s in self.sections if s[1] == 2)
        names = self.section_data(section[6])
        self.symbols = []
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", self.data, offset)
            self.symbols.append((names[name:names.index(0, name)].decode(), value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4] + section[5]]

    def function(self, name):
        _, start, size, index = next(s for s in self.symbols if s[0] == name)
        code = bytearray(self.section_data(index)[start:start + size])
        references = []
        for section in self.sections:
            if section[1] != 4 or section[7] != index:
                continue
            for offset in range(section[4], section[4] + section[5], section[9]):
                at, info, addend = struct.unpack_from(">IIi", self.data, offset)
                if not start <= at < start + size:
                    continue
                symbol = self.symbols[info >> 8][0]
                kind = info & 255
                references.append({"offset": at - start, "kind": kind,
                                   "symbol": symbol, "addend": addend})
                assert kind == 10, (name, "Only REL24 calls/tail branches expected", kind)
                # Preserve opcode and AA/LK. Normalize only the relocated branch displacement.
                instruction = struct.unpack_from(">I", code, at - start)[0]
                struct.pack_into(">I", code, at - start, instruction & 0xFC000003)
        return bytes(code), references


def compiler(flags_name):
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == flags_name for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Configured compiler flags missing: " + flags_name)
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    return command


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    sources = {}
    commands = {}
    for unit, (source, flags, _) in SOURCES.items():
        sources[source] = sha(ROOT / source)
        command = compiler(flags) + ["-c", source, "-o", str(BUILD / (unit + ".o"))]
        commands[unit] = command
        run(command, unit + ".compile.log")
        assert sources[source] == sha(ROOT / source), "Source changed during compilation"
    (BUILD / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")

    checks = {
        "sizeof(J3DVertexBuffer)": 0x38,
        "offsetof(J3DVertexBuffer, mVtxData)": 0x00,
        "offsetof(J3DVertexBuffer, mVtxPosArray)": 0x04,
        "offsetof(J3DVertexBuffer, mVtxNrmArray)": 0x0C,
        "offsetof(J3DVertexBuffer, mVtxColArray)": 0x14,
        "offsetof(J3DVertexBuffer, mTransformedVtxPosArray)": 0x1C,
        "offsetof(J3DVertexBuffer, mTransformedVtxNrmArray)": 0x24,
        "offsetof(J3DVertexBuffer, mCurrentVtxPos)": 0x2C,
        "offsetof(J3DVertexBuffer, mCurrentVtxNrm)": 0x30,
        "offsetof(J3DVertexBuffer, mCurrentVtxCol)": 0x34,
        "offsetof(J3DVertexData, mVtxPosArray)": 0x18,
        "offsetof(J3DVertexData, mVtxNrmArray)": 0x1C,
        "offsetof(J3DVertexData, mVtxColorArray)": 0x24,
        "sizeof(XjointTransform)": 0x70,
        "offsetof(XanimeCore, mTransformList)": 0x14,
    }
    probe = '#include "JSystem/J3DGraphBase/J3DVertex.hpp"\n#include "Game/Animation/XanimeCore.hpp"\n#include <stddef.h>\n'
    probe += "\n".join(f"typedef char Check{i}[({expr} == {value}) ? 1 : -1];"
                       for i, (expr, value) in enumerate(checks.items())) + "\n"
    (BUILD / "layout.cpp").write_text(probe)
    run(compiler("cflags_game") + ["-c", str(BUILD / "layout.cpp"), "-o", str(BUILD / "layout.o")],
        "layout.compile.log")

    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", str(BUILD / "config.yml"),
         str(BUILD / "retail")], "dtk.log")
    comparisons = {}
    objects = {}
    for unit, (_, _, target_name) in SOURCES.items():
        target = BUILD / "retail/obj" / target_name
        compiled = BUILD / (unit + ".o")
        run(["build/tools/objdiff-cli", "diff", "-1", str(target), "-2", str(compiled),
             "-o", str(BUILD / (unit + ".objdiff.json")), "--format", "json-pretty"], unit + ".objdiff.log")
        comparisons[unit] = json.loads((BUILD / (unit + ".objdiff.json")).read_text())
        objects[unit] = (Elf(target), Elf(compiled))

    evidence = {
        "scope": "Three restored VertexBuffer methods, existing inline frameInit, and restored Core getter; exact relocation-aware original compiler comparison.",
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "compiler": "GC/3.0a3; configure.py cflags_jsys for J3DVertex and cflags_game for XanimeCore; RMGK01 VERSION=0",
        "source_sha256": sources,
        "header_sha256": {str(p): sha(ROOT / p) for p in (
            "libs/JSystem/include/JSystem/J3DGraphBase/J3DVertex.hpp", "include/Game/Animation/XanimeCore.hpp")},
        "tool_sha256": {str(p): sha(ROOT / p) for p in (
            "build/compilers/GC/3.0a3/mwcceppc.exe", "build/tools/sjiswrap.exe",
            "build/tools/dtk", "build/tools/objdiff-cli")},
        "verified_ppc_layout": checks,
        "normalization": "Only REL24 branch displacement bits are masked; opcode, AA/LK and complete other instruction words are retained. Relocation offset/type/name/addend must agree exactly.",
        "functions": [],
    }
    aligned = []
    for unit, name, address, size in FUNCTIONS:
        diff = comparisons[unit]
        sides = [next(s for s in diff[k]["symbols"] if s["name"] == name) for k in ("left", "right")]
        assert [int(s["size"]) for s in sides] == [size, size], name
        bytes_refs = [obj.function(name) for obj in objects[unit]]
        assert bytes_refs[0] == bytes_refs[1], (name, "Instructions/relocations differ")
        assert sides[0]["match_percent"] == 100, name
        evidence["functions"].append({
            "name": name, "address": hex(address), "retail_size": size, "compiled_size": int(sides[1]["size"]),
            "objdiff_match_percent": sides[0]["match_percent"],
            "relocation_normalized_instructions_equal": True,
            "retail_function_sha256": hashlib.sha256(dol_bytes(dol, address, size)).hexdigest(),
            "references": bytes_refs[0][1],
        })
        aligned.append(f"\n{name}: retail {address:#x}, {size} bytes, 100%")
        for left, right in zip(sides[0]["instructions"], sides[1]["instructions"]):
            texts = [entry.get("instruction", {}).get("formatted", "") for entry in (left, right)]
            aligned.append(f"{texts[0]:100} | {texts[1]}")
        print(f"{name}: 100%, {size} bytes; instructions and relocations identical")
    (BUILD / "compiler-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    (BUILD / "function-comparison.txt").write_text("\n".join(aligned) + "\n")


if __name__ == "__main__":
    main()
