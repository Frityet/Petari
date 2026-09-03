#!/usr/bin/env python3
from pathlib import Path
import hashlib
root = Path(__file__).resolve().parents[3]
def function(path, name):
    text = (root / path).read_text()
    start = text.index("void " + name + "(")
    brace = text.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]
header = "libs/JSystem/include/JSystem/J3DGraphBase/J3DTexture.hpp"
native = "pc-port/src/JSystem/J3DGraphBase/J3DTexture.hpp"
assert (root/header).read_bytes() == (root/native).read_bytes()
for source, native, names in [
    ("src/JSystem/J3DGraphBase/J3DTevs.cpp", "pc-port/src/compat/J3DTevsCompat.cpp", ["loadTexNo"]),
    ("src/JSystem/J3DGraphBase/J3DGD.cpp", "pc-port/src/compat/J3DGDCompat.cpp", ["J3DGDSetTexImgPtr", "J3DGDLoadTlut"]),
]:
    for name in names:
        assert function(source,name) == function(native,name), name
        print("unchanged original", name)
paths = [header, "src/JSystem/J3DGraphLoader/J3DModelLoader.cpp", "libs/JSystem/include/JSystem/JUtility/JUTTexture.hpp",
    "pc-port/src/resource/J3dTextureData.cpp", "pc-port/src/resource/J3dTextureData.hpp",
    "pc-port/src/resource/Mem1ResourceHeap.cpp", "pc-port/src/resource/Mem1ResourceHeap.hpp",
    "pc-port/tests/OriginalJ3DTextureResourceTests.cpp", "pc-port/aurora/include/dolphin/os/OSAlloc.h",
    "pc-port/aurora/lib/dolphin/os/OSAlloc.cpp", "pc-port/aurora/include/dolphin/gx/GXAurora.h",
    "pc-port/aurora/lib/dolphin/gx/GXAurora.cpp"]
for path in paths:
    print(hashlib.sha256((root/path).read_bytes()).hexdigest(), path)
