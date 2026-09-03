#!/usr/bin/env python3
"""Reproduce the compile-only jump foundation and bounded retail evidence."""

import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / "build/original-mario-jump-20260903"
DOL = ROOT / "build/compat-math-oracle/main.dol"
SELECTED = {
    "MarioJump": ["startTornadoCentering", "taskOnTornadoCentering", "tryJump", "initJumpParam", "procJump", "doAirWalk", "doLanding"],
    "MarioCollision": ["updateCubeCode", "updateBinderInfo"],
    "MarioSlope": ["lockGroundCheck", "unlockGroundCheck"],
    "MarioBee": ["entryWallWalkMode"],
    "MarioWall": ["start__9MarioWall"],
    "MarioHang": ["fixHangDir"],
    "MarioSpin": ["taskOnRotation"],
    "Mario": ["fixFrontVecByGravity", "setFrontVecKeepSide"],
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    subprocess.run([sys.executable, str(NOTES / "probe-source.py"), "--syntax", "--original"], cwd=ROOT, check=True)
    probes = json.loads((BUILD / "source-probe.json").read_text())
    assert all(u["syntax_exit"] == u["original_exit"] == 0 for u in probes["units"])
    spec = importlib.util.spec_from_file_location("original_mario_evidence", ROOT / "pc-port/notes/mario-update-restoration-20260903/verify-object.py")
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    dol = DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"
    instructions = {
        0x802E359C: (0x93FE0A38, "startTornadoCentering stores retained sensor in Mario+0xA38"),
        0x802E35E4: (0x809F0A38, "taskOnTornadoCentering loads that retained sensor"),
        0x80306370: (0x90830574, "lockGroundCheck stores its void-pointer argument at Mario+0x574"),
        0x80306388: (0x80030574, "unlockGroundCheck loads retained owner for comparison"),
        0x8030639C: (0x90830574, "unlockGroundCheck clears retained owner"),
        0x802D5020: (0x939E0568, "updateCubeCode stores selected area pointer at Mario+0x568"),
        0x802DB0DC: (0x880309F1, "entryWallWalkMode reads existing mBeeWallWalk byte"),
        0x802DB104: (0x9BE309F1, "entryWallWalkMode writes existing mBeeWallWalk byte"),
        0x802F6750: (0x80030024, "Wall start loads previous movement second word"),
        0x802F6754: (0x53E0BA10, "Wall start clears bit 0x800000"),
        0x802F6758: (0x90030024, "Wall start stores previous movement second word"),
    }
    witnesses = []
    for address, (word, meaning) in instructions.items():
        actual = struct.unpack(">I", helper.dol_bytes(dol, address, 4))[0]
        assert actual == word
        witnesses.append({"address": hex(address), "word": hex(word), "meaning": meaning})
    comparisons = []
    for unit, names in SELECTED.items():
        target = ROOT / "build/mario-update-restoration-20260903/retail/obj/Game/Player" / (unit + ".o")
        compiled = BUILD / (unit + ".original.o")
        output = BUILD / (unit + ".diff.json")
        subprocess.run([str(ROOT / "build/tools/objdiff-cli"), "diff", "-1", str(target), "-2", str(compiled),
                        "-o", str(output), "--format", "json-pretty"], check=True, capture_output=True)
        diff = json.loads(output.read_text())
        right = {s["name"]: s for s in diff["right"]["symbols"]}
        for name in names:
            prefix = name if "__" in name else name + "__"
            matches = [s for s in diff["left"]["symbols"] if s["name"].startswith(prefix)]
            assert len(matches) == 1, (unit, name)
            left = matches[0]
            comparisons.append({"unit": unit, "symbol": left["name"], "objdiff_match_percent": left.get("match_percent"),
                                "retail_size": int(left["size"]), "compiled_size": int(right[left["name"]]["size"]),
                                "retail_object_sha256": sha(target), "compiled_object_sha256": sha(compiled)})
    for header in (ROOT / "include/Game/Player/Mario.hpp", ROOT / "pc-port/src/Game/Player/Mario.hpp"):
        text = header.read_text()
        for declaration in ("AreaObj* _568;", "void* _574;", "HitSensor* _A38;"):
            assert declaration in text
    report = {"scope": "Compilation foundation only; fuzzy scores are not runtime or complete semantic proof",
              "dol_sha1": hashlib.sha1(dol).hexdigest(), "compiler": "GC3.0a3, configured cflags_game, sjiswrap",
              "root_mario_header_sha256": sha(ROOT / "include/Game/Player/Mario.hpp"),
              "native_mario_header_sha256": sha(ROOT / "pc-port/src/Game/Player/Mario.hpp"),
              "probes": probes, "retail_instruction_witnesses": witnesses, "comparisons": comparisons}
    (NOTES / "source-evidence.json").write_text(json.dumps(report, indent=2) + "\n")
    print("13 original and 13 native syntax compilations pass; retail witnesses checked.")


if __name__ == "__main__":
    main()
