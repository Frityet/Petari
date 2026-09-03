#!/usr/bin/env python3
"""Check original source imports and native boundary invariants for this checkpoint."""
from pathlib import Path
import hashlib

ROOT = Path(__file__).resolve().parents[3]
for path in ['Game/Animation/XanimeCore.cpp', 'Game/Animation/XanimeCore.hpp']:
    source = ROOT / ('src' if path.endswith('.cpp') else 'include') / path
    native = ROOT / 'pc-port/src' / path
    assert source.read_bytes() == native.read_bytes(), path
    print('Byte-identical original:', path, hashlib.sha256(source.read_bytes()).hexdigest())

owner = (ROOT / 'pc-port/src/compat/OriginalJ3dJointTree.cpp').read_text()
assert 'XanimeCore core;' in owner
assert 'calculate_transform' not in owner and 'animation->calcTransform' not in owner
assert 'J3DAnmTransformKey playback;' in owner and 'playback = *animation;' in owner
assert 'const_cast' not in owner and 'reinterpret_cast' not in owner
assert 'j3dSys.mCurrentMtxCalc = _system_calculator;' in owner
assert 'joint_storage(core.mJointList)' in owner and 'track_storage(core.mTrackList)' in owner
math = (ROOT / 'pc-port/src/compat/J3DJointCompat.cpp').read_text()
assert 'return ppc_fres(value);' in math and 'reciprocalEstimateTable' not in math
header = (ROOT / 'pc-port/aurora/include/dolphin/mtx.h').read_text()
assert '#define PSMTXQuat' not in header
assert '#ifdef MTX_USE_PS\n#define MTXQuat PSMTXQuat' in header
print('Actual core, independent playback frame, explicit storage ownership and shared SDK math boundary verified.')
