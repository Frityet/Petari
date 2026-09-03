#!/usr/bin/env python3
"""Stage literal original construction sources; compile without publishing providers."""
from pathlib import Path
import difflib
import hashlib
import json
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-effect-construction-20260903'
STAGE = BUILD / 'staged'
SOURCES = [
    'Game/LiveActor/EffectKeeper.cpp', 'Game/Effect/MultiEmitter.cpp',
    'Game/Effect/MultiEmitterCallBack.cpp', 'Game/Effect/MultiEmitterParticleCallBack.cpp',
    'Game/Effect/SingleEmitter.cpp', 'Game/Effect/ParticleResourceHolder.cpp',
    'Game/Effect/EffectSystemUtil.cpp', 'JSystem/JParticle/JPAEmitter.cpp',
    'JSystem/JParticle/JPAEmitterManager.cpp', 'JSystem/JParticle/JPAResourceManager.cpp',
    'JSystem/JParticle/JPAResourceLoader.cpp',
]

def main():
    STAGE.mkdir(parents=True, exist_ok=True)
    for name in SOURCES:
        target = STAGE / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / 'src' / name, target)
    for name in ['Game/Util/Array.hpp', 'Game/Effect/ParticleResourceHolder.hpp']:
        target = STAGE / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / 'include' / name, target)
    name = 'JSystem/JParticle/JPAEmitter.hpp'
    target = STAGE / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / 'libs/JSystem/include' / name, target)
    # Restore original general APIs missing from native header facades.
    path = ROOT / 'pc-port/src/Game/Util/JMapInfo.hpp'
    text = path.read_text()
    original = (ROOT / 'include/Game/Util/JMapInfo.hpp').read_text()
    start = original.index('    bool operator==(const JMapInfoIter&')
    end = original.index('    bool isValid() const', start)
    marker = '    [[nodiscard]] bool isValid() const {'
    assert marker in text
    text = text.replace(marker, original[start:end] + marker, 1)
    (STAGE / 'Game/Util/JMapInfo.hpp').write_text(text)
    path = ROOT / 'pc-port/src/JSystem/JGeometry/TMatrix.hpp'
    text = path.read_text()
    original = (ROOT / 'libs/JSystem/include/JSystem/JGeometry/TMatrix.hpp').read_text()
    start = original.index('        void setEulerZ(f32 angle)')
    end = original.index('\n        void getQuat', start)
    marker = '        void getQuat(TQuat4f& rDest) const;'
    assert marker in text
    text = text.replace(marker, original[start:end] + '\n' + marker, 1)
    (STAGE / 'JSystem/JGeometry').mkdir(parents=True, exist_ok=True)
    (STAGE / 'JSystem/JGeometry/TMatrix.hpp').write_text(text)
    source = (ROOT / 'src/Game/LiveActor/LiveActor.cpp').read_text()
    start = source.index('void LiveActor::initEffectKeeper(')
    end = source.index('\n}', start) + 2
    (STAGE / 'OriginalLiveActorEffectInit.cpp').write_text('#include "Game/LiveActor/LiveActor.hpp"\n#include "Game/LiveActor/EffectKeeper.hpp"\n#include "Game/Util/LiveActorUtil.hpp"\n\n' + source[start:end] + '\n')
    source = (ROOT / 'src/Game/System/Overwrite.cpp').read_text()
    start = source.index('void JPABaseEmitter::init(')
    end = source.index('\n}', start) + 2
    (STAGE / 'OriginalJPAEmitterInit.cpp').write_text('#include "Game/Util/MathUtil.hpp"\n#include "JSystem/JParticle/JPABaseShape.hpp"\n#include "JSystem/JParticle/JPAEmitter.hpp"\n#include "JSystem/JParticle/JPAEmitterManager.hpp"\n\n' + source[start:end] + '\n')
    entries = json.loads((ROOT / 'pc-port/compile_commands.json').read_text())
    entry = next(e for e in entries if e['file'].endswith('/MarioMove.cpp'))
    command = []
    skip = False
    for arg in entry['arguments']:
        if skip:
            skip = False
        elif arg == '-o':
            skip = True
        elif arg not in ('-c', entry['file']):
            command.append(arg)
    command[1:1] = ['-I' + str(STAGE), '-I' + str(ROOT / 'pc-port/src'), '-I' + str(ROOT / 'pc-port/aurora/include')]
    results = []
    for name in SOURCES + ['OriginalLiveActorEffectInit.cpp', 'OriginalJPAEmitterInit.cpp']:
        path = STAGE / name
        obj = BUILD / (path.stem + '-native.o')
        cmd = command + ['-c', str(path), '-o', str(obj)]
        result = subprocess.run(cmd, cwd=entry['directory'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (BUILD / (path.stem + '-native.log')).write_text(result.stdout)
        results.append({'source': name, 'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'command': cmd, 'returncode': result.returncode})
        print(name, result.returncode)
        if result.returncode:
            print(result.stdout[-2200:])
    (NOTES / 'native-compiles.json').write_text(json.dumps(results, indent=2) + '\n')
    shutil.copytree(STAGE, NOTES / 'native', dirs_exist_ok=True)
    native = ROOT / 'pc-port/src/Game/LiveActor/LiveActor.cpp'
    current = native.read_text()
    start = current.index('void LiveActor::initEffectKeeper(')
    end = current.index('\n}', start) + 2
    body = (STAGE / 'OriginalLiveActorEffectInit.cpp').read_text().split('\n\n', 1)[1].rstrip()
    replaced = current[:start] + body + current[end:]
    replaced = '#include "Game/LiveActor/EffectKeeper.hpp"\n' + replaced
    patch = ''.join(difflib.unified_diff(current.splitlines(True), replaced.splitlines(True), 'a/pc-port/src/Game/LiveActor/LiveActor.cpp', 'b/pc-port/src/Game/LiveActor/LiveActor.cpp'))
    (NOTES / 'liveactor-owner-proposal.patch').write_text(patch)
    nm = ['/opt/homebrew/opt/llvm/bin/llvm-nm', '--undefined-only', '--demangle']
    nm += [str(BUILD / (Path(e['source']).stem + '-native.o')) for e in results if e['returncode'] == 0]
    (NOTES / 'direct-undefined.txt').write_text(subprocess.check_output(nm, text=True))

if __name__ == '__main__':
    main()
