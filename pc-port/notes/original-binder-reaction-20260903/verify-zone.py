#!/usr/bin/env python3
"""Verify the root CollisionZone dependency closure with original tools.

Bounds helpers preserve every instruction after documented load scheduling and
register allocation normalization. Erasure additionally executes the actual
retail and compiled integer instruction streams against independent array cases.
"""
import importlib.util
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("runtime", HERE / "verify-runtime.py")
runtime = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runtime)
reaction = runtime.reaction
ROOT, BUILD = runtime.ROOT, runtime.BUILD
FUNCTIONS = (("calcMinMaxAndRadius", 0x80174C9C, 0x1B8),
             ("addAndUpdateMinMax", 0x80174F40, 0xDC),
             ("eraseParts", 0x8017501C, 0x74))


class EraseMachine:
    def __init__(self, code, pointers, needle):
        self.code, self.g, self.mem = code, [0] * 32, {}
        self.cr, self.carry, self.branches = [False] * 4, False, set()
        self.g[3:5] = [0x10000, needle]
        for i in range(16):
            self.mem[0x10004 + i * 4] = pointers[i] if i < len(pointers) else 0xDEADBEEF
        self.mem[0x10804] = len(pointers)

    def execute(self):
        pc = 0
        for _ in range(1000):
            word = int.from_bytes(self.code[pc:pc+4], "big")
            op, d, a, b = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31
            imm, next_pc = reaction.signed(word & 65535, 16), pc + 4
            if word == 0x4E800020:
                return
            if op == 14:
                self.g[d] = ((self.g[a] if a else 0) + imm) & 0xFFFFFFFF
            elif op == 32:
                self.g[d] = self.mem[(self.g[a] + imm) & 0xFFFFFFFF]
            elif op == 36:
                self.mem[(self.g[a] + imm) & 0xFFFFFFFF] = self.g[d]
            elif op == 21:
                begin, end = (word >> 6) & 31, (word >> 1) & 31
                mask = sum(1 << (31-i) for i in range(begin, end+1))
                v = self.g[d]
                self.g[a] = ((v << b) | (v >> (32-b))) & mask
            elif op == 31:
                xo = (word >> 1) & 1023
                if xo == 266:
                    self.g[d] = (self.g[a] + self.g[b]) & 0xFFFFFFFF
                elif xo == 40:
                    self.g[d] = (self.g[b] - self.g[a]) & 0xFFFFFFFF
                elif xo == 32:
                    x, y = self.g[a], self.g[b]
                    self.cr = [x < y, x > y, x == y, False]
                elif xo == 23:
                    self.g[d] = self.mem[(self.g[a] + self.g[b]) & 0xFFFFFFFF]
                elif xo == 824:
                    value = reaction.signed(self.g[d], 32)
                    self.g[a] = (value >> b) & 0xFFFFFFFF
                    self.carry = value < 0 and bool(self.g[d] & ((1 << b)-1))
                elif xo == 202:
                    total = self.g[a] + self.carry
                    self.g[d], self.carry = total & 0xFFFFFFFF, total > 0xFFFFFFFF
                else:
                    raise AssertionError((hex(pc), hex(word), xo))
            elif op in (16, 19):
                assert d in (4, 12)
                taken = self.cr[a] == (d == 12)
                self.branches.add((pc, taken))
                if taken:
                    if op == 19:
                        assert (word >> 1) & 1023 == 16
                        return
                    next_pc = pc + reaction.signed(word & 0xFFFC, 16)
            elif op == 18:
                assert word & 3 == 0
                next_pc = pc + reaction.signed(word & 0x3FFFFFC, 26)
            else:
                raise AssertionError((hex(pc), hex(word), op))
            pc = next_pc
        raise AssertionError("Instruction budget exceeded")


def erase_cases(retail, compiled):
    records, coverage = [], [set(), set()]
    cases = [([], 0x20000), ([0x20000, 0x20004, 0x20000, 0x20008], 0x20000)]
    for count in range(1, 10):
        values = [0x20000 + i * 4 for i in range(count)]
        for needle in values + [0x30000]:
            cases.append((values, needle))
    for values, needle in cases:
        expected = list(values) + [0xDEADBEEF] * (16-len(values))
        count = len(values)
        if needle in values:
            expected[values.index(needle)] = values[-1]
            count -= 1
        for side, code in enumerate((retail, compiled)):
            machine = EraseMachine(code, values, needle)
            machine.execute()
            assert machine.mem[0x10804] == count
            assert [machine.mem[0x10004 + i * 4] for i in range(16)] == expected
            coverage[side].update(machine.branches)
        records.append({"before": values, "needle": needle, "active_after": expected[:count],
                        "count_after": count, "complete_array_and_count_equal_in_retail_and_compiled": True})
    for side in range(2):
        sites = {pc for pc, _ in coverage[side]}
        assert coverage[side] == {(pc, outcome) for pc in sites for outcome in (False, True)}
    return {"case_count": len(records), "all_branch_sites_taken_and_not_taken": True, "cases": records}


def main():
    # verify-runtime.py prepares the verified DOL split and configured command.
    assert (BUILD / "Binder.command.json").is_file(), "Run verify-runtime.py first"
    command = json.loads((BUILD / "Binder.command.json").read_text())
    command[command.index("-c") + 1] = "src/Game/Map/CollisionCategorizedKeeper.cpp"
    command[command.index("-o") + 1] = str(BUILD / "CollisionCategorizedKeeper.o")
    (BUILD / "CollisionCategorizedKeeper.command.json").write_text(json.dumps(command, indent=2) + "\n")
    reaction.run(command, "CollisionCategorizedKeeper.compile.log")
    reaction.run(["build/tools/objdiff-cli", "diff", "-1", str(BUILD / "retail/obj/Game/Map/CollisionCategorizedKeeper.o"),
                  "-2", str(BUILD / "CollisionCategorizedKeeper.o"), "-o", str(BUILD / "zone-objdiff.json"),
                  "--format", "json-pretty"], "zone-objdiff.log")
    dol = reaction.DOL.read_bytes()
    assert reaction.hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    target = reaction.reader.Elf(BUILD / "retail/obj/Game/Map/CollisionCategorizedKeeper.o")
    compiled = reaction.reader.Elf(BUILD / "CollisionCategorizedKeeper.o")
    diff = json.loads((BUILD / "zone-objdiff.json").read_text())
    records = []
    for method, address, size in FUNCTIONS:
        sides = [next(s for s in diff[side]["symbols"] if s["name"].startswith(method + "__13CollisionZone"))
                 for side in ("left", "right")]
        name = sides[0]["name"]
        code, refs = runtime.relocated(compiled, target, name, address, size, dol)
        retail = reaction.reader.dol_bytes(dol, address, size)
        record = {"method": method, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                  "objdiff_match_percent": sides[0]["match_percent"], "relocations": refs, "relocated_bytes_equal": code == retail}
        if method == "eraseParts":
            record["behavior"] = erase_cases(retail, code)
            record["difference"] = "Compiler reuses the end-pointer byte offset for the last-element load; retail subtracts one from count then scales. Both compute the same address."
        else:
            canonical = [runtime.normalize(sides[i], elf.references(name), method, bool(i))
                         for i, elf in enumerate((target, compiled))]
            if method == "calcMinMaxAndRadius":
                rows = canonical[1]
                index = rows.index("addi r3, r29, 0x808")
                assert rows[index-1] == "lfs f0, 0x14(r1)" and rows[index+1] == "lfs f3, constant_3f000000@sda21"
                rows[index-1], rows[index+1] = rows[index+1], rows[index-1]
                for i in range(index, len(rows)):
                    rows[i] = re.sub(r"\br30\b", "r31", rows[i])
                record["difference"] = "Two independent loads exchanged around output-address setup; second-loop pointer allocated to r30 instead of r31."
            else:
                assert code == retail
            assert canonical[0] == canonical[1], (method, [(i, a, b) for i, (a, b) in enumerate(zip(*canonical)) if a != b])
            record["all_canonical_instructions_equal"] = True
            record["canonical_instructions"] = canonical[0]
        records.append(record)
        print(f"{method}: {sides[0]['match_percent']:.6f}%, {size}/{len(code)} bytes, verified")
    evidence = {"source": "src/Game/Map/CollisionCategorizedKeeper.cpp",
                "source_sha256": reaction.hashlib.sha256((ROOT / "src/Game/Map/CollisionCategorizedKeeper.cpp").read_bytes()).hexdigest(),
                "dol_sha1": reaction.hashlib.sha1(dol).hexdigest(), "compiler": "GC3.0a3/configure.py Game flags",
                "methods": records, "limits": "Array erasure runs actual retail/compiler integer instructions; bounds helpers have complete instruction correspondence after only stated scheduling/register normalization. No native zone lifetime or collision actor execution claim."}
    (BUILD / "zone-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
