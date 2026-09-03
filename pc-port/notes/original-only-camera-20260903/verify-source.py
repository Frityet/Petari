#!/usr/bin/env python3
"""Verify exact OnlyCamera imports and recorded retail instruction ranges."""

import hashlib
import json
from pathlib import Path
import struct


note = Path(__file__).resolve().parent
root = note.parents[2]
manifest = json.loads((note / "source-correspondence.json").read_text())
for record in manifest["files"]:
    source = (root / record["source"]).read_bytes()
    assert source == (root / record["destination"]).read_bytes(), record["destination"]
    assert hashlib.sha256(source).hexdigest() == record["sha256"], record["source"]
print("Verified both exact OnlyCamera source/header mirrors.")

evidence = json.loads((note / "compiler-evidence.json").read_text())
assert evidence["source_sha256"] == manifest["files"][0]["sha256"]
assert evidence["header_sha256"] == manifest["files"][1]["sha256"]
assert all(record["relocated_bytes_equal"] for record in evidence["functions_and_vtable"])

path = root / manifest["dol"]["path"]
if not path.exists():
    print("Retail DOL absent; optional instruction-range checks skipped.")
else:
    dol = path.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == manifest["dol"]["sha1"]
    for record in evidence["functions_and_vtable"]:
        address, size = int(record["address"], 0), int(record["size"], 0)
        for index in range(18):
            offset, base, length = [struct.unpack_from(">I", dol, field + index * 4)[0] for field in (0, 0x48, 0x90)]
            if base <= address and address + size <= base + length:
                start = offset + address - base
                assert hashlib.sha256(dol[start:start + size]).hexdigest() == record["sha256"], record["name"]
                break
        else:
            raise AssertionError(record["name"])
    print("Verified all six retail method ranges and the complete vtable.")
