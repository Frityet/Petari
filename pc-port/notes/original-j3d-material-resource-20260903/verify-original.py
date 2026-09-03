#!/usr/bin/env python3
"""Verify root-first material factory restoration with the original compiler."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-j3d-material-resource-20260903'
spec = importlib.util.spec_from_file_location('original', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
original = importlib.util.module_from_spec(spec)
spec.loader.exec_module(original)

def body(text):
    return re.sub(r'^#include[^\n]*\n', '', text, flags=re.M).strip()

source = ROOT / 'src/JSystem/J3DGraphLoader/J3DMaterialFactory.cpp'
native = ROOT / 'pc-port/src/compat/J3DMaterialLoaderFactoryCompat.cpp'
assert source.read_bytes() == native.read_bytes()
variants = (ROOT / 'src/JSystem/J3DGraphBase/J3DMaterial.cpp').read_text().split('void J3DPatchedMaterial::initialize()', 1)[1]
assert body((ROOT / 'pc-port/src/compat/J3DMaterialVariantsCompat.cpp').read_text()) == ('void J3DPatchedMaterial::initialize()' + variants).strip()
BUILD.mkdir(parents=True, exist_ok=True)
obj = BUILD / 'J3DMaterialFactory.o'
command = original.compiler('cflags_jsys') + ['-c', str(source), '-o', str(obj)]
with (BUILD / 'compiler.log').open('w') as log:
    subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
retail = ROOT / 'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj/JSystem/J3DGraphLoader/J3DMaterialFactory.o'
output = BUILD / 'objdiff.json'
subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(retail), '-2', str(obj), '-o', str(output), '--format', 'json-pretty'], cwd=ROOT, check=True)
diff = json.loads(output.read_text())
right = {s['name']: s for s in diff['right']['symbols'] if int(s.get('size', 0))}
entries = []
for left in diff['left']['symbols']:
    if left['name'] in right and left.get('instructions'):
        entries.append({'name': left['name'], 'retail_size': int(left['size']), 'compiled_size': int(right[left['name']]['size']),
                        'objdiff_percent': left.get('match_percent')})
indirect = next(s for s in diff['left']['symbols'] if s['name'].startswith('newIndTexStageNum__'))
assert any(i['instruction']['formatted'] == 'cmplwi r0, 0x1' for i in indirect['instructions'])
evidence = {
    'dol_sha1': hashlib.sha1(original.DOL.read_bytes()).hexdigest(),
    'source_sha256': {str(p.relative_to(ROOT)): original.sha(p) for p in (
        source, native, ROOT / 'libs/JSystem/include/JSystem/J3DGraphBase/J3DMaterial.hpp',
        ROOT / 'pc-port/src/JSystem/J3DGraphBase/J3DMaterial.hpp')},
    'command': command,
    'functions': entries,
    'indirect_enable': 'Retail loads the raw byte and compares it with exactly1.',
    'scope': 'Restored previously commented original createNormalMaterial/createPatchedMaterial; entire native provider is root-identical.',
}
constructor = next(e for e in entries if e['name'] == '__ct__11J3DMaterialFv')
assert constructor['retail_size'] == constructor['compiled_size'] == 84 and constructor['objdiff_percent'] == 100
assert evidence['dol_sha1'] == '25c5959534b3c21246c6c7e42021b916b41fb578'
(HERE / 'compiler-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
for entry in entries:
    if entry['name'].startswith(('createNormalMaterial', 'createPatchedMaterial', 'createLockedMaterial')):
        print(entry)
