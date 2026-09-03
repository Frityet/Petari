#!/usr/bin/env python3
"""Check literal original J3DSys construction/table copies and reviewed hashes."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent


def body(text, key):
    start = text.index(key)
    end = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def main():
    evidence = json.loads((HERE / "sys-compiler-evidence.json").read_text())
    native = (ROOT / "pc-port/src/compat/J3DSysCompat.cpp").read_text()
    for function in evidence["functions"]:
        original = (ROOT / function["source"]).read_text()
        original_body = body(original, function["source_key"])
        assert original_body == body(native, function["source_key"])
        digest = hashlib.sha256(original_body.encode()).hexdigest()
        assert digest == function["source_body_sha256"]
        print(function["symbol"], digest)
    for declaration in (
        "J3DSys j3dSys;", "J3DTexCoordScaleInfo J3DSys::sTexCoordScaleTable[8];",
        "u32 j3dDefaultViewNo;", "static u8 j3dTexCoordTable[7623];",
        "u8 j3dTevSwapTableTable[1024];", "u8 j3dAlphaCmpTable[768];", "u8 j3dZModeTable[96];",
    ):
        assert declaration in native
    for name in ("mCurrentMtx", "mCurrentS", "mParentS"):
        assert "J3DSys::" + name not in native
    print("Complete original constructor/table bodies and global definitions agree.")


if __name__ == "__main__":
    main()
