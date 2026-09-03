#!/usr/bin/env python3
"""Verify the complete recovered root Binder group with the original compiler.

The only instruction normalization beyond actual relocation verification is
listed per method below. Operands, offsets, call order and branch destinations
otherwise remain intact. This script also runs verify.py's numeric oracle.
"""
import importlib.util
import json
from pathlib import Path
import re
import struct

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("reaction", HERE / "verify.py")
reaction = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reaction)
ROOT, BUILD = reaction.ROOT, reaction.BUILD
FUNCTIONS = (
    ("copyPlaneArrayAndSortingSensor", 0x8015D718, 0x128),
    ("compSensor", 0x8015D840, 0x18),
    ("bind", 0x8015D858, 0x4A4),
    ("moveAlongHittedPlanes", 0x8015DCFC, 0x104),
    ("findBindedPos", 0x8015DE00, 0x184),
    ("moveWithCollisionParts", 0x8015DF84, 0x94),
    ("storeCurrentHitInfo", 0x8015E018, 0xE4),
    ("obtainMomentFixReaction", 0x8015E1D4, 0x1DC),
    ("storeContactPlane", 0x8015E3B0, 0x100),
)


def relocated(elf, target, name, address, size, dol):
    symbols = {name: int(address, 16) for name, address in re.findall(
        r"^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);", (ROOT / "config/RMGK01/symbols.txt").read_text(), re.M)}
    retail = reaction.reader.dol_bytes(dol, address, size)
    target_refs = target.references(name)
    constants = {}
    for ref in target_refs:
        if "value_hex" not in ref:
            continue
        offset = int(ref["offset"], 16)
        if ref["kind"] == 109:
            word = struct.unpack_from(">I", retail, offset)[0]
            assert (word >> 16) & 31 == 2
            effective = 0x806BFC20 + reaction.signed(word & 65535, 16)
        elif ref["kind"] in (4, 6):
            pair = {r["kind"]: int(r["offset"], 16) for r in target_refs
                    if r["symbol"] == ref["symbol"] and r["addend"] == ref["addend"]}
            high = struct.unpack_from(">H", retail, pair[6])[0]
            low = struct.unpack_from(">h", retail, pair[4])[0]
            effective = ((high << 16) + low) & 0xFFFFFFFF
        else:
            raise AssertionError(ref)
        expected = bytes.fromhex(ref["value_hex"])
        assert reaction.reader.dol_bytes(dol, effective, len(expected)) == expected
        constants[ref["value_hex"]] = effective
    _, start, compiled_size, section = next(s for s in elf.symbols if s[0] == name)
    code = bytearray(elf.section_data(section)[start:start + compiled_size])
    records = []
    for ref in elf.references(name):
        offset, kind = int(ref["offset"], 16), ref["kind"]
        effective = constants[ref["value_hex"]] if "value_hex" in ref else symbols[ref["symbol"]] + ref["addend"]
        if kind in (4, 6):
            value = effective if kind == 4 else (effective + 0x8000) >> 16
            struct.pack_into(">H", code, offset, value & 65535)
        else:
            word = struct.unpack_from(">I", code, offset)[0]
            if kind == 10:
                word = (word & 0xFC000003) | ((effective - address - offset) & 0x3FFFFFC)
            elif kind == 109:
                word = (word & ~0x1FFFFF) | (2 << 16) | ((effective - 0x806BFC20) & 65535)
            else:
                raise AssertionError(ref)
            struct.pack_into(">I", code, offset, word)
        records.append({**ref, "effective_retail_target": hex(effective)})
    return bytes(code), records


def normalize(symbol, refs, method, compiled):
    rows = [[int(i["instruction"]["address"]), i["instruction"]["formatted"].strip()]
            for i in symbol["instructions"] if "instruction" in i]
    constants = {r["symbol"]: "constant_" + r["value_hex"] for r in refs if "value_hex" in r}
    for row in rows:
        for name, value in constants.items():
            row[1] = row[1].replace(name + "@", value + "@")
    if method == "bind":
        # Both sequences capture mask0x04 as bool and clear that bit before clear().
        end = next(i for i, row in enumerate(rows) if row[1] == "bl clear__6BinderFv")
        captured = [r[1] for r in rows[5:end]]
        expected = (["lbz r6, 0x1ec(r4)", "mr r29, r4", "mr r28, r3", "mr r30, r5",
                     "rlwinm r0, r6, 0, 30, 28", "mr r3, r29", "stb r0, 0x1ec(r4)", "extrwi r31, r6, 1, 29"]
                    if compiled else
                    ["lbz r0, 0x1ec(r4)", "mr r29, r4", "mr r28, r3", "mr r30, r5",
                     "extrwi r6, r0, 1, 29", "rlwinm r0, r0, 0, 30, 28", "subic r5, r6, 0x1",
                     "stb r0, 0x1ec(r4)", "mr r3, r29", "subfe r31, r5, r6"])
        assert captured == expected
        del rows[5:end]
        if compiled:
            candidates = [i for i, row in enumerate(rows) if row[1] == "cmpwi r0, 0x0"]
            # Three pointer null checks are also present; only the loop predicate
            # immediately follows the noMargin bne (its preceding cmp uses r31).
            index = next(i for i in candidates if rows[i-2][1] == "cmpwi r31, 0x0")
            assert rows[index+1][1].startswith("bne 0x")
            rows[index][1] = "cmplwi r0, 0x1"
            rows[index+1][1] = rows[index+1][1].replace("bne", "beq", 1)
    if method == "storeContactPlane" and compiled:
        for row in rows:
            row[1] = re.sub(r"\br(29|30)\b", lambda m: "r" + str(59 - int(m[1])), row[1])
    if method == "storeCurrentHitInfo":
        for row in rows:
            match = re.fullmatch(r"add r0, (r\d+), (r\d+)", row[1])
            if match:
                row[1] = "add r0, " + ", ".join(sorted(match.groups()))
    if method == "obtainMomentFixReaction":
        for index in range(len(rows)-1):
            pair = [rows[index][1], rows[index+1][1]]
            if set(pair) in ({"lfs f1, 0x7c(r30)", "lfs f0, 0x20(r1)"},
                             {"lfs f1, 0x80(r30)", "lfs f0, 0x24(r1)"},
                             {"lfs f1, 0x84(r30)", "lfs f0, 0x28(r1)"}):
                rows[index][1], rows[index+1][1] = sorted(pair)
    addresses = {row[0]: i for i, row in enumerate(rows)}
    for row in rows:
        match = re.fullmatch(r"(b\w*) (0x[0-9a-f]+)", row[1])
        if match:
            row[1] = match[1] + " instruction_" + str(addresses[int(match[2], 16)])
    return [row[1] for row in rows]


def main():
    reaction.main()
    dol = reaction.DOL.read_bytes()
    target = reaction.reader.Elf(BUILD / "retail/obj/Game/LiveActor/Binder.o")
    compiled = reaction.reader.Elf(BUILD / "Binder.o")
    diff = json.loads((BUILD / "objdiff.json").read_text())
    records = []
    for method, address, size in FUNCTIONS:
        sides = [next(s for s in diff[side]["symbols"] if s["name"].startswith(method + "__6Binder"))
                 for side in ("left", "right")]
        name = sides[0]["name"]
        assert int(sides[0]["size"]) == size
        refs = [elf.references(name) for elf in (target, compiled)]
        canonical = [normalize(sides[i], refs[i], method, bool(i)) for i in range(2)]
        assert canonical[0] == canonical[1], (method, [(i, a, b) for i, (a, b) in enumerate(zip(*canonical)) if a != b])
        code, relocations = relocated(compiled, target, name, address, size, dol)
        retail = reaction.reader.dol_bytes(dol, address, size)
        byte_exact = code == retail
        if method in ("copyPlaneArrayAndSortingSensor", "compSensor", "moveAlongHittedPlanes", "findBindedPos", "moveWithCollisionParts"):
            assert byte_exact, method
        records.append({"method": method, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                        "objdiff_match_percent": sides[0]["match_percent"], "relocated_bytes_equal": byte_exact,
                        "all_canonical_instructions_equal": True, "canonical_instruction_count": len(canonical[0]),
                        "relocations": relocations, "canonical_instructions": canonical[0]})
        print(f"{method}: {sides[0]['match_percent']:.6f}%, {size}/{len(code)} bytes, canonical instructions equal, actual relocated bytes equal={byte_exact}")
    evidence = {"compiler": "GC3.0a3 with unmodified configure.py Game flags", "source": "src/Game/LiveActor/Binder.cpp",
                "source_sha256": reaction.hashlib.sha256((ROOT / "src/Game/LiveActor/Binder.cpp").read_bytes()).hexdigest(),
                "header_sha256": reaction.hashlib.sha256((ROOT / "include/Game/LiveActor/Binder.hpp").read_bytes()).hexdigest(),
                "dol_sha1": reaction.hashlib.sha1(dol).hexdigest(), "functions": records,
                "normalization": {"bind": "Explicitly checked equivalent boolean capture; valid bool canMoveMore!=0 vs==1. All other instructions, operands, branches and calls equal.",
                                  "storeCurrentHitInfo": "Two commutative integer index addition operands reversed.",
                                  "obtainMomentFixReaction": "Three pairs of independent scalar loads reordered.",
                                  "storeContactPlane": "One bijective r29/r30 allocation exchange.",
                                  "others": "No instruction normalization beyond actual relocation resolution."},
                "limits": "Only the reaction has raw-instruction numerical cases here. Full Binder query/lifetime/native integration is a separate parent-owned test gate."}
    (BUILD / "runtime-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
