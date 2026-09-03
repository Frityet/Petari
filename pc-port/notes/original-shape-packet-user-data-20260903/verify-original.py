#!/usr/bin/env python3
"""Verify actual root source against the retained RMGK01 object/DOL evidence.

Uses configured GC3.0a3 flags and real root include paths. No overlays, native
build, or build-configuration mutations. Struct's second compile is explicitly
a diagnostic using the existing Game IPA flags, not the canonical SDK flags.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess

NOTE = Path(__file__).resolve().parent
ROOT = NOTE.parents[2]
BUILD = ROOT/'build/original-shape-packet-user-data-20260903'
spec = importlib.util.spec_from_file_location('model_proof', NOTE.parent/'original-j3d-model-owner-20260903/verify-original.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)

FUNCTIONS = {
    '__ct__19ShapePacketUserDataFv': (0x803A9DD0, 0x38),
    'init__19ShapePacketUserDataFP11J3DMaterial': (0x803A9E08, 0x234),
    'callDL__19ShapePacketUserDataCFv': (0x803AA03C, 0x10),
    'loadTexMtx__19ShapePacketUserDataCFP11J3DMaterialiUs': (0x803AA04C, 0x144),
    'getJ3DShapePacketUserData__2MRFPC14J3DShapePacket': (0x803AA190, 0x14),
    'initJ3DShapePacketUserData__2MRFP8J3DModel': (0x803AA1A4, 0xD4),
    '__as__13J3DTexMtxInfoFRC13J3DTexMtxInfo': (0x804313A8, 0x7C),
    'setEffectMtx__13J3DTexMtxInfoFPA4_f': (0x80431424, 0x48),
}


def run(command, stem):
    (BUILD/(stem+'-command.json')).write_text(json.dumps(command, indent=2)+'\n')
    p = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD/(stem+'.log')).write_text(p.stdout)
    p.check_returncode()


def load_store_map(elf, name, alias=False):
    """Bounded symbolic byte transfer for these straight-line copy functions.

    Reject every instruction except the actual loads/stores and blr. This is
    not a general PPC or floating-point emulator. It compares field transfers,
    padding preservation, and constants without treating objdiff as equality.
    """
    code, refs = elf.code_and_refs(name)
    refs = {r['offset']: r for r in refs}
    source_base = 0x1000 if alias else 0x2000
    memory = {0x1000+i: f'dst:{i:02x}' for i in range(0x64)}
    if not alias:
        memory.update({source_base+i: f'src:{i:02x}' for i in range(0x64)})
    gpr = {3: 0x1000, 4: source_base}
    values = {}
    fpr = {}
    for off in range(0, len(code), 4):
        word = struct.unpack_from('>I', code, off)[0]
        if word == 0x4E800020:
            assert off == len(code)-4
            break
        opcode, reg, parent = word >> 26, (word >> 21) & 31, (word >> 16) & 31
        displacement = word & (0xFFF if opcode in (56, 60) else 0xFFFF)
        sign = 0x800 if opcode in (56, 60) else 0x8000
        if displacement & sign:
            displacement -= sign*2
        if opcode in (56, 60):
            assert word & 0xF000 == 0, 'Only unquantized two-float PS transfers are expected'
        if off in refs:
            ref = refs[off]
            assert opcode == 48 and ref['kind'] == 109
            _, at, _, section = next(s for s in elf.symbols if s[0] == ref['symbol'])
            data = list(elf.section_data(section)[at+ref['addend']:at+ref['addend']+4])
        else:
            address = gpr[parent]+displacement
            length = {34: 1, 32: 4, 48: 4, 56: 8, 38: 1, 36: 4, 52: 4, 60: 8}[opcode]
            if opcode in (34, 32, 48, 56):
                data = [memory[address+i] for i in range(length)]
        if opcode in (34, 32):
            values[reg] = data
        elif opcode == 48:
            fpr[reg] = data*2  # Gekko lfs loads the scalar into both PS lanes.
        elif opcode == 56:
            fpr[reg] = data
        elif opcode in (38, 36):
            for i, byte in enumerate(values[reg]): memory[address+i] = byte
        elif opcode in (52, 60):
            for i, byte in enumerate(fpr[reg][:length]): memory[address+i] = byte
        else:
            raise AssertionError(hex(word))
    return [memory[0x1000+i] for i in range(0x64)]


def verify_struct_transfers(retail, compiled, dol):
    results = {}
    for name in list(FUNCTIONS)[-2:]:
        original_code, refs = retail.code_and_refs(name)
        address, size = FUNCTIONS[name]
        raw = base.dol_bytes(dol, address, size)
        relocations = {r['offset']: r for r in refs}
        assert len(original_code) == len(raw)
        for off in range(0, size, 4):
            left, right = struct.unpack_from('>I', original_code, off)[0], struct.unpack_from('>I', raw, off)[0]
            if off in relocations:
                assert relocations[off]['kind'] == 109
                assert (left & 0xFFE00000) == (right & 0xFFE00000)
            else:
                assert left == right
        for alias in (False, True):
            expected = load_store_map(retail, name, alias)
            actual = load_store_map(compiled, name, alias)
            assert actual == expected, name
        mapping = load_store_map(retail, name)
        if name.startswith('__as__'):
            assert mapping[:2] == ['src:00', 'src:01']
            assert mapping[2:4] == ['dst:02', 'dst:03']
            assert mapping[4:] == [f'src:{i:02x}' for i in range(4, 0x64)]
        else:
            assert mapping[:0x24] == [f'dst:{i:02x}' for i in range(0x24)]
            assert mapping[0x24:0x54] == [f'src:{i:02x}' for i in range(0x30)]
            assert bytes(mapping[0x54:]) == struct.pack('>4f', 0, 0, 0, 1)
        results[name] = {'retail_split_matches_dol_except_SDA_relocations': True,
                        'symbolic_transfer_matches_retail': True,
                        'separate_and_identical_source_destination_checked': True,
                        'output_byte_mapping': mapping}
    return results


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = base.DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    units = [
        ('ShapePacketUserData', 'src/Game/System/ShapePacketUserData.cpp', 'J3DModelX', 'Game/System/ShapePacketUserData.o'),
        ('J3DStruct', 'src/JSystem/J3DGraphBase/J3DStruct.cpp', 'SDK', 'JSystem/J3DGraphBase/J3DStruct.o'),
        ('J3DStruct-inlined', 'src/JSystem/J3DGraphBase/J3DStruct.cpp', 'J3DModelX', 'JSystem/J3DGraphBase/J3DStruct.o'),
    ]
    evidence = {'dol_sha1': hashlib.sha1(dol).hexdigest(), 'functions': [], 'source_sha256': {},
                'compiler': 'GC3.0a3; configured Game/SDK flags; inlined Struct is an explicitly labelled Game-flag diagnostic'}
    commands = {}
    for stem, source, flags, original in units:
        obj = BUILD/(stem+'.o')
        command = base.compiler(flags)+['-c', source, '-o', str(obj)]
        commands[stem] = command
        source_hash = base.sha(ROOT/source)
        run(command, stem+'-compile')
        assert source_hash == base.sha(ROOT/source)
        evidence['source_sha256'][source] = source_hash
        out = BUILD/(stem+'-objdiff.json')
        run(['build/tools/objdiff-cli', 'diff', '-1', str(base.RET/original), '-2', str(obj),
             '-o', str(out), '--format', 'json-pretty'], stem+'-objdiff')
        diff = json.loads(out.read_text())
        right = {s['name']: s for s in diff['right']['symbols']}
        for s in diff['left']['symbols']:
            name = s['name']
            if name not in FUNCTIONS or name not in right: continue
            address, size = FUNCTIONS[name]
            item = {'unit': stem, 'name': name, 'address': hex(address), 'retail_size': size,
                    'compiled_size': int(right[name]['size']), 'objdiff_match_percent': s.get('match_percent'),
                    'retail_sha256': hashlib.sha256(base.dol_bytes(dol, address, size)).hexdigest()}
            evidence['functions'].append(item)
            print(f"{stem}: {name}: {item['objdiff_match_percent']}% ({item['compiled_size']}/{size} bytes)")
            if stem == 'ShapePacketUserData': assert item['objdiff_match_percent'] == 100
    for name in ['include/Game/System/ShapePacketUserData.hpp',
                 'libs/JSystem/include/JSystem/J3DGraphBase/J3DStruct.hpp',
                 'libs/RVL_SDK/include/revolution/gd/GDGeometry.h',
                 'libs/RVL_SDK/include/revolution/gd/GDTransform.h']:
        evidence['source_sha256'][name] = base.sha(ROOT/name)
    evidence['struct_transfers'] = verify_struct_transfers(
        base.Elf(base.RET/'JSystem/J3DGraphBase/J3DStruct.o'), base.Elf(BUILD/'J3DStruct-inlined.o'), dol)
    (BUILD/'compiler-evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
    (BUILD/'original-commands.json').write_text(json.dumps(commands, indent=2)+'\n')
    print('Both Struct byte-transfer maps agree with retail, including padding, constants, and exact aliasing.')


if __name__ == '__main__': main()
