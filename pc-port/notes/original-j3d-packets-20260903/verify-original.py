#!/usr/bin/env python3
"""Compile root sources with configured GC flags and record retail comparisons.

This does not claim that inherited partial decompilations are exact. The new
methods and all imported methods retain their individual comparison results.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / 'build/original-j3d-packets-20260903'
RET = ROOT / 'build/j3d-vertex-buffer-lifecycle-20260903/retail/obj'
NOTES = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('vertex_proof', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)

UNITS = {
    name: 'JSystem/J3DGraphBase/' + name for name in (
        'J3DPacket', 'J3DDrawBuffer', 'J3DShape', 'J3DShapeDraw', 'J3DShapeMtx',
        'J3DGD', 'J3DVertex', 'J3DTevs', 'J3DTransform', 'J3DSys')
}
UNITS['Overwrite'] = 'Game/System/Overwrite'
NEW = (
    'setArray__15J3DVertexBufferCFv',
    'calcNBTScale__16J3DShapeMtxMultiFRC3VecPA3_A3_fPA3_A3_f',
    'loadMtxIndx_PNGP__11J3DShapeMtxCFiUs',
    'J3DGDWriteBPCmd__FUl', 'J3DGDWriteCPCmd__FUcUl',
    'J3DGDWriteXFCmdHdr__FUsUc', 'J3DGDWriteXFCmd__FUsUl',
)


def run(command, name):
    proc = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (name + '.log')).write_text(proc.stdout)
    if proc.returncode:
        print(proc.stdout)
    proc.check_returncode()


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    evidence = {
        'dol_sha1': hashlib.sha1(helper.DOL.read_bytes()).hexdigest(),
        'scope': 'Actual root translation units; individual objdiff percentages are not a bit-exact assertion.',
        'source_sha256': {}, 'commands': {}, 'units': {}, 'recovered_functions': []
    }
    assert evidence['dol_sha1'] == '25c5959534b3c21246c6c7e42021b916b41fb578'
    for name, path in UNITS.items():
        source = ROOT / ('src/' + path + '.cpp')
        evidence['source_sha256'][str(source.relative_to(ROOT))] = helper.sha(source)
        command = helper.compiler('cflags_game' if path.startswith('Game/') else 'cflags_jsys')
        command += ['-c', str(source), '-o', str(BUILD / (name + '.o'))]
        evidence['commands'][name] = command
        run(command, name + '.compile')
        result = BUILD / (name + '.objdiff.json')
        run(['build/tools/objdiff-cli', 'diff', '-1', str(RET / (path + '.o')),
             '-2', str(BUILD / (name + '.o')), '-o', str(result), '--format', 'json-pretty'], name + '.objdiff')
        diff = json.loads(result.read_text())
        right = {s['name']: s for s in diff['right']['symbols'] if int(s.get('size', 0)) > 0}
        entries = []
        for left in diff['left']['symbols']:
            symbol = left['name']
            if symbol not in right or not left.get('instructions'):
                continue
            entry = {'name': symbol, 'retail_size': int(left['size']),
                     'compiled_size': int(right[symbol]['size']),
                     'objdiff_percent': left.get('match_percent')}
            entries.append(entry)
            if symbol in NEW:
                evidence['recovered_functions'].append({'unit': name, **entry})
                print(name, symbol, entry['objdiff_percent'], entry['retail_size'], entry['compiled_size'], flush=True)
        evidence['units'][name] = entries
    # The inline BP/CP functions normally disappear into their callers. Taking
    # their real header addresses emits diagnostic copies, without overlays or
    # changing production optimization flags. XF lives in a different split TU.
    diagnostic = BUILD / 'inline-writers.cpp'
    diagnostic.write_text('#include "JSystem/J3DGraphBase/J3DGD.hpp"\n'
                          'void (*emitBP)(u32) = J3DGDWriteBPCmd;\n'
                          'void (*emitCP)(u8,u32) = J3DGDWriteCPCmd;\n')
    command = helper.compiler('cflags_jsys') + ['-c', str(diagnostic), '-o', str(BUILD / 'inline-writers.o')]
    evidence['commands']['inline-writers-diagnostic'] = command
    run(command, 'inline-writers.compile')
    for label, retail, native, names in (
        ('inline-writers', 'JSystem/J3DGraphBase/J3DGD.o', 'inline-writers.o', NEW[3:5]),
        ('xf-writer', 'JSystem/J3DGraphBase/J3DMaterial.o', 'J3DGD.o', NEW[6:7]),
    ):
        result = BUILD / (label + '.objdiff.json')
        run(['build/tools/objdiff-cli', 'diff', '-1', str(RET / retail), '-2', str(BUILD / native),
             '-o', str(result), '--format', 'json-pretty'], label + '.objdiff')
        diff = json.loads(result.read_text())
        for name in names:
            left = next(s for s in diff['left']['symbols'] if s['name'] == name)
            right = next(s for s in diff['right']['symbols'] if s['name'] == name)
            entry = {'unit': label, 'name': name, 'retail_size': int(left['size']),
                     'compiled_size': int(right['size']), 'objdiff_percent': left.get('match_percent')}
            evidence['recovered_functions'].append(entry)
            print(label, name, entry['objdiff_percent'], entry['retail_size'], entry['compiled_size'], flush=True)
    assert set(NEW) <= {v['name'] for v in evidence['recovered_functions']}
    for source, expected in evidence['source_sha256'].items():
        assert helper.sha(ROOT / source) == expected, 'Root source changed while compiling: ' + source
    (NOTES / 'compiler-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')


if __name__ == '__main__':
    main()
