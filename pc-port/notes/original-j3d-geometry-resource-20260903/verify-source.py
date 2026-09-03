#!/usr/bin/env python3
"""Verify exact original SDK imports, PPC compilation, and retail metadata rules."""
import ast
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess
import types

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-j3d-geometry-resource-20260903'
BUILD.mkdir(parents=True, exist_ok=True)

factory = ROOT / 'src/JSystem/J3DGraphLoader/J3DShapeFactory.cpp'
native_factory = ROOT / 'pc-port/src/compat/J3DShapeFactoryCompat.cpp'
assert factory.read_bytes() == native_factory.read_bytes()
table = ROOT / 'src/JSystem/J3DGraphAnimator/J3DShapeTable.cpp'
native_table = ROOT / 'pc-port/src/compat/J3DShapeTableCompat.cpp'
assert table.read_text() == native_table.read_text().replace('#include "JSystem/J3DGraphBase/J3DShape.hpp"\n', '')
assert (ROOT / 'libs/JSystem/include/JSystem/J3DGraphLoader/J3DShapeFactory.hpp').read_bytes() == (ROOT / 'pc-port/src/JSystem/J3DGraphLoader/J3DShapeFactory.hpp').read_bytes()
for node in ast.parse((ROOT / 'configure.py').read_text()).body:
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_jsys' for t in node.targets):
        flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                     {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
        break
results = {}
for source in [factory, table]:
    name = source.stem
    command = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
    for flag in flags:
        command.extend(shlex.split(flag))
    command += ['-c', str(source.relative_to(ROOT)), '-o', str(BUILD / (name + '.o'))]
    (BUILD / (name + '.command.json')).write_text(json.dumps(command, indent=2) + '\n')
    run = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (name + '.compile.log')).write_text(run.stdout)
    run.check_returncode()
    relative = source.relative_to(ROOT / 'src').with_suffix('.o')
    retail = ROOT / 'build/original-j3d-joint-traversal-20260903/retail/obj' / relative
    output = BUILD / (name + '-objdiff.json')
    subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(retail), '-2', str(BUILD / (name + '.o')),
                    '-o', str(output), '--format', 'json-pretty'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
    diff = json.loads(output.read_text())
    results[name] = {symbol['name']: {'match_percent': symbol['match_percent'], 'size': int(symbol['size'])}
                     for symbol in diff['left']['symbols'] if name in symbol['name'] and symbol.get('match_percent') is not None and symbol.get('size', 0)}

dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
def word(address):
    for i in range(18):
        offset, base, size = [struct.unpack_from('>I', dol, field + i * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + 4 <= base + size:
            return struct.unpack_from('>I', dol, offset + address - base)[0]
    raise AssertionError(hex(address))
checks = {
    0x8043e874: 0x2c00000a,  # search NRM format
    0x8043e898: 0x2c000004,  # F32 predicate
    0x8043e89c: 0x38c00006,  # non-F32 normal stride
    0x8043e8a4: 0x38c0000c,  # F32 normal stride
    0x8043e908: 0x7ca03396,  # normal distance / stride
    0x8043e90c: 0x38050001,  # +1
    0x8043e924: 0x7ca03396,  # final-array fallback / stride
    0x8043e928: 0x38050001,  # +1
    0x8043e97c: 0x5405f0be,  # color distance / 4
    0x8043e980: 0x38050001,  # +1
    0x8043e998: 0x5405f0be,  # final color fallback / 4
    0x8043e99c: 0x38050001,  # +1
    0x8043e9c8: 0x5404e8fe,  # texture distance / 8 regardless format
    0x8043e9cc: 0x38040001,  # +1
    0x804408dc: 0xa0030004,  # shape VCD byte offset
    0x804408e0: 0x7c050214,  # byte-add, not scaled index
    0x80440948: 0x1c1c00c0,  # original command buffer slice = logicalNo * 192
    0x804409ac: 0xb39f0008,  # original logical shape numbering
}
for address, expected in checks.items():
    assert word(address) == expected, hex(address)
report = {
    'dol_sha1': hashlib.sha1(dol).hexdigest(),
    'factory_exact_root_copy_sha256': hashlib.sha256(factory.read_bytes()).hexdigest(),
    'shape_table_exact_bodies_with_one_compile_include': True,
    'shape_factory_header_exact': True,
    'original_compiler': results,
    'retail_instruction_checks': {hex(address): hex(value) for address, value in checks.items()},
    'scope': 'Existing original factory/table sources with sizeof-only native ABI corrections. Matching percentages are reported without claiming binary identity for lower-matching factory allocation bodies. Decoder behavior is covered by separate native resource tests.'
}
(HERE / 'source-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
