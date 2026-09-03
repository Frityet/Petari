#!/usr/bin/env python3
"""Verify the documented root restoration and historical body provenance."""

import hashlib
import json
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[3]
NOTE = Path(__file__).resolve().parent


def sha(data):
    return hashlib.sha256(data).hexdigest()


def body(source, name):
    start = source.index("void Mario::" + name + "() {")
    position = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[position] == "{") - (source[position] == "}")
        position += 1
    return source[start:position].encode()


def main():
    data = json.loads((NOTE / "source-correspondence.json").read_text())
    for file in data["source_files"]:
        assert sha((ROOT / file["path"]).read_bytes()) == file["sha256"], file["path"]
    historical = subprocess.check_output(["git", "show", data["historical_commit"] + ":src/Game/Player/Mario.cpp"], cwd=ROOT)
    assert sha(historical) == data["historical_source_sha256"]
    current = (ROOT / "src/Game/Player/Mario.cpp").read_text()
    for function in data["functions"]:
        name = function["method"][len("Mario::"):-2]
        assert sha(body(current, name)) == function["source_body_sha256"], name
        assert sha(body(historical.decode(), name)) == function["historical_body_sha256"], name
    assert "mMovementStates._1 = checkGround();" in current
    assert "vertical >= 5.0f && floorDelta.dot(mFrontVec) > 0.0f" in current
    assert "if (!(__fabsf(gravityDot) < 30.0f))" in current
    evidence = json.loads((NOTE / "compiler-evidence.json").read_text())
    assert evidence["source_sha256"] == data["source_files"][0]["sha256"]
    assert evidence["header_sha256"] == data["source_files"][1]["sha256"]
    print("Root source/header hashes, six historical/current bodies, and recorded compiler evidence agree.")


if __name__ == "__main__":
    main()
