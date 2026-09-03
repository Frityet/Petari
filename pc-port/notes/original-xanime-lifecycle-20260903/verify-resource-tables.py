#!/usr/bin/env python3
"""Check the unchanged original resource and transform object bodies."""

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]


def main():
    source = ROOT / "src/Game/System/ResourceInfo.cpp"
    imported = ROOT / "pc-port/src/Game/System/ResourceInfo.cpp"
    expected = source.read_text().replace("#include <size_t.h>", "#include <cstddef>")
    expected = expected.replace('#include "Game/Util.hpp"',
                                '#include "Game/Util.hpp"\n#include "Game/Util/HashUtil.hpp"')
    assert imported.read_text() == expected.rstrip("\n") + "\n"

    header = ROOT / "libs/JSystem/include/JSystem/J3DGraphAnimator/J3DAnimation.hpp"
    native_header = ROOT / "pc-port/src/JSystem/J3DGraphAnimator/J3DAnimation.hpp"
    text = header.read_text()
    declarations = text[text.index("class J3DAnmBase {"):text.index("class J3DAnmTransformKey :")].rstrip()
    assert declarations in native_header.read_text()

    animation = ROOT / "src/JSystem/J3DGraphAnimator/J3DAnimation.cpp"
    native_animation = ROOT / "pc-port/src/compat/J3DTransformAnimationCompat.cpp"
    text = animation.read_text()
    start = text.index("J3DAnmTransform::J3DAnmTransform(")
    constructor = text[start:text.index("\n}", start) + 2]
    assert native_animation.read_text().endswith(constructor + "\n")

    paths = [source, imported, header, native_header, animation, native_animation]
    report = {
        "resource_info": "Complete original body; only explicit HashUtil include and size_t.h to cstddef compile fixes.",
        "j3d_transform": "Original base/transform declarations and constructor unchanged; no sampler or loader is supplied yet.",
        "source_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
    }
    Path(__file__).with_name("resource-table-source-evidence.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Original ResourceInfo bodies and J3D base/transform object definitions preserved")


if __name__ == "__main__":
    main()
