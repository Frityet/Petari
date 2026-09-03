#!/usr/bin/env python3
"""Verify complete recovered root methods with configured original compiler flags."""
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-effect-construction-20260903'
METHODS = {
    'Game/Effect/MultiEmitter': ['allocateEmitter__12MultiEmitterFPCc'],
    'Game/Effect/ParticleResourceHolder': ['__ct__22ParticleResourceHolderFPCc', 'getUserIndex__22ParticleResourceHolderCFPCc',
        'countAutoEffectNum__22ParticleResourceHolderFv', 'swapTexture__22ParticleResourceHolderFPC7ResTIMGPCc',
        'isExistInResource__22ParticleResourceHolderCFPCcPUs', 'getAutoEffectListBinary__22ParticleResourceHolderCFv',
        'getAutoEffectNum__22ParticleResourceHolderCFPCc'],
    'Game/Effect/EffectSystemUtil': ['isExistInResource__Q22MR6EffectFPUsPCc', 'getAutoEffectNum__Q22MR6EffectFPCc',
        'getAutoEffectListBinary__Q22MR6EffectFv', 'isExistInResource__Q22MR6EffectFPUsPCcl'],
    'Game/System/Overwrite': ['init__14JPABaseEmitterFP17JPAEmitterManagerP11JPAResource'],
}

def main():
    spec = importlib.util.spec_from_file_location('helper', ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    results = []
    for name, methods in METHODS.items():
        source = ROOT / ('src/' + name + '.cpp')
        obj = BUILD / (source.stem + '.o')
        command = helper.compiler('cflags_game') + ['-c', str(source.relative_to(ROOT)), '-o', str(obj)]
        result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (BUILD / (source.stem + '.log')).write_text(result.stdout)
        result.check_returncode()
        target = ROOT / ('build/mario-update-restoration-20260903/retail/obj/' + name + '.o')
        comparison = BUILD / (source.stem + '.objdiff.json')
        subprocess.run([str(ROOT / 'build/tools/objdiff-cli'), 'diff', '-1', str(target), '-2', str(obj), '-o', str(comparison), '--format', 'json-pretty'], check=True, capture_output=True)
        data = json.loads(comparison.read_text())
        symbols = {s['name']: s for s in data['left']['symbols']}
        selected = [{'name': name, 'size': int(symbols[name]['size']), 'match_percent': symbols[name]['match_percent']} for name in methods]
        assert all(s['match_percent'] >= 90 for s in selected), selected
        results.append({'source': str(source.relative_to(ROOT)), 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'command': command, 'methods': selected})
    probe = BUILD / 'layout.cpp'
    probe.write_text('#include "Game/Effect/ParticleResourceHolder.hpp"\n#include "JSystem/JParticle/JPAEmitter.hpp"\n#include <stddef.h>\n'
        'typedef char HolderSize[sizeof(ParticleResourceHolder) == 0x1010 ? 1 : -1];\n'
        'typedef char CountOffset[offsetof(ParticleResourceHolder, mNumParticles) == 0x100c ? 1 : -1];\n'
        'typedef char EmitterSize[sizeof(JPABaseEmitter) == 0x114 ? 1 : -1];\n'
        'typedef char UserWorkOffset[offsetof(JPABaseEmitter, mpUserWork) == 0xc0 ? 1 : -1];\n')
    command = helper.compiler('cflags_game') + ['-c', str(probe), '-o', str(BUILD / 'layout.o')]
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / 'layout.log').write_text(result.stdout)
    result.check_returncode()
    (NOTES / 'retail-evidence.json').write_text(json.dumps({'dol_sha1': hashlib.sha1(dol).hexdigest(), 'sources': results, 'layout_command': command, 'layout_returncode': 0}, indent=2) + '\n')
    for result in results:
        for method in result['methods']:
            print(method['name'], method['match_percent'])

if __name__ == '__main__':
    main()
