#!/usr/bin/env python3
"""Compile root/import with retail flags and verify the native ABI edits are neutral on Wii."""

import ast
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess
import types


ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/compat-xanime-resource"


class Elf:
    def __init__(self, path):
        self.data = path.read_bytes()
        assert self.data[:6] == b"\x7fELF\x01\x02"
        offset = struct.unpack_from(">I", self.data, 0x20)[0]
        size, count = struct.unpack_from(">HH", self.data, 0x2E)
        self.sections = [struct.unpack_from(">10I", self.data, offset + i * size) for i in range(count)]
        table = next(s for s in self.sections if s[1] == 2)
        names = self.section_data(table[6])
        self.symbols = []
        for offset in range(table[4], table[4] + table[5], table[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", self.data, offset)
            self.symbols.append((names[name:names.index(0, name)].decode(), value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4] + section[5]]

    def function(self, symbol):
        name, start, size, section_index = symbol
        code = self.section_data(section_index)[start:start + size]
        relocations = []
        for section in self.sections:
            if section[1] != 4 or section[7] != section_index:
                continue
            for offset in range(section[4], section[4] + section[5], section[9]):
                location, info, addend = struct.unpack_from(">IIi", self.data, offset)
                if not start <= location < start + size:
                    continue
                target_name, value, target_size, target_section = self.symbols[info >> 8]
                target = target_name
                if target_name.startswith("@"):
                    target = self.section_data(target_section)[value:value + target_size].hex()
                relocations.append((location - start, info & 255, addend, target))
        return code, relocations


def dol_bytes(data, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from(">I", data, field + index * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start:start + size]
    raise AssertionError(f"DOL range absent: {address:#x}+{size:#x}")


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    hash_source = (ROOT / "src/Game/Util/HashUtil.cpp").read_text()
    original_methods = hash_source[hash_source.index("HashSortTable::HashSortTable"):hash_source.index("\nnamespace MR {")]
    native_hash = (ROOT / "pc-port/src/compat/HashSortTableCompat.cpp").read_text()
    native_methods = native_hash[native_hash.index("HashSortTable::HashSortTable"):native_hash.index("\nnamespace MR {")]
    assert original_methods == native_methods, "HashSortTable method extraction differs from root source"
    math_source = (ROOT / "src/Game/Util/MathUtil.cpp").read_text()
    sort_start = math_source.index("    void sortSmall(s32 length, u32* sortArray, s32* indexArray) {")
    sort_end = math_source.index("\n    };", sort_start) + len("\n    };")
    original_sort = math_source[sort_start:sort_end]
    native_sort_start = native_hash.index("    void sortSmall(s32 length, u32* sortArray, s32* indexArray) {")
    native_sort_end = native_hash.index("\n    };", native_sort_start) + len("\n    };")
    assert original_sort == native_hash[native_sort_start:native_sort_end], "Unsigned sortSmall extraction differs from root source"
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Configured Game compiler flags absent")

    sources = {"root": "src/Game/Animation/XanimeResource.cpp", "native-import": "pc-port/src/Game/Animation/XanimeResource.cpp"}
    objects = {}
    for label, source in sources.items():
        command = ["build/tools/wibo", "build/compilers/GC/3.0a3/mwcceppc.exe"]
        for flag in flags:
            command.extend(shlex.split(flag))
        command.extend(["-c", source, "-o", str(BUILD / f"{label}.o")])
        (BUILD / f"{label}.command.json").write_text(json.dumps(command, indent=2) + "\n")
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (BUILD / f"{label}.compile.log").write_text(result.stdout)
        print(result.stdout, end="")
        result.check_returncode()
        objects[label] = Elf(BUILD / f"{label}.o")

    reference, imported = objects["root"], objects["native-import"]
    functions = []
    for symbol in reference.symbols:
        name, start, size, index = symbol
        if name.startswith(".") or size == 0 or index == 0 or ("__19XanimeResourceTable" not in name and "__15XanimeGroupInfo" not in name):
            continue
        actual = next(s for s in imported.symbols if s[0] == name)
        expected_code, expected_relocations = reference.function(symbol)
        actual_code, actual_relocations = imported.function(actual)
        assert actual_code == expected_code, (name, "instruction difference")
        assert actual_relocations == expected_relocations, (name, "relocation difference")
        functions.append({"symbol": name, "bytes": size, "relocations": len(expected_relocations)})

    dol = (ROOT / "build/compat-math-oracle/main.dol").read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    # initGroupInfo reads exactly the fields named in the original J3DAnmBase header.
    assert dol_bytes(dol, 0x8001CCE8, 4) == bytes.fromhex("88640004")  # lbz r3, 4(r4)
    assert dol_bytes(dol, 0x8001CCFC, 4) == bytes.fromhex("a8040006")  # lha r0, 6(r4)
    report = {
        "comparison": "Root vs native import compiled with original Wii toolchain; this is not a whole-TU retail match claim.",
        "sources": {source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest() for source in sources.values()},
        "dol_sha1": hashlib.sha1(dol).hexdigest(),
        "exact_source_helpers": {
            "HashSortTable methods": hashlib.sha256(original_methods.encode()).hexdigest(),
            "MR::sortSmall(s32,u32*,s32*)": hashlib.sha256(original_sort.encode()).hexdigest(),
        },
        "functions": functions,
        "instruction_count": sum(f["bytes"] // 4 for f in functions),
        "relocation_count": sum(f["relocations"] for f in functions),
    }
    output = Path(__file__).with_name("resource-import-evidence.json")
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Verified {len(functions)} functions, {report['instruction_count']} unchanged PPC instructions, {report['relocation_count']} identical relocations.")
    print(f"Evidence: {output.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
