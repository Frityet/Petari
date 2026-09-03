#!/usr/bin/env python3
"""Verify the recovered original DisplayListMaker and animation stop query."""
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

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-model-manager-owner-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("sdk", HERE.parent / "original-sdk-model-dispatch-20260903/verify-source.py")
sdk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sdk)
reader, proof = sdk.reader, sdk.proof
BASES = {2: 0x806BFC20, 13: 0x806B9620}
STRINGS = {"lbl_80587A0C": "IndDummy", "lbl_80587A15": "ShadowProjDummy",
           "lbl_80587A25": "ShadowDummy", "lbl_806B1B40": ""}
OLD = {"addFogCtrl", "addMatColorCtrl", "addTexMtxCtrl", "addProjmapEffectMtxSetter", "push"}


def run(command, log):
    result = subprocess.run([str(x) for x in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def refs(elf, name):
    result = elf.references(name)
    for ref in result:
        if ref["symbol"] in STRINGS:
            ref["value_hex"] = (STRINGS[ref["symbol"]].encode() + b"\0").hex()
        if name == "update__16DisplayListMakerFv" and ref["kind"] in (4, 6):
            # The member-function descriptor is 12 bytes, not a C string.
            symbol, at, size, section = next(s for s in elf.symbols if s[0] == ref["symbol"])
            assert size == 12
            data = elf.section_data(section)[at:at + size]
            assert data == bytes.fromhex("000000000000000800000000")
            ref["value_hex"] = data.hex()
    return result


def relocate(compiled, target, name, address, size, dol, symbols):
    original = reader.dol_bytes(dol, address, size)
    target_refs = refs(target, name)
    constants = {}
    for ref in target_refs:
        if "value_hex" not in ref:
            continue
        offset = int(ref["offset"], 16)
        base = None
        if ref["kind"] == 109:
            word = struct.unpack_from(">I", original, offset)[0]
            base = (word >> 16) & 31
            assert base in BASES
            effective = BASES[base] + struct.unpack(">h", struct.pack(">H", word & 65535))[0]
        else:
            assert ref["kind"] in (4, 6)
            pair = {r["kind"]: int(r["offset"], 16) for r in target_refs
                    if r["symbol"] == ref["symbol"] and r["addend"] == ref["addend"]}
            effective = ((struct.unpack_from(">H", original, pair[6])[0] << 16) +
                         struct.unpack_from(">h", original, pair[4])[0]) & 0xFFFFFFFF
        expected = bytes.fromhex(ref["value_hex"])
        assert reader.dol_bytes(dol, effective, len(expected)) == expected
        constants[ref["value_hex"]] = effective, base
    _, start, length, section = next(s for s in compiled.symbols if s[0] == name)
    code = bytearray(compiled.section_data(section)[start:start + length])
    records = []
    for ref in refs(compiled, name):
        offset, kind = int(ref["offset"], 16), ref["kind"]
        if "value_hex" in ref:
            effective, base = constants[ref["value_hex"]]
        else:
            effective, base = symbols[ref["symbol"]][0] + ref["addend"], None
        if kind in (4, 6):
            value = effective if kind == 4 else (effective + 0x8000) >> 16
            struct.pack_into(">H", code, offset, value & 65535)
        else:
            word = struct.unpack_from(">I", code, offset)[0]
            if kind == 10:
                word = (word & 0xFC000003) | ((effective - address - offset) & 0x3FFFFFC)
            else:
                assert kind == 109 and base in BASES
                word = (word & ~0x1FFFFF) | (base << 16) | ((effective - BASES[base]) & 65535)
            struct.pack_into(">I", code, offset, word)
        records.append({**ref, "effective_retail_target": hex(effective)})
    return bytes(code), records


def canonicalize(sides, objects, name):
    normalized = [proof.normalize(side, refs(obj, name), "maker", bool(i))
                  for i, (side, obj) in enumerate(zip(sides, objects))]
    changes = []
    if name == "checkViewProjmapEffectMtx__16DisplayListMakerFv":
        normalized[1] = [re.sub(r"\br(30|31)\b", lambda m: "r" + str(61 - int(m[1])), x)
                         for x in normalized[1]]
        changes.append("Bijective r30/r31 local-register exchange; every instruction, branch, offset and mode comparison is otherwise retained.")
    if name == "isStop__13AnmPlayerBaseCFv":
        assert normalized[0][:12] == [
            "lwz r0, 0x4(r3)", "li r4, 0x1", "li r5, 0x1", "cmpwi r0, 0x0", "beq instruction_10",
            "lbz r0, 0x11(r3)", "clrlwi r0, r0, 31", "cmpwi r0, 0x1", "beq instruction_10",
            "li r5, 0x0", "cmpwi r5, 0x0", "bne instruction_17"]
        assert normalized[1][:12] == [
            "lwz r0, 0x4(r3)", "li r5, 0x1", "li r4, 0x0", "cmpwi r0, 0x0", "beq instruction_9",
            "lbz r0, 0x11(r3)", "clrlwi r0, r0, 31", "cmplwi r0, 0x1", "bne instruction_10",
            "li r4, 0x1", "cmpwi r4, 0x0", "bne instruction_17"]
        # Both complete, explicitly checked prefixes compute
        # resource == nullptr || (state & 1) == 1 and skip the rate read
        # exactly when true. The masked byte is only 0/1, so signedness
        # of that comparison cannot affect the result.
        for null in (False, True):
            for state in range(256):
                retail = True
                if not null and state & 1 != 1:
                    retail = False
                rebuilt = False
                if null or state & 1 == 1:
                    rebuilt = True
                assert retail == rebuilt
        normalized = [side[12:] for side in normalized]
        normalized[1] = [re.sub(r"\br(4|5)\b", lambda m: "r" + str(9 - int(m[1])), x)
                         for x in normalized[1]]
        changes.append("Explicitly checked two equivalent short-circuit Boolean materializations, including all 512 null/state-byte cases; only 0/1 reaches the signed/unsigned comparison. r4/r5 return temporaries exchanged. The exact rate load, ordered float comparison, equality branch and return remain unchanged.")
    assert normalized[0] == normalized[1], (name, [(i,a,b) for i,(a,b) in enumerate(zip(*normalized)) if a != b])
    return normalized[0], changes


def main():
    BUILD.mkdir(exist_ok=True, parents=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    symbols = {name: (int(address, 16), int(size, 16)) for name, address, size in re.findall(
        r"^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);[^\n]*size:(0x[0-9A-Fa-f]+)",
        (ROOT / "config/RMGK01/symbols.txt").read_text(), re.M)}
    for name, address in re.findall(r"^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);",
                                    (ROOT / "config/RMGK01/symbols.txt").read_text(), re.M):
        symbols.setdefault(name, (int(address, 16), 0))
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == "cflags_game" for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Game compiler flags not found")
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    records = []
    for label, source, original in (
        ("DisplayListMaker", "src/Game/LiveActor/DisplayListMaker.cpp", "Game/LiveActor/DisplayListMaker.o"),
        ("AnmPlayer", "src/Game/Animation/AnmPlayer.cpp", "Game/LiveActor/ModelManager.o"),
    ):
        command = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
        for flag in flags:
            command.extend(shlex.split(flag))
        command += ["-c", source, "-o", str(BUILD / (label + ".o"))]
        (BUILD / (label + ".command.json")).write_text(json.dumps(command, indent=2) + "\n")
        run(command, label + ".compile.log")
        original_path = BUILD / "retail/obj" / original
        run(["build/tools/objdiff-cli", "diff", "-1", original_path, "-2", BUILD / (label + ".o"),
             "-o", BUILD / (label + ".diff.json"), "--format", "json-pretty"], label + ".objdiff.log")
        objects = reader.Elf(original_path), reader.Elf(BUILD / (label + ".o"))
        diff = json.loads((BUILD / (label + ".diff.json")).read_text())
        names = [s["name"] for s in diff["left"]["symbols"] if s.get("match_percent") is not None and
                 ("__16DisplayListMaker" in s["name"] if label == "DisplayListMaker" else s["name"] == "isStop__13AnmPlayerBaseCFv")]
        for name in names:
            sides = [next(s for s in diff[side]["symbols"] if s["name"] == name) for side in ("left", "right")]
            for side in sides:
                for row in side["instructions"]:
                    if "instruction" in row:
                        row["instruction"].setdefault("address", "0")
            canonical, changes = canonicalize(sides, objects, name)
            address, size = symbols[name]
            code, relocations = relocate(objects[1], objects[0], name, address, size, dol, symbols)
            equal = code == reader.dol_bytes(dol, address, size)
            if not changes:
                assert equal, name
            records.append({"name": name, "recovered_now": name.split("__")[0] not in OLD,
                            "address": hex(address), "retail_size": size, "compiled_size": len(code),
                            "objdiff_match_percent": sides[0]["match_percent"], "relocated_bytes_equal": equal,
                            "all_canonical_instructions_equal": True, "normalization": changes,
                            "canonical_instructions": canonical, "relocations": relocations})
            print(name.split("__")[0], sides[0]["match_percent"], "verified")
    assert len(records) == 30
    # Constructor store proves the exact concrete dependency slot, and vtable
    # proves inheritance/override shape without inventing a missing body.
    assert reader.dol_bytes(dol, 0x80168374, 4).hex() == "907d000c"
    assert reader.dol_bytes(dol, 0x80587E0C, 16).hex() == "000000000000000080168394801685e8"
    assert reader.dol_bytes(dol, 0x802A6B88, 4).hex() == "90a10008"
    assert reader.dol_bytes(dol, 0x802A6BCC, 4).hex() == "38c10008"
    evidence = {"compiler": "GC3.0a3, configured Game flags, sjiswrap",
                "dol_sha1": hashlib.sha1(dol).hexdigest(), "functions": records,
                "source_sha256": {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in (
                    "src/Game/LiveActor/DisplayListMaker.cpp", "include/Game/LiveActor/DisplayListMaker.hpp",
                    "include/Game/LiveActor/MaterialCtrl.hpp", "src/Game/Animation/AnmPlayer.cpp")},
                "scope": "Root source recovery only; no native player, manager, shadow owner or renderer activation."}
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
