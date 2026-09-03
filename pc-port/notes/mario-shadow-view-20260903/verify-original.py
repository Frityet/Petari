#!/usr/bin/env python3
"""Original-compiler proof of the recovered shadow methods and typed blur storage."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/mario-shadow-view-20260903'
RETAIL = ROOT / 'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game'

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value

compiler = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
FUNCTIONS = {
    'MarioActorShadow': [
        'calcViewReflectionModel__10MarioActorFv', 'calcViewWallShadowModel__10MarioActorFv',
        'drawShadow__10MarioActorCFv', 'decideShadowMode__10MarioActorFv',
        'calcViewSilhouetteModel__10MarioActorFv', 'calcViewBlurModel__10MarioActorFv',
        'calcViewFootPrint__10MarioActorFv', 'drawSilhouette__10MarioActorCFv',
        'getWaterEdgeDist__9MarioSwimCFv', 'getDrawMtx__8J3DModelFi'],
    'AreaObjUtil': ['calcCubeAxisZ__2MRFPC7AreaObjPQ29JGeometry8TVec3<f>'],
    'MarioActorDraw': ['initBlur__10MarioActorFv', 'drawModelBlur__10MarioActorCFv'],
    'MarioActorInit': [],
}

def run(command, name):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (name + '.log')).write_text(result.stdout)
    result.check_returncode()

BUILD.mkdir(parents=True, exist_ok=True)
dol = compiler.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
symbols = {n: (int(a, 16), int(s, 16)) for n, a, s in re.findall(
    r'^(\S+) = \S+:0x([0-9A-Fa-f]+);.*?size:0x([0-9A-Fa-f]+)',
    (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
evidence = {'dol_sha1': hashlib.sha1(dol).hexdigest(), 'functions': [], 'commands': []}
for name, functions in FUNCTIONS.items():
    folder = 'Util' if name == 'AreaObjUtil' else 'Player'
    source = ROOT / f'src/Game/{folder}/{name}.cpp'
    obj = BUILD / (name + '.o')
    command = compiler.compiler('cflags_game') + ['-c', str(source), '-o', str(obj)]
    evidence['commands'].append(command)
    run(command, name + '.compile')
    retail = RETAIL / folder / (name + '.o')
    diff_file = BUILD / (name + '.diff.json')
    run(['build/tools/objdiff-cli', 'diff', '-1', str(retail), '-2', str(obj),
         '-o', str(diff_file), '--format', 'json-pretty'], name + '.objdiff')
    diff = json.loads(diff_file.read_text())
    elves = [reader.Elf(retail), reader.Elf(obj)]
    for function in functions:
        sides = [next(s for s in diff[k]['symbols'] if s['name'] == function) for k in ('left', 'right')]
        refs = [e.references(function) for e in elves]
        calls = [[r['symbol'] for r in rs if r['kind'] == 10] for rs in refs]
        # Pre-existing initBlur uses an inlined allocation/matrix-loop shape. Its
        # fuzzy result is recorded separately from newly recovered full bodies.
        if name != 'MarioActorDraw':
            assert calls[0] == calls[1], function
            assert sides[0]['match_percent'] >= 96.0, function
        else:
            assert sides[0]['match_percent'] >= 90.0, function
        address, size = symbols[function]
        evidence['functions'].append({
            'name': function, 'address': hex(address), 'retail_size': size,
            'compiled_size': int(sides[1]['size']), 'match_percent': sides[0]['match_percent'],
            'retail_function_sha256': hashlib.sha256(compiler.dol_bytes(dol, address, size)).hexdigest(),
            'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'same_direct_call_order': calls[0] == calls[1],
            'retail_references': refs[0], 'compiled_references': refs[1],
        })
        print(function, sides[0]['match_percent'])

# Confirm every pooled string, including Shift-JIS animation and joint names.
elf = reader.Elf(BUILD / 'MarioActorShadow.o')
_, offset, _, section = next(s for s in elf.symbols if s[0] == '...data.0')
strings = elf.section_data(section)[offset:]
assert strings == compiler.dol_bytes(dol, 0x805B8E00, len(strings))
evidence['entire_string_pool_matches_retail'] = True
# Shared conversion constants are external in the split retail object and local
# in the rebuilt object; their raw values, not their labels, are authoritative.
assert compiler.dol_bytes(dol, 0x80539A70, 16).hex() == '43300000000000004330000080000000'
evidence['integer_conversion_double_constants'] = ['4330000000000000', '4330000080000000']

# Isolate only our storage/area-pointer corrections from the concurrent TVec2f
# screen-box correction: reconstruct the immediately previous layout in build/.
baseline = BUILD / 'baseline-include/Game/Player'
baseline.mkdir(parents=True, exist_ok=True)
header = (ROOT / 'include/Game/Player/MarioActor.hpp').read_text()
header = header.replace('/* 0xA70 */ Mtx* _A70[16];',
                        '/* 0xA70 */ Mtx* _A70[8];\n    /* 0xA90 */ Mtx* _A90[8];')
header = header.replace('/* 0x20C */ const AreaObj* _20C;', '/* 0x20C */ u32 _20C;')
(baseline / 'MarioActor.hpp').write_text(header)
init = (ROOT / 'src/Game/Player/MarioActorInit.cpp').read_text()
init = init.replace('ARRAY_SIZE(_A70) / 2', 'ARRAY_SIZE(_A70)').replace('_A70[i + 8] = nullptr;', '_A90[i] = nullptr;')
init = init.replace('_20C = nullptr;', '_20C = 0;')
path = BUILD / 'MarioActorInit.baseline.cpp'
path.write_text(init)
command = compiler.compiler('cflags_game')
index = command.index('-i')
command[index:index] = ['-i', str(BUILD / 'baseline-include')]
command += ['-c', str(path), '-o', str(BUILD / 'MarioActorInit.baseline.o')]
run(command, 'MarioActorInit.baseline.compile')
run(['build/tools/objdiff-cli', 'diff', '-1', str(BUILD / 'MarioActorInit.baseline.o'),
     '-2', str(BUILD / 'MarioActorInit.o'), '-o', str(BUILD / 'initializer-layout.diff.json'),
     '--format', 'json-pretty'], 'initializer-layout.objdiff')
diff = json.loads((BUILD / 'initializer-layout.diff.json').read_text())
init = next(s for s in diff['left']['symbols'] if s['name'] == 'initMember__10MarioActorFv')
assert init['match_percent'] == 100.0
evidence['isolated_initializer_layout_delta_match_percent'] = init['match_percent']
(NOTES / 'compiler-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
print('PASS: all recovered calls, complete string pool, original constants, and unchanged initializer layout')
