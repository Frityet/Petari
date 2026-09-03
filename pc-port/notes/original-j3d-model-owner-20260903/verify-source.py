#!/usr/bin/env python3
"""Check literal root/native correspondence for this ownership group."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
NATIVE = ROOT/'pc-port/src'
NOTE = Path(__file__).resolve().parent


def read(name): return (ROOT/name).read_text()
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def method(source, signature):
    start = source.index(signature)
    pos = source.index('{', start)+1
    depth = 1
    while depth:
        if source[pos] == '{': depth += 1
        elif source[pos] == '}': depth -= 1
        pos += 1
    return source[start:pos]


def main():
    checks = []
    model_source = read('src/JSystem/J3DGraphAnimator/J3DModel.cpp')
    model_native = (NATIVE/'compat/J3DModelCompat.cpp').read_text()
    assert model_native.startswith(model_source)
    dtor = method(read('src/Game/Player/J3DModelX.cpp'), 'J3DModel::~J3DModel()')
    assert model_native.count(dtor) == 1
    checks.append('Complete original Model.cpp and exact separately defined Model destructor')
    for root, native, extra in [
        ('src/JSystem/J3DGraphAnimator/J3DCluster.cpp', 'compat/J3DClusterCompat.cpp', '#include <dolphin/base/PPCArch.h>\n'),
        ('src/JSystem/J3DGraphAnimator/J3DSkinDeform.cpp', 'compat/J3DSkinDeformCompat.cpp', ''),
    ]:
        assert (NATIVE/native).read_text().replace(extra, '', 1) == read(root) if extra else (NATIVE/native).read_text() == read(root)
        checks.append(root+' copied literally'+(' except the PPCSync declaration include' if extra else ''))
    material_source = read('src/JSystem/J3DGraphBase/J3DMaterial.cpp')
    begin = material_source.index('void J3DMaterial::initialize()')
    end = material_source.index('void J3DPatchedMaterial::initialize()')
    material_native = (NATIVE/'compat/J3DMaterialHelpersCompat.cpp').read_text()
    assert material_source[begin:end].strip() in material_native
    assert method(read('src/JSystem/J3DGraphBase/J3DTevs.cpp'), 'void loadNBTScale(') in material_native
    checks.append('Complete base Material virtuals/nonfactory helpers plus original loadNBTScale')
    data = read('src/JSystem/J3DGraphAnimator/J3DModelData.cpp')
    data_native = (NATIVE/'compat/J3DModelDataCompat.cpp').read_text()
    for signature in ['void J3DModelData::clear()', 'J3DModelData::J3DModelData()', 'J3DModelData::~J3DModelData()',
                      'void J3DModelData::syncJ3DSysFlags()', 's32 J3DModelData::newSharedDisplayList(', 'void J3DModelData::indexToPtr()']:
        assert method(data, signature) in data_native
    checks.append('All six original ModelData methods present without changes')
    for name in ['JSystem/J3DGraphAnimator/J3DModel.hpp', 'JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp',
                 'JSystem/J3DGraphBase/J3DMatBlock.hpp', 'JSystem/J3DGraphBase/J3DTevs.hpp',
                 'JSystem/J3DGraphBase/J3DTexture.hpp', 'JSystem/JUtility/JUTNameTab.hpp']:
        assert (NATIVE/name).read_bytes() == (ROOT/'libs/JSystem/include'/name).read_bytes()
        checks.append(name+' header byte identity')
    name = 'JSystem/J3DGraphAnimator/J3DAnimation.hpp'
    expected = read('libs/JSystem/include/'+name).replace('typedef struct _GXColor GXColor;\ntypedef struct _GXColorS10 GXColorS10;\n', '')
    expected = expected.replace('getTevColorReg(u16, _GXColorS10*)', 'getTevColorReg(u16, GXColorS10*)')
    assert (NATIVE/name).read_text() == expected
    checks.append('Full Animation header; only redundant GX tag aliases removed/replaced')
    name = 'JSystem/J3DGraphBase/J3DMaterial.hpp'
    native = (NATIVE/name).read_text()
    native = native.replace('        // Preserve retail\'s rejected 32-bit range while allowing real native\n        // pointers above the Wii address space.\n', '')
    native = native.replace(' || (uintptr_t)mMaterialAnm > 0xFFFFFFFF', '')
    assert native == read('libs/JSystem/include/'+name)
    checks.append('Material header; only native pointer-width rejection boundary adapted')
    evidence = json.loads((NOTE/'compiler-evidence.json').read_text())
    for name, sha in evidence['source_sha256'].items():
        assert digest(ROOT/name) == sha, 'Source differs from original compiler evidence: '+name
    for name, sha in evidence['headers_sha256'].items():
        assert digest(ROOT/name) == sha, 'Header differs from original compiler evidence: '+name
    print('\n'.join(checks))
    print('All owned root/native correspondence and original compiled-source hashes verified.')


if __name__ == '__main__': main()
