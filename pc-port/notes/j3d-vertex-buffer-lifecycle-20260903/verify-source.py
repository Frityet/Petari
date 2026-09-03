#!/usr/bin/env python3
"""Check the literal root/native correspondence and single native ownership."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
NOTE = Path(__file__).resolve().parent
FUNCTIONS = {
    "void J3DVertexBuffer::setVertexData(": ("src/JSystem/J3DGraphBase/J3DVertex.cpp", "pc-port/src/compat/J3DVertexBufferCompat.cpp"),
    "void J3DVertexBuffer::init(": ("src/JSystem/J3DGraphBase/J3DVertex.cpp", "pc-port/src/compat/J3DVertexBufferCompat.cpp"),
    "J3DVertexBuffer::~J3DVertexBuffer(": ("src/JSystem/J3DGraphBase/J3DVertex.cpp", "pc-port/src/compat/J3DVertexBufferCompat.cpp"),
    "XjointTransform* XanimeCore::getJointTransform(": ("src/Game/Animation/XanimeCore.cpp", "pc-port/src/Game/Animation/XanimeCore.cpp"),
}
HEADERS = (
    ("libs/JSystem/include/JSystem/J3DGraphBase/J3DVertex.hpp", "pc-port/src/JSystem/J3DGraphBase/J3DVertex.hpp"),
    ("include/Game/Animation/XanimeCore.hpp", "pc-port/src/Game/Animation/XanimeCore.hpp"),
)


def function(text, signature):
    start = text.index(signature)
    end = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def main():
    evidence = json.loads((NOTE / "compiler-evidence.json").read_text())
    for path, expected in evidence["source_sha256"].items():
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == expected, (path, "changed since compiler proof")
    rows = []
    for signature, paths in FUNCTIONS.items():
        texts = [(ROOT / path).read_text() for path in paths]
        bodies = [function(text, signature) for text in texts]
        assert bodies[0] == bodies[1], (signature, "root/native method mismatch")
        rows.append({
            "signature": signature, "root": paths[0], "native": paths[1],
            "root_line": texts[0][:texts[0].index(signature)].count("\n") + 1,
            "native_line": texts[1][:texts[1].index(signature)].count("\n") + 1,
            "body_sha256": hashlib.sha256(bodies[0].encode()).hexdigest(),
        })
    for original, native in HEADERS:
        assert (ROOT / original).read_bytes() == (ROOT / native).read_bytes(), (original, "native header differs")
    for signature in FUNCTIONS:
        definitions = [str(path.relative_to(ROOT)) for path in (ROOT / "pc-port/src").rglob("*.cpp")
                       if signature in path.read_text()]
        assert definitions == [FUNCTIONS[signature][1]], (signature, "native definition owner", definitions)
    (NOTE / "source-correspondence.json").write_text(json.dumps(rows, indent=2) + "\n")
    print("4 exact root/native methods, unchanged headers, compile-time source hashes, and unique native ownership verified")


if __name__ == "__main__":
    main()
