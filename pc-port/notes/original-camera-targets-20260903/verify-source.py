#!/usr/bin/env python3
"""Check original target mirrors, extracted methods, and rotation evidence."""

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

for record in manifest["functions"]:
    original = function_body((root / record["source"]).read_text(), record["signature"])
    imported = function_body((root / record["destination"]).read_text(), record["signature"])
    assert original == imported, record["signature"]
    assert hashlib.sha256(original.encode()).hexdigest() == record["sha256"], record["signature"]

print("Verified two matrix target files and fifteen exact source function bodies.")
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
                assert hashlib.sha256(dol[start:start + size]).hexdigest() == record["sha256"], record["name"]
                break
        else:
            raise AssertionError(record["name"])
    print("Verified both original rotation instruction ranges against the RMGK01 DOL.")
else:
    print("Retail DOL absent; skipped optional instruction-range hash verification.")
