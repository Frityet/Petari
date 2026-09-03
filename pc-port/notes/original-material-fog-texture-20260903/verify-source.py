#!/usr/bin/env python3
"""Compile original material fog/texture controllers and compare every retail byte."""
import importlib.util
import json
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("controllers", HERE.parent / "original-material-controllers-20260903/verify-source.py")
controllers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(controllers)
controllers.HERE = HERE
controllers.BUILD = controllers.ROOT / "build/original-material-fog-texture-20260903"
controllers.FUNCTIONS = (
    "__ct__7FogCtrlFP12J3DModelDatab", "update__7FogCtrlFv",
    "__ct__10TexMtxCtrlFP12J3DModelDataPCc", "setTexMtx__10TexMtxCtrlFUlP9J3DTexMtx",
    "updateMaterial__10TexMtxCtrlFP11J3DMaterial",
)
controllers.NEW = set(controllers.FUNCTIONS)

def canonicalize(sides, objects, name):
    normalized = [controllers.proof.normalize(side, controllers.refs(obj, name), "material", bool(i))
                  for i, (side, obj) in enumerate(zip(sides, objects))]
    assert normalized[0] == normalized[1], name
    return normalized[0], []

def verify_vtables(compiled, dol, symbols):
    records = []
    for cls, slots in (
        ("7FogCtrl", ["update__7FogCtrlFv", "updateMaterial__12MaterialCtrlFP11J3DMaterial"]),
        ("10TexMtxCtrl", ["update__12MaterialCtrlFv", "updateMaterial__10TexMtxCtrlFP11J3DMaterial"]),
    ):
        name = "__vt__" + cls
        address, size = symbols[name]
        assert size == 16
        raw = controllers.reader.dol_bytes(dol, address, size)
        assert raw[:8] == bytes(8)
        assert list(struct.unpack(">II", raw[8:])) == [symbols[s][0] for s in slots]
        refs = sorted(compiled.references(name), key=lambda ref: int(ref["offset"], 16))
        assert [(r["offset"], r["kind"], r["symbol"]) for r in refs] == [
            ("0x8", 1, slots[0]), ("0xc", 1, slots[1])]
        records.append({"name": name, "retail_address": hex(address), "size": size, "slots": slots})
    return records

controllers.canonicalize = canonicalize
controllers.verify_vtables = verify_vtables
if __name__ == "__main__":
    controllers.main()
    evidence = HERE / "source-evidence.json"
    report = json.loads(evidence.read_text())
    report["scope"] = "Root fog and texture-controller source recovery only; every relocated instruction byte and both vtables verified. No native activation."
    evidence.write_text(json.dumps(report, indent=2) + "\n")
