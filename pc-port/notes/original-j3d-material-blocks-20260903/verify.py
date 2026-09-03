#!/usr/bin/env python3
"""Compile actual root SDK units and verify imported source and retail data."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-j3d-material-blocks-20260903'
RET = ROOT / 'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/JSystem/J3DGraphBase'
spec = importlib.util.spec_from_file_location('original', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
original = importlib.util.module_from_spec(spec)
spec.loader.exec_module(original)

NEW = {
    '__as__11J3DTexCoordFRC11J3DTexCoord',
    '__as__13J3DGXColorS10FRC11_GXColorS10',
    'J3DGDSetZMode__FUc10_GXCompareUc',
    '__as__16J3DIndTexMtxInfoFRC16J3DIndTexMtxInfo',
    '__as__10J3DFogInfoFRC10J3DFogInfo',
    '__as__15J3DNBTScaleInfoFRC15J3DNBTScaleInfo',
    'load__11J3DLightObjCFUl',
}
CONSTANTS = {
    'j3dDefaultTexMtxInfo': (0x8055C250, 100),
    'j3dDefaultColInfo': (0x806C1AF0, 4),
    'j3dDefaultAmbInfo': (0x806C1AF4, 4),
    'j3dDefaultNumChans': (0x806C1AF8, 1),
    'j3dDefaultTevOrderInfoNull': (0x806C1AF9, 4),
    'j3dDefaultIndTexOrderNull': (0x806C1AFD, 4),
    'j3dDefaultTevKColor': (0x806C1B01, 4),
    'j3dDefaultTevColor': (0x8055C2D0, 8),
    'j3dDefaultTevSwapModeTable': (0x806C1B05, 4),
    'j3dDefaultBlendInfo': (0x806C1B09, 4),
    'j3dDefaultTevSwapTableID': (0x806C1B0D, 1),
    'j3dDefaultAlphaCmpID': (0x806C1B0E, 2),
    'j3dDefaultZModeID': (0x806C1B10, 2),
    'j3dDefaultColorChanInfo': (0x8055C334, 8),
}


def run(command, label):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (label + '.log')).write_text(result.stdout)
    if result.returncode:
        print(result.stdout)
    result.check_returncode()


def without_includes(source):
    return re.sub(r'^#include[^\n]*\n', '', source, flags=re.M).strip()


def verify_imports():
    root = ROOT / 'src/JSystem/J3DGraphBase'
    native = ROOT / 'pc-port/src/compat'
    expected = (root / 'J3DMatBlock.cpp').read_text()
    expected = expected.replace('*(u32*)color', 'smgpc::compat::read_be_u32(color)')
    expected = expected.replace('*(u32*)(color + 1)', 'smgpc::compat::read_be_u32(color + 1)')
    assert without_includes(expected) == without_includes((native / 'J3DMatBlockCompat.cpp').read_text())
    assert (root / 'J3DStruct.cpp').read_bytes() == (native / 'J3DStructCompat.cpp').read_bytes()
    expected = (root / 'J3DMaterial.cpp').read_text().split('void J3DMaterial::initialize()')[0]
    assert without_includes(expected) == without_includes((native / 'J3DMaterialFactoryCompat.cpp').read_text())
    tevs = (root / 'J3DTevs.cpp').read_text()
    native_tevs = (native / 'J3DTevsCompat.cpp').read_text()
    # Every provider body and constant is extracted literally, except one native
    # unaligned/endian load. No replacement algorithm is accepted by this check.
    canonical = native_tevs.replace('smgpc::compat::read_be_u32((u8*)pDL + 1)', '*(u32*)((u8*)pDL + 1)')
    parts = without_includes(canonical)
    for part in re.split(r'\n\s*\n', parts):
        assert part.strip() in tevs, 'Unexpected Tevs native source: ' + part[:100]
    header = (ROOT / 'libs/JSystem/include/JSystem/J3DGraphBase/J3DTevs.hpp').read_text()
    header = header.replace('*(u32*)&field_0x0', 'smgpc::compat::read_be_u32(&field_0x0)')
    header = header.replace('*(u32*)&field_0x4', 'smgpc::compat::read_be_u32(&field_0x4)')
    assert without_includes(header) == without_includes((ROOT / 'pc-port/src/JSystem/J3DGraphBase/J3DTevs.hpp').read_text())


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    verify_imports()
    dol = original.DOL.read_bytes()
    evidence = {'dol_sha1': hashlib.sha1(dol).hexdigest(), 'source_sha256': {}, 'commands': {}, 'units': {}, 'recovered': [], 'constants': []}
    assert evidence['dol_sha1'] == '25c5959534b3c21246c6c7e42021b916b41fb578'
    for unit in ('J3DMatBlock', 'J3DStruct', 'J3DTevs', 'J3DMaterial'):
        source = ROOT / ('src/JSystem/J3DGraphBase/' + unit + '.cpp')
        evidence['source_sha256'][str(source.relative_to(ROOT))] = original.sha(source)
        command = original.compiler('cflags_jsys') + ['-c', str(source), '-o', str(BUILD / (unit + '.o'))]
        evidence['commands'][unit] = command
        run(command, unit + '.compile')
        output = BUILD / (unit + '.objdiff.json')
        run(['build/tools/objdiff-cli', 'diff', '-1', str(RET / (unit + '.o')), '-2', str(BUILD / (unit + '.o')),
             '-o', str(output), '--format', 'json-pretty'], unit + '.objdiff')
        diff = json.loads(output.read_text())
        right = {s['name']: s for s in diff['right']['symbols'] if int(s.get('size', 0))}
        entries = []
        for left in diff['left']['symbols']:
            if left['name'] not in right or not left.get('instructions'):
                continue
            entry = {'name': left['name'], 'retail_size': int(left['size']), 'compiled_size': int(right[left['name']]['size']),
                     'objdiff_percent': left.get('match_percent')}
            entries.append(entry)
            if left['name'] in NEW:
                evidence['recovered'].append({'unit': unit, **entry})
                print(entry['name'], entry['objdiff_percent'], flush=True)
        evidence['units'][unit] = entries
    assert NEW == {e['name'] for e in evidence['recovered']}
    obj = original.Elf(BUILD / 'J3DTevs.o')
    for name, (address, size) in CONSTANTS.items():
        _, offset, actual_size, section = next(s for s in obj.symbols if s[0] == name)
        assert actual_size == size, (name, actual_size, size)
        actual = obj.section_data(section)[offset:offset + size]
        expected = original.dol_bytes(dol, address, size)
        assert actual == expected, (name, actual.hex(), expected.hex())
        evidence['constants'].append({'name': name, 'address': hex(address), 'size': size, 'bytes': expected.hex()})
    # The zero constants live in retail BSS; do not pretend there are DOL bytes
    # for them. Confirm section classification and emitted native source values.
    symbols = (ROOT / 'config/RMGK01/symbols.txt').read_text()
    for name, address in [('j3dDefaultIndTexCoordScaleInfo', 0x806C2910), ('j3dDefaultTevSwapMode', 0x806C2914)]:
        assert re.search(r'= \.sbss2:0x' + format(address, 'X') + ';', symbols)
        _, offset, size, section = next(s for s in obj.symbols if s[0] == name)
        data = bytes(size) if obj.sections[section][1] == 8 else obj.section_data(section)[offset:offset + size]
        assert data == bytes(4)
        evidence['constants'].append({'name': name, 'address': hex(address), 'size': 4, 'storage': 'retail zero-initialized BSS'})
    for source, digest in evidence['source_sha256'].items():
        assert original.sha(ROOT / source) == digest
    (NOTES / 'compiler-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print('Verified source correspondence, all four original TUs, and16 default objects')


if __name__ == '__main__':
    main()
