#!/usr/bin/env python3
"""Compile and relocate four complete original emitter link/lifecycle methods."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-particle-emitter-link-20260903'
BUILD.mkdir(parents=True, exist_ok=True)
def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT/path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value
compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
proof = module('proof', 'pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
source = ROOT/'src/Game/Effect/EffectSystemUtil.cpp'
obj = BUILD/'EffectSystemUtil.o'
command = compiler.compiler('cflags_game') + ['-c', str(source), '-o', str(obj)]
subprocess.run(command, cwd=ROOT, check=True)
retail = ROOT/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Effect/EffectSystemUtil.o'
output = BUILD/'objdiff.json'
subprocess.run([ROOT/'build/tools/objdiff-cli', 'diff', '-1', retail, '-2', obj, '-o', output, '--format', 'json-pretty'], check=True)
a, b = reader.Elf(retail), reader.Elf(obj)
diff = json.loads(output.read_text())
addresses = {name: int(address, 16) for name, address in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);', (ROOT/'config/RMGK01/symbols.txt').read_text(), re.M)}
dol = compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
rows = []
for name, offset, size, section in a.symbols:
    if not size or not name.startswith(('createParticleEmitter__', 'deleteParticleEmitter__', 'setLinkSingleEmitter__', 'getLinkSingleEmitter__')):
        continue
    assert next(s for s in diff['left']['symbols'] if s['name'] == name)['match_percent'] == 100
    code, references = proof.relocated(b, a, name, addresses[name], size, dol)
    assert code == reader.dol_bytes(dol, addresses[name], size)
    rows.append({'name': name, 'address': hex(addresses[name]), 'bytes': size,
                 'relocated_bytes_equal': True, 'match_percent': 100, 'references': references})
assert len(rows) == 4
(NOTES/'evidence.json').write_text(json.dumps({'command': command,
    'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'functions': rows}, indent=2)+'\n')
print('Four original emitter lifecycle/link methods: 180 relocated instruction bytes match retail exactly')
