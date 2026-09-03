#!/usr/bin/env python3
"""Check original helper import and retail joint/draw metadata instructions."""
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
BUILD = ROOT / 'build/original-j3d-joint-resource-20260903'
BUILD.mkdir(parents=True, exist_ok=True)

def body(path, marker):
    source = (ROOT / path).read_text()
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

signature = 'void J3DJointTree::findImportantMtxIndex()'
source = body('src/JSystem/J3DGraphAnimator/J3DJointTree.cpp', signature)
assert source == body('pc-port/src/compat/J3DJointTreeCompat.cpp', signature)
for node in ast.parse((ROOT / 'configure.py').read_text()).body:
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_jsys' for t in node.targets):
        flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                     {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
        break
command = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags:
    command.extend(shlex.split(flag))
command += ['-c', 'src/JSystem/J3DGraphAnimator/J3DJointTree.cpp', '-o', str(BUILD / 'JointTree-original.o')]
(BUILD / 'original.command.json').write_text(json.dumps(command, indent=2) + '\n')
result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
(BUILD / 'original.compile.log').write_text(result.stdout)
result.check_returncode()
retail = ROOT / 'build/original-j3d-joint-traversal-20260903/retail/obj/JSystem/J3DGraphAnimator/J3DJointTree.o'
subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(retail), '-2', str(BUILD / 'JointTree-original.o'),
                '-o', str(BUILD / 'joint-tree-objdiff.json'), '--format', 'json-pretty'], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
diff = json.loads((BUILD / 'joint-tree-objdiff.json').read_text())
symbol = next(s for s in diff['left']['symbols'] if s['name'] == 'findImportantMtxIndex__12J3DJointTreeFv')
dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
def word(address):
    for i in range(18):
        offset, base, size = [struct.unpack_from('>I', dol, field + i * 4)[0] for field in (0, 0x48, 0x90)]
        if base <= address and address + 4 <= base + size:
            return struct.unpack_from('>I', dol, offset + address - base)[0]
    raise AssertionError(hex(address))
checks = {
    0x8043ea78: 0xa0040008, # serialized draw count
    0x8043ea88: 0xa0a7002e, # current envelope count
    0x8043ea8c: 0x7c050050, # subtract
    0x8043ea90: 0xb0070044, # store native draw entry count
    0x8043eadc: 0x28000001, # exact flag == 1
    0x80440674: 0x7c05022e, # load authored kind u16
    0x80440678: 0x981f0016, # narrow kind into byte field
    0x80440690: 0x88040002, # load raw compensation byte
    0x80440694: 0x981f0017, # preserve raw byte
    0x80440740: 0x280000ff, # original ff sentinel test
    0x80440748: 0x987f0017, # normalize sentinel to zero
}
for address, expected in checks.items():
    assert word(address) == expected, hex(address)
report = {
    'dol_sha1': hashlib.sha1(dol).hexdigest(),
    'important_index_body_exact': True,
    'important_index_body_sha256': hashlib.sha256(source.encode()).hexdigest(),
    'important_index_original_compiler_match_percent': symbol.get('match_percent'),
    'important_index_retail_size': int(symbol['size']),
    'retail_metadata_instruction_checks': {hex(k): hex(v) for k, v in checks.items()},
    'scope': 'Retail metadata stores and an unchanged original helper; native byte decoder is validated by focused resource tests, not an original-compiler matching claim.'
}
(HERE / 'source-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
