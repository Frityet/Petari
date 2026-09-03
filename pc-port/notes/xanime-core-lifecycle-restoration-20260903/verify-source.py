#!/usr/bin/env python3
"""Check current source and the historical freezeCopy provenance."""

import hashlib
import json
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[3]
NOTE = Path(__file__).resolve().parent


def sha(data):
    return hashlib.sha256(data).hexdigest()


def body(source, key):
    start = source.index(key)
    position = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[position] == "{") - (source[position] == "}")
        position += 1
    return source[start:position].encode()


def main():
    data = json.loads((NOTE / "source-correspondence.json").read_text())
    for file in data["files"]:
        assert sha((ROOT / file["path"]).read_bytes()) == file["sha256"], file["path"]
    for method in data["methods"]:
        source = (ROOT / method["source"]).read_text()
        assert sha(body(source, method["body_key"])) == method["body_sha256"], method["symbol"]
    old = subprocess.check_output(["git", "show", data["historical_freeze_copy_commit"] + ":src/Game/Animation/XanimeCore.cpp"], cwd=ROOT)
    assert sha(old) == data["historical_source_sha256"]
    assert sha(body(old.decode(), "void XanimeCore::freezeCopy(")) == data["historical_freeze_copy_body_sha256"]
    source = (ROOT / "src/Game/Animation/XanimeCore.cpp").read_text()
    assert "pOther->mJointList[jointIndex]._0 = mJointList[jointIndex]._28;" in source
    evidence = json.loads((NOTE / "compiler-evidence.json").read_text())
    assert evidence["source_sha256"] == data["files"][0]["sha256"]
    assert evidence["header_sha256"] == data["files"][1]["sha256"]
    print("Nine root method declarations/bodies, historical freezeCopy, and compiler evidence hashes agree.")


if __name__ == "__main__":
    main()
