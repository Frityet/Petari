#!/usr/bin/env python3
"""Compile root Binder and verify the recovered reaction against RMGK01.

The numerical oracle executes both retail and relocated compiler instructions.
It models only the called integer-vector constructor, copies, add, getNormal and
isNearZero contracts. This is not a native build or a full collision simulation.
"""
import ast
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import re
import shlex
import struct
import subprocess
import types

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/original-binder-reaction-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
ADDRESS, SIZE = 0x8015E1D4, 0x1DC
NAME = "obtainMomentFixReaction__6BinderFP7HitInfoUlPQ29JGeometry8TVec3<f>Ul"
spec = importlib.util.spec_from_file_location(
    "reader", ROOT / "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def f32(value):
    return struct.unpack(">f", struct.pack(">f", value))[0]


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def run(command, log):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (BUILD / log).write_text(result.stdout)
    print(result.stdout, end="")
    result.check_returncode()


def compile_and_split():
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Configured Game flags absent")
    command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        command.extend(shlex.split(flag))
    command.extend(["-c", "src/Game/LiveActor/Binder.cpp", "-o", str(BUILD / "Binder.o")])
    (BUILD / "Binder.command.json").write_text(json.dumps(command, indent=2) + "\n")
    run(command, "Binder.compile.log")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", str(BUILD / "config.yml"),
         str(BUILD / "retail")], "dtk.log")
    run(["build/tools/objdiff-cli", "diff", "-1", str(BUILD / "retail/obj/Game/LiveActor/Binder.o"),
         "-2", str(BUILD / "Binder.o"), "-o", str(BUILD / "objdiff.json"), "--format", "json-pretty"], "objdiff.log")


def relocate(elf, dol):
    _, start, size, section = next(s for s in elf.symbols if s[0] == NAME)
    code = bytearray(elf.section_data(section)[start:start + size])
    symbols = {name: int(address, 16) for name, address in re.findall(
        r"^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);", (ROOT / "config/RMGK01/symbols.txt").read_text(), re.M)}
    refs = elf.references(NAME)
    for ref in refs:
        offset = int(ref["offset"], 16)
        word = struct.unpack_from(">I", code, offset)[0]
        if ref["kind"] == 10:
            target = symbols[ref["symbol"]] + ref["addend"]
            displacement = target - ADDRESS - offset
            assert -0x02000000 <= displacement < 0x02000000
            word = (word & 0xFC000003) | (displacement & 0x3FFFFFC)
        elif ref["kind"] == 109:
            # Actual retail load at 0x8015E2E0: r2 + signed(0xBFD4).
            retail_word = int.from_bytes(reader.dol_bytes(dol, 0x8015E2E0, 4), "big")
            assert retail_word == 0xC022BFD4
            target = 0x806BFC20 + signed(retail_word & 65535, 16)
            bits = reader.dol_bytes(dol, target, 4).hex()
            assert bits == ref["value_hex"] == "3a83126f"
            word = (word & ~0x1FFFFF) | (2 << 16) | ((target - 0x806BFC20) & 65535)
        else:
            raise AssertionError(ref)
        ref["effective_retail_target"] = hex(target)
        struct.pack_into(">I", code, offset, word)
    return bytes(code), refs


class Machine:
    def __init__(self, dol, code):
        self.dol, self.code = dol, code
        self.mem, self.g, self.f, self.cr = {}, [0] * 32, [[0.0, 0.0] for _ in range(32)], [False] * 32
        self.g[1], self.g[2] = 0x200000, 0x806BFC20
        self.lr = 0
        self.branches, self.calls = set(), []

    def read(self, address, size=4):
        if all(address + i in self.mem for i in range(size)):
            data = bytes(self.mem[address + i] for i in range(size))
        elif address >= 0x80000000:
            data = reader.dol_bytes(self.dol, address, size)
        else:
            data = bytes(self.mem.get(address + i, 0) for i in range(size))
        return int.from_bytes(data, "big")

    def write(self, address, value, size=4):
        data = (value & ((1 << (size * 8)) - 1)).to_bytes(size, "big")
        self.mem.update({address + i: byte for i, byte in enumerate(data)})

    def getf(self, address):
        return struct.unpack(">f", self.read(address).to_bytes(4, "big"))[0]

    def putf(self, address, value):
        self.write(address, int.from_bytes(struct.pack(">f", value), "big"))

    def vector(self, address, values):
        for i, value in enumerate(values):
            self.putf(address + 4 * i, value)

    def values(self, address):
        return [self.getf(address + 4 * i) for i in range(3)]

    def call(self, address):
        self.calls.append(hex(address))
        if address == 0x805189FC:  # _savegpr_26
            for i in range(26, 32):
                self.write(self.g[11] - (32 - i) * 4, self.g[i])
        elif address == 0x80518A48:  # _restgpr_26
            for i in range(26, 32):
                self.g[i] = self.read(self.g[11] - (32 - i) * 4)
        elif address == 0x8003754C:  # TVec3f(integer,integer,integer)
            self.vector(self.g[3], [signed(x, 32) for x in self.g[4:7]])
        elif address in (0x80018EF0, 0x8001D2C8):  # Copy construction / set
            self.vector(self.g[3], self.values(self.g[4]))
        elif address == 0x8001D2E4:  # TVec3f::add
            self.vector(self.g[3], [f32(a + b) for a, b in zip(self.values(self.g[3]), self.values(self.g[4]))])
        elif address == 0x80182B78:  # Triangle::getNormal(i)
            self.g[3] += 12 + 12 * self.g[4]
        elif address == 0x803E6FE8:  # MR::isNearZero, actual ordered-comparison contract
            tolerance = self.f[1][0]
            self.g[3] = int(all(not (x > tolerance or x < -tolerance) for x in self.values(self.g[3])))
        else:
            raise AssertionError(f"Unmodeled helper {address:#x}")

    def execute(self):
        pc = ADDRESS
        for _ in range(10000):
            assert ADDRESS <= pc < ADDRESS + len(self.code)
            word = int.from_bytes(self.code[pc - ADDRESS:pc - ADDRESS + 4], "big")
            op, d, a, b, c = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31, (word >> 6) & 31
            imm, next_pc = signed(word & 65535, 16), pc + 4
            if word == 0x4E800020:
                return
            if op == 14:
                self.g[d] = ((self.g[a] if a else 0) + imm) & 0xFFFFFFFF
            elif op == 7:
                self.g[d] = (signed(self.g[a], 32) * imm) & 0xFFFFFFFF
            elif op == 11:
                x, y = signed(self.g[a], 32), imm
                self.cr[0:4] = [x < y, x > y, x == y, False]
            elif op in (32, 34):
                self.g[d] = self.read((self.g[a] + imm) & 0xFFFFFFFF, 4 if op == 32 else 1)
            elif op in (36, 37):
                address = (self.g[a] + imm) & 0xFFFFFFFF
                self.write(address, self.g[d])
                if op == 37:
                    self.g[a] = address
            elif op == 48:
                self.f[d][0] = self.getf((self.g[a] + imm) & 0xFFFFFFFF)
            elif op == 52:
                self.putf((self.g[a] + imm) & 0xFFFFFFFF, self.f[d][0])
            elif op == 21:
                shift, begin, end = b, (word >> 6) & 31, (word >> 1) & 31
                mask = sum(1 << (31 - i) for i in range(begin, end + 1))
                value = self.g[d]
                self.g[a] = ((value << shift) | (value >> (32 - shift))) & mask
                if word & 1:
                    self.cr[0:4] = [signed(self.g[a], 32) < 0, signed(self.g[a], 32) > 0, self.g[a] == 0, False]
            elif op == 31:
                xo = (word >> 1) & 1023
                if xo == 444:
                    self.g[a] = self.g[d] | self.g[b]
                elif xo == 266:
                    self.g[d] = (self.g[a] + self.g[b]) & 0xFFFFFFFF
                elif xo == 339:
                    self.g[d] = self.lr
                elif xo == 467:
                    self.lr = self.g[d]
                elif xo == 32:
                    x, y = self.g[a], self.g[b]
                    self.cr[0:4] = [x < y, x > y, x == y, False]
                else:
                    raise AssertionError((hex(pc), hex(word), "op31"))
            elif op == 59:
                assert (word >> 1) & 31 == 25
                self.f[d][0] = f32(self.f[a][0] * self.f[c][0])
            elif op == 63:
                assert (word >> 1) & 1023 == 32
                x, y = self.f[a][0], self.f[b][0]
                self.cr[0:4] = [x < y, x > y, x == y, math.isnan(x) or math.isnan(y)]
            elif op in (56, 60):
                address = (self.g[a] + signed(word & 4095, 12)) & 0xFFFFFFFF
                count = 1 if (word >> 15) & 1 else 2
                assert (word >> 12) & 7 == 0
                if op == 56:
                    self.f[d] = [self.getf(address + 4 * i) for i in range(count)] + ([1.0] if count == 1 else [])
                else:
                    for i in range(count):
                        self.putf(address + 4 * i, self.f[d][i])
            elif op == 4:
                assert (word >> 1) & 1023 == 21
                self.f[d] = [f32(x + y) for x, y in zip(self.f[a], self.f[b])]
            elif op == 16:
                assert d in (4, 12)
                taken = self.cr[a] == (d == 12)
                self.branches.add((pc - ADDRESS, taken))
                if taken:
                    next_pc = pc + signed(word & 0xFFFC, 16)
            elif op == 18:
                target = pc + signed(word & 0x3FFFFFC, 26)
                if word & 1:
                    self.lr = next_pc
                    self.call(target)
                else:
                    next_pc = target
            else:
                raise AssertionError((hex(pc), hex(word), op))
            pc = next_pc
        raise AssertionError("Instruction budget exceeded")


def verify_behavior(dol, retail, compiled):
    records, coverage = [], [set(), set()]
    zero = (0, 0, 0)
    def case(label, contacts, expected, start=0, count=None, capacity=32, flags=0x40, alias=False):
        count = len(contacts) if count is None else count
        outputs = []
        for side, code in enumerate((retail, compiled)):
            m = Machine(dol, code)
            binder, planes, output = 0x10000, 0x11000, 0x12000
            if alias:
                output = planes + 0x7C
            m.write(binder + 0x28, count)
            m.write(binder + 0x1EC, flags, 1)
            for index, (normal, depth, moving) in enumerate(contacts):
                address = planes + 0x8C * index
                m.vector(address + 12, normal)
                m.putf(address + 0x60, depth)
                m.vector(address + 0x7C, moving)
            m.g[3:8] = [binder, planes, capacity, output, start]
            m.execute()
            actual = m.values(output)
            assert struct.pack(">3f", *actual) == struct.pack(">3f", *expected), (label, side, actual, expected)
            assert m.read(binder + 0x28) == count and m.read(binder + 0x1EC, 1) == flags
            outputs.append(actual)
            coverage[side].update(m.branches)
        records.append({"case": label, "contacts": contacts, "start": start, "count": count,
                        "capacity": capacity, "flags": flags, "output_aliases_first_movement": alias,
                        "output": outputs[0], "retail_and_original_compiler_bits_equal": True})

    case("empty overwrites output", [], zero)
    case("single non-unit normal is used unchanged", [((2, -3, 4), 0.5, zero)], (1, -1.5, 2))
    case("nonorthogonal planes use component extrema", [((1, 0, 0), 1, zero), ((0.6, 0.8, 0), 1, zero)], (1, 0.8, 0))
    case("positive and negative extrema both contribute", [((1, 3, -1), 2, zero), ((-3, -1, 2), 1, zero)], (-1, 5, 0))
    case("opposing corrections cancel", [((1, 0, 0), 2, zero), ((-1, 0, 0), 2, zero)], zero)
    case("negative depth is not clamped", [((1, -2, 3), -2, zero)], (-2, 4, -6))
    case("suffix starts at supplied index", [((100, 100, 100), 1, zero), ((-1, 2, 3), 2, zero)], (-2, 4, 6), start=1)
    case("member count limits array", [((1, 2, 3), 1, zero), ((100, 100, 100), 1, zero)], (1, 2, 3), count=1)
    case("capacity parameter is unused", [((1, 2, 3), 1, zero)], (1, 2, 3), capacity=0)
    case("start beyond count returns zero", [((1, 2, 3), 1, zero)], zero, start=2)
    case("moving reaction merges independently of depth", [((1, 0, 0), 2, (4, -3, 1))], (4, -3, 1))
    case("moving reaction combines signs with normal correction", [((1, -1, 1), 2, (-5, 3, -4))], (-3, 1, -2))
    case("moving flag disabled", [((1, 0, 0), 2, (4, -3, 1))], (2, 0, 0), flags=0)
    case("neighbor flags do not enable moving reaction", [((1, 0, 0), 2, (4, -3, 1))], (2, 0, 0), flags=0xBF)
    epsilon = f32(0.001)
    above = struct.unpack(">f", struct.pack(">I", 0x3A831270))[0]
    case("all moving components at epsilon are ignored", [(zero, 0, (epsilon, -epsilon, epsilon))], zero)
    case("one outside component includes all components", [(zero, 0, (above, -epsilon, epsilon))], (above, -epsilon, epsilon))
    case("negative outside component enables moving reaction", [(zero, 0, (epsilon, -above, -epsilon))], (epsilon, -above, -epsilon))
    case("output may alias already-consumed input", [((1, 2, 3), 2, (4, -3, 1))], (4, 1, 6), alias=True)
    case("later smaller extrema do not overwrite", [((3, -4, 5), 1, zero), ((1, -2, 3), 1, zero)], (3, -4, 5))
    case("equal extrema do not accumulate", [((1, -2, 3), 1, zero), ((1, -2, 3), 1, zero)], (1, -2, 3))

    rng = random.Random(0x8015E1D4)
    for index in range(256):
        contacts = [(tuple(rng.randint(-8, 8) / 4 for _ in range(3)), rng.randint(-8, 8) / 2,
                     tuple(rng.randint(-8, 8) / 4 for _ in range(3))) for _ in range(rng.randint(1, 8))]
        count, start, flags = rng.randint(0, len(contacts)), rng.randint(0, len(contacts)), rng.choice((0, 0x40, 0xBF, 0xFF))
        vectors = []
        for normal, depth, movement in contacts[start:count]:
            vectors.append([f32(x * depth) for x in normal])
            if flags & 0x40 and any(abs(x) > epsilon for x in movement):
                vectors.append(movement)
        expected = [f32(max([0] + [v[i] for v in vectors]) + min([0] + [v[i] for v in vectors])) for i in range(3)]
        case(f"seeded finite combination {index}", contacts, expected, start=start, count=count, flags=flags)
    for side, code in enumerate((retail, compiled)):
        offsets = {i for i in range(0, len(code), 4) if code[i] >> 2 == 16}
        assert coverage[side] == {(i, taken) for i in offsets for taken in (False, True)}, (side, offsets, coverage[side])
    return {"case_count": len(records), "fixed_case_count": 20, "seeded_case_count": 256,
            "every_conditional_branch_taken_and_not_taken": True,
            "retail_branch_outcomes": sorted(coverage[0]), "compiled_branch_outcomes": sorted(coverage[1]),
            "fixed_cases": records[:20]}


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    source = ROOT / "src/Game/LiveActor/Binder.cpp"
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    compile_and_split()
    assert hashlib.sha256(source.read_bytes()).hexdigest() == digest
    elf = reader.Elf(BUILD / "Binder.o")
    code, refs = relocate(elf, dol)
    retail = reader.dol_bytes(dol, ADDRESS, SIZE)
    (BUILD / "reaction.retail.bin").write_bytes(retail)
    (BUILD / "reaction.compiled.relocated.bin").write_bytes(code)
    diff = json.loads((BUILD / "objdiff.json").read_text())
    symbols = [next(s for s in diff[side]["symbols"] if s["name"] == NAME) for side in ("left", "right")]
    comparison = []
    for left, right in zip(symbols[0]["instructions"], symbols[1]["instructions"]):
        cells = [f"{i.get('diff_kind', ''):20} {i.get('instruction', {}).get('formatted', '')}" for i in (left, right)]
        comparison.append(f"{cells[0]:110} | {cells[1]}")
    (BUILD / "reaction-comparison.txt").write_text("\n".join(comparison) + "\n")
    evidence = {"method": NAME, "address": hex(ADDRESS), "retail_size": SIZE, "compiled_size": len(code),
                "objdiff_match_percent": symbols[0]["match_percent"], "source_sha256": digest,
                "dol_sha1": hashlib.sha1(dol).hexdigest(), "retail_function_sha256": hashlib.sha256(retail).hexdigest(),
                "compiler": "GC3.0a3/configure.py cflags_game, no matching-only source pragmas",
                "relocations": refs, "behavior": verify_behavior(dol, retail, code),
                "limits": "Finite binary32 cases and all method branch outcomes. SDK helpers modeled by their simple existing contracts; no native SDK, KCL query, moving-platform lifecycle, or complete Binder runtime execution."}
    (BUILD / "evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"Reaction: {symbols[0]['match_percent']:.5f}% objdiff; {evidence['behavior']['case_count']} identical retail/compiler numeric cases; all conditional outcomes covered.")


if __name__ == "__main__":
    main()
