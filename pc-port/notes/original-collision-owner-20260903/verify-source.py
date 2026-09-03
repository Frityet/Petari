#!/usr/bin/env python3
"""Compile the root collision lifecycle group and verify its retail instructions."""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-collision-owner-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
spec = importlib.util.spec_from_file_location("proof", HERE.parent / "original-binder-reaction-20260903/verify-runtime.py")
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)
reader = proof.reaction.reader

FUNCTIONS = {
    "CollisionCategorizedKeeper": (
        ("__ct__26CollisionCategorizedKeeperFl", 0x80173B6C, 0x98),
        ("movement__26CollisionCategorizedKeeperFv", 0x80173C04, 0xE0),
        ("getStrikeInfo__26CollisionCategorizedKeeperFUl", 0x80174B44, 0x10),
        ("getZone__26CollisionCategorizedKeeperFi", 0x80174B54, 0xA0),
        ("calcMinMaxAndRadiusIfMoveOuter__13CollisionZoneFP14CollisionParts", 0x80174E54, 0xEC),
        ("__dt__26CollisionCategorizedKeeperFv", 0x80175090, 0x58),
    ),
    "KCollision": (
        ("setData__16KCollisionServerFPv", 0x8018318C, 0x7C),
        ("isBinaryInitialized__16KCollisionServerFPCv", 0x80183390, 0xC),
    ),
}


def run(command, log):
    result = subprocess.run([str(item) for item in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def function_body(text, prefix):
    start = text.index(prefix)
    position = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[position] == "{") - (text[position] == "}")
        position += 1
    return text[start:position]


def main():
    BUILD.mkdir(exist_ok=True, parents=True)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    for node in ast.parse((ROOT / "configure.py").read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == "cflags_game" for target in node.targets):
            flags = eval(compile(ast.Expression(node.value), "configure.py", "eval"),
                         {"config": types.SimpleNamespace(version="RMGK01"), "version_num": 0})
            break
    else:
        raise AssertionError("Game compiler flags not found")
    base = ["build/tools/wibo", "build/tools/sjiswrap.exe", "build/compilers/GC/3.0a3/mwcceppc.exe"]
    for flag in flags:
        base.extend(shlex.split(flag))
    config = (ROOT / "config/RMGK01/config.yml").read_text()
    config = config.replace("object_base: orig/RMGK01", "object_base: " + str(DOL.parent))
    config = config.replace("object: sys/main.dol", "object: " + DOL.name)
    config = config.replace("symbols: config/", "symbols: " + str(ROOT / "config") + "/")
    config = config.replace("splits: config/", "splits: " + str(ROOT / "config") + "/")
    (BUILD / "config.yml").write_text(config)
    run(["build/tools/dtk", "dol", "split", "--no-update", "-j", "2", BUILD / "config.yml", BUILD / "retail"], "dtk.log")
    records = []
    for unit, functions in FUNCTIONS.items():
        command = base + ["-c", f"src/Game/Map/{unit}.cpp", "-o", str(BUILD / f"{unit}.o")]
        (BUILD / f"{unit}.command.json").write_text(json.dumps(command, indent=2) + "\n")
        run(command, f"{unit}.compile.log")
        target_path = BUILD / f"retail/obj/Game/Map/{unit}.o"
        run(["build/tools/objdiff-cli", "diff", "-1", target_path, "-2", BUILD / f"{unit}.o",
             "-o", BUILD / f"{unit}.diff.json", "--format", "json-pretty"], f"{unit}.objdiff.log")
        target, compiled = reader.Elf(target_path), reader.Elf(BUILD / f"{unit}.o")
        diff = json.loads((BUILD / f"{unit}.diff.json").read_text())
        for name, address, size in functions:
            sides = [next(symbol for symbol in diff[side]["symbols"] if symbol["name"] == name) for side in ("left", "right")]
            for side in sides:
                for row in side["instructions"]:
                    if "instruction" in row:
                        row["instruction"].setdefault("address", "0")
            canonical = [proof.normalize(sides[i], obj.references(name), "lifecycle", bool(i))
                         for i, obj in enumerate((target, compiled))]
            if name.startswith("movement__"):
                registers = {26: 27, 27: 28, 28: 29, 29: 26}
                canonical[0] = [re.sub(r"\br(26|27|28|29)\b", lambda match: "r" + str(registers[int(match[1])]), line)
                                for line in canonical[0]]
            assert canonical[0] == canonical[1], (name, [(a, b) for a, b in zip(*canonical) if a != b])
            code, relocations = proof.relocated(compiled, target, name, address, size, dol)
            retail = reader.dol_bytes(dol, address, size)
            if not name.startswith("movement__"):
                assert code == retail, name
            records.append({"name": name, "address": hex(address), "retail_size": size, "compiled_size": len(code),
                            "objdiff_match_percent": sides[0]["match_percent"], "relocated_bytes_equal": code == retail,
                            "all_canonical_instructions_equal": True, "relocations": relocations,
                            "canonical_instructions": canonical[0]})
            print(name, sides[0]["match_percent"], "verified")

    original = (ROOT / "src/Game/Map/KCollision.cpp").read_text()
    native = (ROOT / "pc-port/src/compat/OriginalKCollisionCompat.cpp").read_text()
    prefixes = ["Fxyz& Fxyz::operator=", "KCollisionServer::KCollisionServer()", "void KCollisionServer::init(",
                "s32 KCollisionServer::toIndex(", "TVec3f* KCollisionServer::getFaceNormal(",
                "TVec3f* KCollisionServer::getEdgeNormal1(", "TVec3f* KCollisionServer::getEdgeNormal2(",
                "TVec3f* KCollisionServer::getEdgeNormal3(", "TVec3f* KCollisionServer::getNormal(",
                "void KCollisionServer::calXvec(", "TVec3f KCollisionServer::getPos(",
                "KC_PrismData* KCollisionServer::getPrismData(", "s32 KCollisionServer::getTriangleNum(",
                "JMapInfoIter KCollisionServer::getAttributes(", "s32* KCollisionServer::searchBlock("]
    for prefix in prefixes:
        assert function_body(original, prefix) == function_body(native, prefix), prefix
    assert (ROOT / "include/Game/Map/KCollision.hpp").read_bytes() == (ROOT / "pc-port/src/Game/Map/KCollision.hpp").read_bytes()
    sources = ["src/Game/Map/CollisionCategorizedKeeper.cpp", "include/Game/Map/CollisionCategorizedKeeper.hpp",
               "src/Game/Map/KCollision.cpp", "include/Game/Map/KCollision.hpp",
               "pc-port/src/compat/OriginalKCollisionCompat.cpp", "pc-port/src/resource/KCollisionResource.cpp",
               "pc-port/src/resource/JMapResource.cpp", "pc-port/src/Game/Util/JMapInfo.cpp"]
    evidence = {"compiler": "GC3.0a3 with configure.py Game flags and Shift-JIS wrapper", "dol_sha1": hashlib.sha1(dol).hexdigest(),
                "functions": records, "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in sources},
                "native_exact_body_count": len(prefixes), "native_header_exact": True,
                "normalization": "movement only: retail r26/r27/r28/r29 become compiled r27/r28/r29/r26. Calls, branch destinations, fields, operations and all other registers are unchanged.",
                "scope": "Root lifecycle/source verification and independent decoded server foundation; no placed CollisionParts or production query activation."}
    (HERE / "source-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")


if __name__ == "__main__":
    main()
