#!/usr/bin/env python3
"""Check original source identity, declared native adaptations, and proof hashes."""
import hashlib
import importlib.util
import json
from pathlib import Path

NOTE = Path(__file__).resolve().parent
ROOT = NOTE.parents[2]
spec = importlib.util.spec_from_file_location('model_source', NOTE.parent/'original-j3d-model-owner-20260903/verify-source.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)


def main():
    pairs = [
        ('src/Game/System/ShapePacketUserData.cpp', 'pc-port/src/Game/System/ShapePacketUserData.cpp'),
        ('include/Game/System/ShapePacketUserData.hpp', 'pc-port/src/Game/System/ShapePacketUserData.hpp'),
        ('src/Game/Util/SchedulerUtil.cpp', 'pc-port/src/Game/Util/SchedulerUtil.cpp'),
        ('include/Game/Util/SchedulerUtil.hpp', 'pc-port/src/Game/Util/SchedulerUtil.hpp'),
        ('src/JSystem/J3DGraphBase/J3DStruct.cpp', 'pc-port/src/compat/J3DStructCompat.cpp'),
        ('libs/JSystem/include/JSystem/J3DGraphBase/J3DStruct.hpp', 'pc-port/src/JSystem/J3DGraphBase/J3DStruct.hpp'),
    ]
    for original, native in pairs:
        assert (ROOT/original).read_bytes() == (ROOT/native).read_bytes(), native
        print(f'Byte identity: {original} -> {native}')
    original = (ROOT/'src/Game/Util/ModelUtil.cpp').read_text()
    native = (ROOT/'pc-port/src/compat/MaterialTextureModeCompat.cpp').read_text()
    for name in ['isEnvelope', 'isUseTexMtx', 'isUseTexMtxEnvMap', 'isUseTexMtxProjMap']:
        assert base.method(original, 'bool '+name+'(') in native
        print('Literal original method: MR::'+name)
    header = (ROOT/'libs/JSystem/include/JSystem/J3DGraphBase/J3DStruct.hpp').read_text()
    assert '__memcpy(&rotation, &other.mRotation, sizeof(rotation));' in header
    assert '__memcpy(&mRotation, &rotation, sizeof(rotation));' in header
    print('Native SRT assignment copies the complete rotation/padding word via a temporary')
    evidence = json.loads((NOTE/'compiler-evidence.json').read_text())
    for path, expected in evidence['source_sha256'].items():
        actual = hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
        assert actual == expected, 'Compiler evidence source changed: '+path
    print('All proof source hashes agree with current root source.')


if __name__ == '__main__': main()
