#!/usr/bin/env python3
"""Check the imported method bodies against their recorded original source."""

import hashlib
import json
from pathlib import Path


def method(source, signature):
    position = source.index(signature)
    start = source.rfind("\n", 0, position) + 1
    end = source.index("{", position) + 1
    depth = 1
    while depth:
        if source[end] == "{":
            depth += 1
        elif source[end] == "}":
            depth -= 1
        end += 1
    return source[start:end]


note = Path(__file__).resolve().parent
root = note.parents[2]
manifest = json.loads((note / "source-correspondence.json").read_text())
for name, expected in manifest["root_source_sha256"].items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name

for record in manifest["methods"]:
    original = method((root / record["source"]).read_text(), record["signature"])
    imported = method((root / record["destination"]).read_text(), record["signature"])
    if "source_body_sha256" in record:
        assert hashlib.sha256(original.encode()).hexdigest() == record["source_body_sha256"], record["signature"]
    for replacement in record.get("replacements", []):
        assert original.count(replacement["before"]) == 1, record["signature"]
        original = original.replace(replacement["before"], replacement["after"])
    assert original == imported, record["signature"]
    assert hashlib.sha256(imported.encode()).hexdigest() == record["body_sha256"], record["signature"]

for record in manifest.get("data_blocks", []):
    original = (root / record["source"]).read_text()
    start = original.index(record["start_marker"])
    end = original.index(record["end_marker"], start) + len(record["end_marker"])
    block = original[start:end]
    assert block in (root / record["destination"]).read_text(), record["start_marker"]
    assert hashlib.sha256(block.encode()).hexdigest() == record["body_sha256"], record["start_marker"]

adapted = sum(bool(record.get("replacements")) for record in manifest["methods"])
print(f"Verified {len(manifest['methods'])} source-backed methods ({adapted} documented adaptations).")
