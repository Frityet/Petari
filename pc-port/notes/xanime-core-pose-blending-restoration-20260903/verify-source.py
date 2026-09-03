#!/usr/bin/env python3
"""Check the root source checkpoint and recorded current-retail compiler evidence."""

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
    evidence = json.loads((NOTE / "compiler-evidence.json").read_text())
    for file in data["files"]:
        assert sha((ROOT / file["path"]).read_bytes()) == file["sha256"], file["path"]
        old = subprocess.check_output(["git", "show", data["baseline_commit"] + ":" + file["path"]], cwd=ROOT)
        assert sha(old) == file["baseline_sha256"], file["path"]
    for method, compiled in zip(data["methods"], evidence["functions"]):
        source = (ROOT / method["source"]).read_text()
        assert sha(body(source, method["body_key"])) == method["body_sha256"]
        assert method["symbol"] == compiled["symbol"]
        assert method["retail_function_sha256"] == compiled["retail_function_sha256"]
        assert method["objdiff_match_percent"] == compiled["objdiff_match_percent"]
    assert evidence["source_sha256"] == data["files"][0]["sha256"]
    assert evidence["header_sha256"] == data["files"][1]["sha256"]
    assert sum(f["identical_normalized_event_count"] for f in evidence["functions"]) == 171
    print("Three root pose methods, baseline hashes, and current-retail compiler evidence agree.")


if __name__ == "__main__":
    main()
