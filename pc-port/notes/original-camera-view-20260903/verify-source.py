#!/usr/bin/env python3
"""Verify original view-interpolator imports, math bodies, and retail evidence."""

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


def dol_bytes(data, address, size):
    for section in range(18):
        offset, base, length = [struct.unpack_from(">I", data, field + section * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            start = offset + address - base
            return data[start:start + size]
    raise AssertionError(f"Missing DOL range at {address:#x}")


note = Path(__file__).resolve().parent
root = note.parents[2]
manifest = json.loads((note / "source-correspondence.json").read_text())
for record in manifest["files"]:
    original = (root / record["source"]).read_bytes()
    assert original == (root / record["destination"]).read_bytes(), record["destination"]
    assert hashlib.sha256(original).hexdigest() == record["sha256"], record["source"]
for record in manifest["functions"]:
    original = function_body((root / record["source"]).read_text(), record["signature"])
    assert hashlib.sha256(original.encode()).hexdigest() == record["sha256"], record["signature"]
    adapted = original
    for before, after in record.get("replacements", []):
        assert adapted.count(before) == 1
        adapted = adapted.replace(before, after)
    imported = function_body((root / record["destination"]).read_text(), record["signature"])
    assert adapted == imported, record["signature"]
print("Verified three exact source files and three mapped original inline math helpers.")

record = manifest["dol"]
path = root / record["path"]
if path.exists():
    dol = path.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == record["sha1"]
    for function in record["functions"]:
        data = dol_bytes(dol, int(function["address"], 0), int(function["size"], 0))
        assert hashlib.sha256(data).hexdigest() == function["sha256"], function["name"]
    for constant in record["constants"]:
        data = dol_bytes(dol, int(constant["address"], 0), 4)
        assert data.hex() == constant["bytes"]
        assert struct.unpack(">f", data)[0] == constant["value"]
    print("Verified three original instruction ranges and three getQuat constants.")
else:
    print("Retail DOL absent; skipped optional instruction-range verification.")
