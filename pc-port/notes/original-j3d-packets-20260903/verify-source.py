#!/usr/bin/env python3
"""Check exact imports with only the listed native architecture substitutions."""
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('model_source', ROOT / 'pc-port/notes/original-j3d-model-owner-20260903/verify-source.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)
method = helper.method


def main():
    checked = []
    for name in ('J3DPacket', 'J3DDrawBuffer', 'J3DShape', 'J3DShapeDraw', 'J3DShapeMtx', 'J3DGD'):
        root = (ROOT / ('src/JSystem/J3DGraphBase/' + name + '.cpp')).read_text()
        native = (ROOT / ('pc-port/src/compat/' + name + 'Compat.cpp')).read_text()
        if name == 'J3DPacket':
            root = root.replace('#include <mem.h>', '#include <cstring>')
        elif name == 'J3DShape':
            root = root.replace('    u32 idx = (attr == GX_VA_NBT) ? 1 : (attr - GX_VA_POS);\n'
                                '    J3DLoadCPCmd(0xA0 + idx, ((uintptr_t)data & 0x7FFFFFFF));',
                                '    GXSetArrayBase(attr, data);')
        elif name == 'J3DShapeDraw':
            root = root.replace('revolution/gx/GXDispList.h', 'dolphin/gx/GXDispList.h')
            root = root.replace('*((u16*)(dl))', '(u16(dl[0]) << 8) | dl[1]')
            root = root.replace('*(u16*)oldDL', '(u16(oldDL[0]) << 8) | oldDL[1]')
            root = root.replace('        *(u16*)newDL = vtxNum;',
                                '        newDL[0] = static_cast<u8>(vtxNum >> 8);\n        newDL[1] = static_cast<u8>(vtxNum);')
        elif name == 'J3DShapeMtx':
            root = '#include <cstring>\n' + root
            root = root.replace(method(root, 'inline void J3DPSMtx33Copy('), method(native, 'inline void J3DPSMtx33Copy('))
            # The shared transform provider owns the separately restored copy.
            copy = method(root, 'void J3DPSMtx33CopyFrom34(')
            other = (ROOT / 'pc-port/src/compat/J3DTransformMtxCompat.cpp').read_text()
            assert copy in other
            root = root.replace(copy, '')
        elif name == 'J3DGD':
            root = root.replace('"revolution/gd/', '"dolphin/gd/')
        assert root.strip() == native.strip(), name + ': unlisted root/native difference'
        checked.append(name)
    for root_path, native_path, signatures in (
        ('src/JSystem/J3DGraphAnimator/J3DJoint.cpp', 'J3DJointEntry', ('void J3DJoint::entryIn()',)),
        ('src/JSystem/J3DGraphBase/J3DVertex.cpp', 'J3DVertexBuffer', ('void J3DVertexBuffer::setArray()',)),
        ('src/JSystem/J3DGraphBase/J3DSys.cpp', 'J3DSys', ('void J3DSys::loadPosMtxIndx(', 'void J3DSys::loadNrmMtxIndx(')),
        ('src/Game/System/Overwrite.cpp', 'J3DShapeMtxGame', ('void J3DShapeMtx::loadMtxIndx_PNGP(',)),
        ('src/JSystem/J3DGraphBase/J3DTransform.cpp', 'J3DTextureMtx', ('void J3DGetTextureMtx(', 'void J3DGetTextureMtxOld(', 'void J3DGetTextureMtxMaya(', 'void J3DGetTextureMtxMayaOld(')),
        ('src/JSystem/J3DGraphBase/J3DTevs.cpp', 'J3DTexMtx', ('void J3DTexMtx::load(', 'void J3DTexMtx::calc(', 'void J3DTexMtx::calcTexMtx(', 'void J3DTexMtx::calcPostTexMtx(', 'void J3DTexMtx::loadTexMtx(', 'void J3DTexMtx::loadPostTexMtx(')),
    ):
        root = (ROOT / root_path).read_text()
        native = (ROOT / ('pc-port/src/compat/' + native_path + 'Compat.cpp')).read_text()
        for signature in signatures:
            assert method(root, signature).replace('_GXTexMtxType', 'GXTexMtxType') in native, signature
            checked.append(signature)
    compiler = json.loads((NOTES / 'compiler-evidence.json').read_text())
    for path, sha in compiler['source_sha256'].items():
        assert hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == sha, path
    (NOTES / 'source-evidence.json').write_text(json.dumps({'verified_imports': checked,
        'source_hashes_match_original_compiler_evidence': True,
        'architecture_boundaries': ['native GX array pointer width', 'unaligned big-endian primitive counts',
            'original paired copy load/store order', 'SDK forwarding include paths', 'GX enum typedef spelling']}, indent=2) + '\n')
    print(len(checked), 'literal imports/extracts verified; compiled root source hashes agree')


if __name__ == '__main__':
    main()
