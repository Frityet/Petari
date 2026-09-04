#!/usr/bin/env python3
from pathlib import Path
import hashlib, importlib.util, json, re, struct, subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-multi-emitter-sync-20260903'
BUILD.mkdir(parents=True, exist_ok=True)

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
proof = module('proof', 'pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
addresses = {n: int(a, 16) for n, a in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);',
             (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
dol = compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
rows, commands = [], []
for unit in ('MultiEmitter', 'SyncBckEffectInfo'):
    source = ROOT / f'src/Game/Effect/{unit}.cpp'
    obj = BUILD / f'{unit}.o'
    command = compiler.compiler('cflags_game') + ['-c', str(source), '-o', str(obj)]
    subprocess.run(command, cwd=ROOT, check=True)
    commands.append(command)
    retail = ROOT / f'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Effect/{unit}.o'
    output = BUILD / f'{unit}-diff.json'
    subprocess.run([ROOT / 'build/tools/objdiff-cli', 'diff', '-1', retail, '-2', obj, '-o', output,
                    '--format', 'json-pretty'], check=True)
    a, b = reader.Elf(retail), reader.Elf(obj)
    diff = json.loads(output.read_text())
    for name, start, size, section in a.symbols:
        if not size or section == 0 or not a.sections[section][2] & 4:
            continue
        if unit == 'MultiEmitter' and not name.startswith(('scanParticleEmitter', 'initSyncBck', 'onDeleteSyncBck', 'addSyncBck', 'setContinueBckEnd')):
            continue
        original, _ = proof.relocated(a, a, name, addresses[name], size, dol)
        assert original == reader.dol_bytes(dol, addresses[name], size), name
        code, refs = proof.relocated(b, a, name, addresses[name], size, dol)
        assert code == original, name
        score = next(s['match_percent'] for s in diff['left']['symbols'] if s['name'] == name)
        rows.append({'name': name, 'address': hex(addresses[name]), 'size': size, 'objdiff_percent': score,
                     'relocated_bytes_equal': code == original, 'relocations': refs})
        print(name, score, size, 'relocated bytes exact=' + str(code == original))
    if unit == 'MultiEmitter':
        # Validate all three words AND the relocation in the member-function
        # descriptor loaded by the recovered bind2nd traversal.
        name = 'scanParticleEmitter__12MultiEmitterFP12EffectSystem'
        descriptors = []
        for elf in (a, b):
            descriptor = elf.references(name)[0]['symbol']
            _, start, size, section = next(s for s in elf.symbols if s[0] == descriptor)
            assert size == 12
            data = bytearray(elf.section_data(section)[start:start + size])
            refs = elf.references(descriptor)
            assert refs == [{'offset': '0x8', 'kind': 1, 'symbol': 'scanParticleEmitter__13SingleEmitterFP12EffectSystem', 'addend': 0}]
            struct.pack_into('>I', data, 8, addresses[refs[0]['symbol']])
            descriptors.append(bytes(data))
        assert descriptors[0] == descriptors[1] == reader.dol_bytes(dol, 0x80578588, 12)

# Preserve every previously compiled MultiEmitter function and data record.
before = BUILD / 'MultiEmitter-before.o'
command = compiler.compiler('cflags_game') + ['-c', str(NOTES / 'baseline/MultiEmitter.cpp'), '-o', str(before)]
subprocess.run(command, cwd=ROOT, check=True)
a, b = reader.Elf(before), reader.Elf(BUILD / 'MultiEmitter.o')
unchanged = []
def canonical(elf, name):
    _, start, size, section = next(s for s in elf.symbols if s[0] == name)
    data = elf.section_data(section)[start:start + size]
    refs = elf.references(name)
    for ref in refs:
        if 'value_hex' in ref:
            # Replace generated literal labels by full data + its relocations.
            symbol = next(s for s in elf.symbols if s[0] == ref['symbol'])
            _, offset, length, index = symbol
            ref['symbol'] = {'bytes': elf.section_data(index)[offset:offset + length].hex(),
                             'relocations': elf.references(symbol[0])}
    return data, refs
for name, start, size, section in a.symbols:
    if not size or section == 0 or not a.sections[section][2] & 4:
        continue
    assert canonical(a, name) == canonical(b, name), name
    unchanged.append({'name': name, 'size': size})
assert len(rows) == 12
layout_command = compiler.compiler('cflags_game') + ['-c', str(NOTES / 'layout-original.cpp'), '-o', str(BUILD / 'layout-original.o')]
subprocess.run(layout_command, cwd=ROOT, check=True)
commands.append(layout_command)
(NOTES / 'root-evidence.json').write_text(json.dumps({
    'dol_sha1': hashlib.sha1(dol).hexdigest(), 'commands': commands,
    'source_sha256': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in (
        'src/Game/Effect/MultiEmitter.cpp', 'src/Game/Effect/SyncBckEffectInfo.cpp', 'include/Game/Effect/SyncBckEffectInfo.hpp')},
    'functions': rows, 'member_function_descriptor_hex': descriptors[0].hex(),
    'all_prior_mult_emitter_functions_unchanged': unchanged,
}, indent=2) + '\n')
print(f'{len(rows)} functions / {sum(r["size"] for r in rows)} bytes verified; {len(unchanged)} previous functions unchanged')
