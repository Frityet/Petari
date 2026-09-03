#!/usr/bin/env python3
"""Compile real root JKR/list sources, verify layout and compare retail objects."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import struct

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-jkr-base-20260903'
BUILD.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('original', HERE.parent / 'j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)
spec = importlib.util.spec_from_file_location('proof', HERE.parent / 'original-binder-reaction-20260903/verify-runtime.py')
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)
raw = v.DOL.read_bytes()
assert hashlib.sha1(raw).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
symbols = {m[1]: (int(m[2], 16), int(m[3], 16)) for m in re.finditer(
    r'^(\S+) = \.text:(0x[\dA-Fa-f]+); // type:function size:(0x[\dA-Fa-f]+)',
    (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
addresses = {m[1]: int(m[2], 16) for m in re.finditer(
    r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',
    (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}

def relocated(elf, target, name, address, size, dol):
    _, start, length, section = next(x for x in elf.symbols if x[0] == name)
    assert length == size
    code = bytearray(elf.section_data(section)[start:start + size])
    retail = v.dol_bytes(dol, address, size)
    refs = elf.references(name)
    for ref in refs:
        offset, kind = int(ref['offset'], 16), ref['kind']
        # Resolve named SDK addresses directly, or verify every byte of an
        # unnamed data object at the address encoded by the retail SDA load.
        if ref['symbol'] in addresses:
            effective = addresses[ref['symbol']] + ref['addend']
        else:
            assert kind == 109 and 'value_hex' in ref, ref
            original = struct.unpack_from('>I', retail, offset)[0]
            base = (original >> 16) & 31
            displacement = struct.unpack('>h', struct.pack('>H', original & 65535))[0]
            effective = {2: 0x806BFC20, 13: 0x806B9620}[base] + displacement
            payload = bytes.fromhex(ref['value_hex'])
            assert v.dol_bytes(dol, effective, len(payload)) == payload, ref
        if kind in (4, 6):
            value = effective if kind == 4 else (effective + 0x8000) >> 16
            struct.pack_into('>H', code, offset, value & 65535)
        else:
            word = struct.unpack_from('>I', code, offset)[0]
            if kind == 10:
                word = (word & 0xFC000003) | ((effective - address - offset) & 0x3FFFFFC)
            elif kind == 109:
                base = (struct.unpack_from('>I', retail, offset)[0] >> 16) & 31
                displacement = effective - {2: 0x806BFC20, 13: 0x806B9620}[base]
                assert -32768 <= displacement < 32768
                word = (word & ~0x1FFFFF) | (base << 16) | (displacement & 65535)
            else: raise AssertionError(ref)
            struct.pack_into('>I', code, offset, word)
    return bytes(code), refs

commands, records, sources = [], {}, []

def run(command, log):
    commands.append([str(x) for x in command])
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()

for unit in ('JKernel/JKRHeap', 'JKernel/JKRSolidHeap', 'JKernel/JKRDisposer', 'JSupport/JSUList'):
    name = unit.split('/')[-1]
    source = ROOT / 'src/JSystem' / (unit + '.cpp')
    sources.append(source)
    obj = BUILD / (name + '.o')
    run(v.compiler('cflags_jsys') + ['-c', str(source), '-o', str(obj)], name + '.compile.log')
    retail = ROOT / 'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/JSystem' / (unit + '.o')
    run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(retail), '-2', str(obj),
         '-o', str(BUILD / (name + '.diff.json')), '--format', 'json-pretty'], name + '.objdiff.log')
    diff = json.loads((BUILD / (name + '.diff.json')).read_text())
    right = {s['name']: s for s in diff['right']['symbols'] if int(s.get('size', 0))}
    target, compiled = proof.reaction.reader.Elf(retail), proof.reaction.reader.Elf(obj)
    entries = []
    for entry in diff['left']['symbols']:
        symbol = entry['name']
        if symbol not in right or symbol not in symbols or 'match_percent' not in entry: continue
        address, size = symbols[symbol]
        item = {'name': symbol, 'address': hex(address), 'retail_size': size,
                'compiled_size': int(right[symbol]['size']), 'objdiff_match_percent': entry['match_percent']}
        if entry['match_percent'] == 100:
            code, refs = relocated(compiled, target, symbol, address, size, raw)
            assert code == v.dol_bytes(raw, address, size), symbol
            item['retail_bytes_equal_after_relocation'] = True
        entries.append(item)
    records[name] = entries
    print(name, len(entries), 'retail methods compared')

solid = proof.reaction.reader.Elf(BUILD / 'JKRSolidHeap.o')
extra = []
for name, address, size in [('do_free__12JKRSolidHeapFPv', 0x803A43C8, 4),
                             ('getMaxAllocatableSize__7JKRHeapFi', 0x803A4530, 0x60)]:
    code, refs = relocated(solid, solid, name, address, size, raw)
    assert code == v.dol_bytes(raw, address, size), name
    extra.append({'name': name, 'address': hex(address), 'size': size,
                  'retail_bytes_equal_after_relocation': True})

checks = {'sizeof(JKRHeap)': 0x6C, 'sizeof(JKRSolidHeap)': 0x7C,
          'offsetof(JKRHeap, mAllocMode)': 0x6A, 'offsetof(JKRHeap, mCurrentGroupId)': 0x6B,
          'offsetof(JKRHeap, mDisposerList)': 0x5C, 'offsetof(JKRSolidHeap, mFreeSize)': 0x6C,
          'sizeof(JKRDisposer)': 0x18, 'sizeof(JSUPtrLink)': 0x10}
layout = '#include "JSystem/JKernel/JKRSolidHeap.hpp"\n#include <stddef.h>\n'
layout += '\n'.join(f'typedef char Check{i}[({expr} == {value}) ? 1 : -1];' for i, (expr, value) in enumerate(checks.items()))
(BUILD / 'layout.cpp').write_text(layout + '\n')
run(v.compiler('cflags_jsys') + ['-c', str(BUILD / 'layout.cpp'), '-o', str(BUILD / 'layout.o')], 'layout.compile.log')
assert v.dol_bytes(raw, 0x806B26D8, 1) == b'\x01'
assert 'static bool byte_806B26D8 = true;' in (ROOT / 'src/JSystem/JKernel/JKRHeap.cpp').read_text()
sources += [ROOT / 'libs/JSystem/include/JSystem' / x for x in ('JKernel/JKRHeap.hpp', 'JKernel/JKRSolidHeap.hpp', 'JKernel/JKRDisposer.hpp', 'JSupport/JSUList.hpp')]
report = {'dol_sha1': hashlib.sha1(raw).hexdigest(), 'compiler_commands': commands,
          'methods': records, 'out_of_unit_methods': extra, 'verified_ppc_layout': checks,
          'restored_static': {'address': '0x806B26D8', 'byte': 1},
          'source_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
          'scope': 'Existing original heap/list algorithms plus recovered Solid no-op free and static default. Percentages include preexisting compiler inlining/scheduling differences; only explicitly marked methods are byte-equal.'}
(HERE / 'original-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
