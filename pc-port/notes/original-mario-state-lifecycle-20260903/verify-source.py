#!/usr/bin/env python3
"""Verify the exact state imports, retail defaults, and original-compiler vtables.

No native build is run. Generated objects and full diffs stay under build/.
"""
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import struct
import subprocess
import types

ROOT = Path(__file__).resolve().parents[3]
NOTE = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-mario-state-lifecycle-20260903'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
RETAIL = ROOT / 'build/mario-update-restoration-20260903/retail/obj/Game/Player'
SHA1 = '25c5959534b3c21246c6c7e42021b916b41fb578'
GROUPS = {
    'MarioState.cpp': {
        'MarioState::MarioState(': '__ct__10MarioStateFP10MarioActorUl',
        'bool MarioState::proc(': 'proc__10MarioStateFUl',
        'void Mario::sendStateMsg(': 'sendStateMsg__5MarioFUl',
        'bool Mario::updatePosture(': 'updatePosture__5MarioFPA4_f',
        'bool MarioState::postureCtrl(': 'postureCtrl__10MarioStateFPA4_f',
        'void Mario::changeStatus(': 'changeStatus__5MarioFP10MarioState',
        'void Mario::closeStatus(': 'closeStatus__5MarioFP10MarioState',
        'u32 MarioState::getNoticedStatus(': 'getNoticedStatus__10MarioStateCFv',
        'void MarioState::init(': 'init__10MarioStateFv',
        'bool MarioState::notice(': 'notice__10MarioStateFv',
        'bool MarioState::keep(': 'keep__10MarioStateFv',
        'void MarioState::hitPoly(': 'hitPoly__10MarioStateFUcRCQ29JGeometry8TVec3<f>P9HitSensor',
        'f32 MarioState::getBlurOffset(': 'getBlurOffset__10MarioStateCFv',
        'void MarioState::draw3D(': 'draw3D__10MarioStateCFv',
    },
    'MarioDamage.cpp': {
        'bool MarioState::start(': 'start__10MarioStateFv',
        'bool MarioState::close(': 'close__10MarioStateFv',
        'bool MarioState::update(': 'update__10MarioStateFv',
    },
    'MarioActorDefensiveMsg.cpp': {
        'void MarioState::hitWall(': 'hitWall__10MarioStateFRCQ29JGeometry8TVec3<f>P9HitSensor',
        'bool MarioState::passRing(': 'passRing__10MarioStateFPC9HitSensor',
    },
}
QUERIES = {
    'u32 Mario::getCurrentStatus(': 'getCurrentStatus__5MarioCFv',
    'bool Mario::isStatusActive(': 'isStatusActive__5MarioCFUl',
}


def body(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def read_dol(data, address, size):
    for index in range(18):
        offset, base, length = [struct.unpack_from('>I', data, field + index * 4)[0]
                                for field in (0, 0x48, 0x90)]
        if base <= address and address + size <= base + length:
            return data[offset + address - base:offset + address - base + size]
    raise AssertionError(hex(address))


def run(command, name):
    (BUILD / (name + '-command.json')).write_text(json.dumps(command, indent=2) + '\n')
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (name + '.log')).write_text(result.stdout)
    if result.stdout:
        print(result.stdout, end='')
    result.check_returncode()


def original_flags():
    for node in ast.parse((ROOT / 'configure.py').read_text()).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_game' for t in node.targets):
            flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                         {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
            return [part for flag in flags for part in shlex.split(flag)]
    raise AssertionError('Game compiler flags missing')


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    data = DOL.read_bytes()
    assert hashlib.sha1(data).hexdigest() == SHA1
    symbols = {}
    for line in (ROOT / 'config/RMGK01/symbols.txt').read_text().splitlines():
        match = re.match(r'(.+?) = \.[^:]+:(0x[0-9A-F]+);.*size:(0x[0-9A-F]+)', line)
        if match:
            symbols[match[1]] = tuple(int(value, 16) for value in match.groups()[1:])
    reverse = {address: name for name, (address, size) in symbols.items()}
    provider = (ROOT / 'pc-port/src/compat/MarioStateCompat.cpp').read_text()
    correspondence = []
    extra_bodies = []
    for unit, functions in GROUPS.items():
        source = (ROOT / 'src/Game/Player' / unit).read_text()
        for marker, symbol in functions.items():
            original = body(source, marker)
            assert body(provider, marker) == original, marker
            correspondence.append({'source': 'src/Game/Player/' + unit, 'symbol': symbol,
                                   'source_body_sha256': hashlib.sha256(original.encode()).hexdigest()})
            if unit != 'MarioState.cpp':
                extra_bodies.append(original)
    query_source = (ROOT / 'pc-port/src/compat/MarioStateAccessCompat.cpp').read_text()
    state_source = (ROOT / 'src/Game/Player/MarioState.cpp').read_text()
    for marker in QUERIES:
        assert body(query_source, marker) == body(state_source, marker), marker
    for name in ('MarioState.hpp', 'MarioHang.hpp'):
        assert (ROOT / 'include/Game/Player' / name).read_bytes() == (ROOT / 'pc-port/src/Game/Player' / name).read_bytes()

    # Reuse the existing ELF reader without executing its independent verifier.
    path = ROOT / 'pc-port/notes/mario-update-restoration-20260903/verify-object.py'
    spec = importlib.util.spec_from_file_location('state_original_elf', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    base = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe'] + original_flags()
    run(base + ['-c', 'src/Game/Player/MarioState.cpp', '-o', str(BUILD / 'MarioState.o')], 'MarioState-compile')
    # The unrelated Mario::fixHangDir declaration still disagrees with its
    # definition, so validate the actual Hang constructor/header independently.
    hang_source = (ROOT / 'src/Game/Player/MarioHang.cpp').read_text()
    hang_probe = BUILD / 'HangConstructor.cpp'
    hang_probe.write_text(hang_source[:hang_source.index('namespace NrvMarioActor')]
                          + body(hang_source, 'MarioHang::MarioHang(') + '\n\n'
                          + body(hang_source, 'bool MarioHang::start(') + '\n')
    run(base + ['-c', str(hang_probe), '-o', str(BUILD / 'MarioHang.o')], 'MarioHang-compile')
    extras = BUILD / 'StateDefaults.cpp'
    extras.write_text('#include "Game/Player/MarioState.hpp"\n\n' + '\n\n'.join(extra_bodies) + '\n')
    run(base + ['-c', str(extras), '-o', str(BUILD / 'StateDefaults.o')], 'StateDefaults-compile')
    elves = {name: module.Elf(BUILD / (name + '.o')) for name in ('MarioState', 'MarioHang', 'StateDefaults')}

    def compiled_bytes(elf, name):
        _, start, size, section = next(s for s in elf.symbols if s[0] == name)
        return elf.section_data(section)[start:start + size]

    defaults = {}
    default_names = [symbol for group in GROUPS.values() for symbol in group.values()
                     if symbols[symbol][1] <= 8]
    for name in default_names:
        address, size = symbols[name]
        elf = elves['MarioState'] if any(name == s[0] and s[3] for s in elves['MarioState'].symbols) else elves['StateDefaults']
        original = read_dol(data, address, size)
        current = compiled_bytes(elf, name)
        if name == 'getBlurOffset__10MarioStateCFv':
            # The load's linked SDA2 displacement differs, but its target is the same +0 constant.
            # R_PPC_EMB_SDA21 patches both the base register and displacement.
            assert len(current) == size and (struct.unpack('>I', current[:4])[0] >> 21
                                             == struct.unpack('>I', original[:4])[0] >> 21)
            assert current[4:] == original[4:]
            refs = elf.references(name)
            assert len(refs) == 1 and refs[0]['value_hex'] == '00000000', refs
            offset = struct.unpack('>h', original[2:4])[0]
            assert read_dol(data, 0x806bfc20 + offset, 4) == bytes(4)
            exact = 'identical opcodes and referenced +0; SDA2 relocation differs'
        else:
            assert original == current, name
            exact = 'identical bytes'
        defaults[name] = {'address': hex(address), 'retail_hex': original.hex(), 'verification': exact}

    vtables = {}
    for name, elf_name in [('__vt__10MarioState', 'MarioState'), ('__vt__9MarioHang', 'MarioHang')]:
        address, size = symbols[name]
        words = struct.unpack('>18I', read_dol(data, address, size))
        expected = {offset * 4: reverse[value] for offset, value in enumerate(words) if value}
        references = {int(r['offset'], 16): r['symbol'] for r in elves[elf_name].references(name)}
        assert references == expected, (name, references, expected)
        vtables[name] = {'address': hex(address), 'size': size,
                         'compiler_relocations_equal_retail_slots': True,
                         'slots': {hex(offset): value for offset, value in expected.items()}}

    objdiff = BUILD / 'state-objdiff.json'
    run(['build/tools/objdiff-cli', 'diff', '-1', str(RETAIL / 'MarioState.o'), '-2', str(BUILD / 'MarioState.o'),
         '-o', str(objdiff), '--format', 'json-pretty'], 'state-objdiff')
    diff = json.loads(objdiff.read_text())
    methods = list(GROUPS['MarioState.cpp'].values())[:8] + list(QUERIES.values())
    results = {}
    for name in methods:
        left, right = [next(s for s in diff[side]['symbols'] if s['name'] == name) for side in ('left', 'right')]
        assert left['match_percent'] >= 95, (name, left['match_percent'])
        results[name] = {'match_percent': left['match_percent'], 'retail_size': left['size'], 'compiled_size': right['size']}
        print(name, left['match_percent'])
    evidence = {'dol_sha1': SHA1, 'native_runtime': 'tested separately; this script only compiles original source',
                'imported_bodies': correspondence, 'retained_read_only_queries': list(QUERIES.values()),
                'base_virtual_defaults': defaults, 'vtable_proof': vtables, 'lifecycle_objdiff': results}
    (NOTE / 'source-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
    print('Exact source imports, all eleven retail base defaults, both sixteen-slot vtables, and lifecycle verified.')


if __name__ == '__main__':
    main()
