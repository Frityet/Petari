#!/usr/bin/env python3
"""Verify FixedPoint mirrors, original accessor bodies, and optional DOL slices."""

import hashlib
import json
from pathlib import Path
import struct


def function_body(text, signature):
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        depth += (text[index] == "{") - (text[index] == "}")
        if depth == 0:
            return text[start:index + 1]
    raise AssertionError(signature)


note = Path(__file__).resolve().parent
root = note.parents[2]
manifest = json.loads((note / "source-correspondence.json").read_text())
for record in manifest["files"]:
    original = (root / record["source"]).read_bytes()
    assert original == (root / record["destination"]).read_bytes(), record["destination"]
    assert hashlib.sha256(original).hexdigest() == record["sha256"], record["source"]

for record in manifest["actor_accessors"]:
    original = function_body((root / record["source"]).read_text(), record["signature"])
    imported = function_body((root / record["destination"]).read_text(), record["signature"])
    assert original == imported, record["signature"]
    assert hashlib.sha256(original.encode()).hexdigest() == record["sha256"], record["signature"]

for record in manifest["player_util_contracts"]:
    original = function_body((root / record["source"]).read_text(), record["signature"])
    assert hashlib.sha256(original.encode()).hexdigest() == record["sha256"], record["signature"]

print("Verified four FixedPoint mirrors and six original accessor source contracts.")
dol_record = manifest["dol"]
dol_path = root / dol_record["path"]
if dol_path.exists():
    dol = dol_path.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == dol_record["sha1"]
    for record in dol_record["functions"]:
        address = int(record["address"], 0)
        size = int(record["size"], 0)
        for section in range(18):
            offset = struct.unpack_from(">I", dol, section * 4)[0]
            section_address = struct.unpack_from(">I", dol, 0x48 + section * 4)[0]
            section_size = struct.unpack_from(">I", dol, 0x90 + section * 4)[0]
            if section_address <= address and address + size <= section_address + section_size:
                start = offset + address - section_address
                body = dol[start:start + size]
                assert hashlib.sha256(body).hexdigest() == record["sha256"], record["name"]
                break
        else:
            raise AssertionError(record["name"])
    print("Verified six recorded accessor instruction ranges against the RMGK01 DOL.")
else:
    print("Retail DOL is absent; skipped optional instruction-range hash verification.")
