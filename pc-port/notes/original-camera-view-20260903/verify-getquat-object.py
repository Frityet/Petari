#!/usr/bin/env python3
"""Compile recovered getQuat and compare relocated floating-point control paths."""

import argparse
import ast
import copy
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess
import types


ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/compat-camera-view"
SYMBOL = "getQuat__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix34<Q29JGeometry13SMatrix34C<f>>>CFRQ29JGeometry9TQuat4<f>"
ADDRESS = 0x800173A0
SIZE = 0x22C
SQRT_ADDRESS = 0x80017320
SDA2 = 0x806BFC20
CONSTANTS = {"00000000": 0x806B7D34, "3f800000": 0x806B7D30, "3f000000": 0x806B7D40}


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
            name = names[name:names.index(0, name)].decode()
            self.symbols.append((name, value, size, index))

    def section_data(self, index):
        section = self.sections[index]
        return self.data[section[4]:section[4] + section[5]]


def dol_bytes(data, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from(">I", data, field + index * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start:start + size]
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
    command.extend(["-c", "src/JSystem/JGeometry/TMatrix.cpp", "-o", str(BUILD / "TMatrix.o")])
    (BUILD / "TMatrix.command.txt").write_text(shlex.join(command) + "\n")
    (BUILD / "TMatrix.command.json").write_text(json.dumps(command, indent=2) + "\n")
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / "TMatrix.compile.log").write_text(result.stdout)
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
            word = struct.unpack_from(">I", code, relative)[0]
            assert addend == 0
            if kind == 10:  # R_PPC_REL24
                assert name == "sqrt__Q29JGeometry8TUtil<f>Ff"
                target = SQRT_ADDRESS
                word = (word & 0xFC000003) | ((target - ADDRESS - relative) & 0x03FFFFFC)
            elif kind == 109:  # R_PPC_EMB_SDA21
                assert symbol_size == 4
                constant = elf.section_data(symbol_section)[value:value + 4]
                target = CONSTANTS[constant.hex()]
                assert constant == dol_bytes(dol, target, 4)
                displacement = target - SDA2
                assert -0x8000 <= displacement < 0x8000
                word = (word & 0xFFE00000) | (2 << 16) | (displacement & 0xFFFF)
            else:
                raise AssertionError((kind, name))
            struct.pack_into(">I", code, relative, word)
            records.append({"offset": hex(relative), "kind": kind, "symbol": name, "target": hex(target)})
    assert len(records) == 13
    return bytes(code), records


def arithmetic(kind, left, right):
    # Preserve every f32 rounding boundary; commute operands only within
    # one add/multiply for finite matrix values. Do not reassociate terms.
    if kind in ("fadds", "fmuls"):
        left, right = sorted((left, right), key=repr)
    return (kind, left, right)


def control_paths(code):
    pending = [{"pc": 0, "gpr": {2: SDA2, 3: "matrix", 4: "output"},
                "fpr": {}, "cr": {}, "conditions": [], "output": {}}]
    finished = []
    reverse_constants = {address: bits for bits, address in CONSTANTS.items()}
    while pending:
        state = pending.pop()
        for _ in range(256):
            pc = state["pc"]
            word = struct.unpack_from(">I", code, pc)[0]
            op = word >> 26
            target, left, right, third = (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31, (word >> 6) & 31
            immediate = (word & 0x7FFF) - (word & 0x8000)
            gpr, fpr, cr = state["gpr"], state["fpr"], state["cr"]
            state["pc"] += 4
            if op == 48:  # lfs
                pointer = gpr[left]
                if pointer == "matrix":
                    assert immediate in range(0, 48, 4)
                    fpr[target] = ("matrix", immediate // 16, (immediate % 16) // 4)
                else:
                    assert pointer == SDA2
                    fpr[target] = ("constant", reverse_constants[pointer + immediate])
            elif op == 52:  # stfs
                assert gpr[left] == "output" and immediate in (0, 4, 8, 12)
                assert immediate not in state["output"]
                state["output"][immediate] = fpr[target]
            elif op == 59:
                kind = {18: "fdivs", 20: "fsubs", 21: "fadds", 25: "fmuls"}[(word >> 1) & 31]
                fpr[target] = arithmetic(kind, fpr[left], fpr[third if kind == "fmuls" else right])
            elif op == 63 and ((word >> 1) & 1023) == 72:  # fmr
                fpr[target] = fpr[right]
            elif op == 63 and ((word >> 1) & 1023) in (0, 32):  # fcmpu/fcmpo, CR0
                assert (word >> 23) & 7 == 0
                comparison = "ordered" if ((word >> 1) & 1023) == 32 else "unordered"
                for bit, relation in enumerate(("lt", "gt", "eq", "nan")):
                    cr[bit] = (comparison, relation, fpr[left], fpr[right])
            elif word == 0x4C411382:  # cror 2,1,2
                cr[2] = ("or", cr[1], cr[2])
            elif op == 16:  # All conditions use branch-if-false on CR0 eq.
                assert target == 4 and left == 2 and not word & 3
                displacement = (word & 0x7FFC) - (word & 0x8000)
                branch = copy.deepcopy(state)
                branch["pc"] = pc + displacement
                branch["conditions"].append((cr[left], False))
                pending.append(branch)
                state["conditions"].append((cr[left], True))
            elif op == 18:
                displacement = (word & 0x01FFFFFC) - (word & 0x02000000)
                assert not word & 2
                if word & 1:
                    assert ADDRESS + pc + displacement == SQRT_ADDRESS
                    argument = fpr[1]
                    fpr.clear()
                    fpr[1] = ("TUtil_sqrt", argument)
                else:
                    state["pc"] = pc + displacement
            elif op == 31 and ((word >> 1) & 1023) == 444:  # mr
                assert target == right
                gpr[left] = gpr[target]
            elif word == 0x4E800020:
                assert set(state["output"]) == {0, 4, 8, 12}
                finished.append({"conditions": state["conditions"], "output": state["output"]})
                break
            elif op in (32, 36, 37):  # Preserved GPR stack loads/stores.
                assert left == 1
            elif op == 14:  # Final stack restore.
                assert target == left == 1 and immediate == 16
            elif word in (0x7C0802A6, 0x7C0803A6):  # mflr/mtlr
                pass
            else:
                raise AssertionError(f"Unsupported instruction {word:08x} at {pc:#x}")
        else:
            raise AssertionError("Unexpected loop in getQuat")
    return sorted(finished, key=lambda path: json.dumps(path, sort_keys=True))


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
    compiled, relocations = relocate(Elf(BUILD / "TMatrix.o"), dol)
    retail = dol_bytes(dol, ADDRESS, SIZE)
    (BUILD / "getQuat_compiled_relocated.bin").write_bytes(compiled)
    retail_paths = control_paths(retail)
    compiled_paths = control_paths(compiled)
    assert retail_paths == compiled_paths
    formulas = sorted({json.dumps(path["output"], sort_keys=True) for path in retail_paths})
    result = {
        "function": SYMBOL,
        "compiled_size": hex(len(compiled)), "retail_size": hex(len(retail)),
        "compiled_instructions": len(compiled) // 4, "retail_instructions": len(retail) // 4,
        "relocations": relocations,
        "matching_symbolic_paths": len(retail_paths),
        "matching_unique_quaternion_formulas": len(formulas),
        "output_formulas": [json.loads(formula) for formula in formulas],
        "path_hashes": [hashlib.sha256(json.dumps(path, sort_keys=True).encode()).hexdigest() for path in retail_paths],
        "compiler_sha256": hashlib.sha256((ROOT / "build/compilers/GC/3.0a3/mwcceppc.exe").read_bytes()).hexdigest(),
        "object_sha256": hashlib.sha256((BUILD / "TMatrix.o").read_bytes()).hexdigest(),
    }
    (BUILD / "getQuat-compiler-evidence.json").write_text(json.dumps(result, indent=2) + "\n")
    report = (
        f"GC/3.0a3 getQuat: {len(compiled) // 4} emitted instructions vs {len(retail) // 4} retail.\n"
        f"Resolved {len(relocations)} relocations: four original sqrt calls and nine verified SDA2 constants.\n"
        f"All {len(retail_paths)} symbolic control paths and {len(formulas)} quaternion output formulas match.\n"
        "Each finite f32 operation grouping and all branch comparisons are retained.\n"
        "Differences: one extra diagonal reload, register allocation, and load/store scheduling.\n"
        "This is functional compiler evidence, not a byte-identical object match.\n"
    )
    (BUILD / "getQuat-verification.txt").write_text(report)
    print(report, end="")


if __name__ == "__main__":
    main()
