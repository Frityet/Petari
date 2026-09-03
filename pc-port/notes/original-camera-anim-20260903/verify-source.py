#!/usr/bin/env python3
"""Verify original CameraAnim mirrors and the shared Hermite scalar body."""

import hashlib
import json
from pathlib import Path


note = Path(__file__).resolve().parent
root = note.parents[2]
manifest = json.loads((note / "source-correspondence.json").read_text())
for record in manifest["files"]:
    original = (root / record["source"]).read_bytes()
    imported = (root / record["destination"]).read_bytes()
    assert original == imported, record["destination"]
    assert hashlib.sha256(original).hexdigest() == record["sha256"], record["source"]

record = manifest["hermite"]
original = (root / record["source"]).read_text()
start = original.index(record["start_marker"])
end = original.index(record["end_marker"], start) + len(record["end_marker"])
body = original[start:end]
assert body in (root / record["destination"]).read_text()
assert hashlib.sha256(body.encode()).hexdigest() == record["body_sha256"]
print("Verified four original CameraAnim mirrors and the shared Hermite fallback.")
