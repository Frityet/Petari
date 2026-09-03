#!/usr/bin/env python3
"""Verify actual restored Game source, Wii layout and complete relocated code."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-particle-emitter-holder-20260903'
BUILD.mkdir(parents=True, exist_ok=True)

def module(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
proof = module('proof', 'pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
source = ROOT / 'src/Game/Effect/ParticleEmitterHolder.cpp'
obj = BUILD / 'ParticleEmitterHolder.o'
command = compiler.compiler('cflags_game') + ['-c', str(source), '-o', str(obj)]
subprocess.run(command, cwd=ROOT, check=True)
retail = ROOT / 'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Effect/ParticleEmitterHolder.o'
diff = BUILD / 'objdiff.json'
subprocess.run([ROOT / 'build/tools/objdiff-cli', 'diff', '-1', retail, '-2', obj,
                '-o', diff, '--format', 'json-pretty'], check=True)
a, b = reader.Elf(retail), reader.Elf(obj)
comparison = json.loads(diff.read_text())
addresses = {n: int(addr, 16) for n, addr in re.findall(
    r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);', (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
dol = compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
functions = []
for name, start, size, section in a.symbols:
    if name not in addresses or not size or not ('ParticleEmitterHolder' in name or name.startswith(('isValid__', 'isContinuousParticle__'))):
        continue
    side = next(s for s in comparison['left']['symbols'] if s['name'] == name)
    assert side['match_percent'] == 100, name
    relocated, refs = proof.relocated(b, a, name, addresses[name], size, dol)
    assert relocated == reader.dol_bytes(dol, addresses[name], size), name
    functions.append({'name': name, 'address': hex(addresses[name]), 'bytes': size,
                      'match_percent': 100, 'relocated_bytes_equal': True, 'relocations': refs})
assert len(functions) == 9

# Verify all twelve bytes and the function relocation in the member predicate,
# not just the constant prefix read by the generic relocation helper.
predicate = 'findAvailableParticleEmitter__21ParticleEmitterHolderFv'
constant = next(r for r in functions[5]['relocations'] if 'value_hex' in r)
constant_address = int(constant['effective_retail_target'], 16)
expected = struct.pack('>III', 0, 0xffffffff, addresses['isValid__15ParticleEmitterCFv'])
assert reader.dol_bytes(dol, constant_address, 12) == expected
for elf in (a, b):
    name = next(r['symbol'] for r in elf.references(predicate) if 'value_hex' in r)
    _, offset, size, section = next(s for s in elf.symbols if s[0] == name)
    assert size == 12
    raw = bytearray(elf.section_data(section)[offset:offset+size])
    refs = elf.references(name)
    assert len(refs) == 1 and refs[0]['kind'] == 1 and refs[0]['offset'] == '0x8'
    assert refs[0]['symbol'] == 'isValid__15ParticleEmitterCFv' and refs[0]['addend'] == 0
    struct.pack_into('>I', raw, 8, addresses[refs[0]['symbol']])
    assert raw == expected

layout = BUILD / 'layout.cpp'
checks = {'sizeof(ParticleEmitterHolder)': 12, 'sizeof(ParticleEmitter)': 8,
          'offsetof(ParticleEmitterHolder, mEffectSystem)': 0,
          'offsetof(ParticleEmitterHolder, mEmitters)': 4,
          'offsetof(ParticleEmitter, mStopped)': 5}
layout.write_text('#include "Game/Effect/ParticleEmitterHolder.hpp"\n#include <stddef.h>\n' +
    '\n'.join(f'typedef char Check{i}[({expr} == {value}) ? 1 : -1];' for i, (expr, value) in enumerate(checks.items())) + '\n')
subprocess.run(compiler.compiler('cflags_game') + ['-c', str(layout), '-o', str(BUILD/'layout.o')], cwd=ROOT, check=True)
paths = ['src/Game/Effect/ParticleEmitterHolder.cpp', 'include/Game/Effect/ParticleEmitter.hpp',
         'include/Game/Effect/ParticleEmitterHolder.hpp', 'libs/MSL_C++/include/functional.hpp']
evidence = {'compiler_command': command, 'dol_sha1': hashlib.sha1(dol).hexdigest(),
            'source_sha256': {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},
            'layout': checks, 'member_predicate_address': hex(constant_address),
            'member_predicate_bytes': expected.hex(), 'functions': functions}
(NOTES/'evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
print(f'{len(functions)} original methods, {sum(f["bytes"] for f in functions)} code bytes and 12 predicate bytes match retail exactly; Wii layout verified')
