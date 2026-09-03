#!/usr/bin/env python3
"""Stage literal original owner accessors; do not publish native providers."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PC = ROOT / 'pc-port'
BUILD = ROOT / 'build/original-model-manager-render-20260903'
STAGE = BUILD / 'staged'
NM = '/opt/homebrew/opt/llvm/bin/llvm-nm'

UNITS = {
    'OriginalModelAccess': {
        'source': 'src/Game/Util/ModelUtil.cpp',
        'names': '''updateModelManager calcAnimModelManager updateModelAnimPlayer
            invalidateMtxCalc invalidateJointCallback getJ3DModel calcJ3DModel
            getJ3DModelData getBckFrameMax getBrkFrameMax getBvaFrameMax
            isBckPlaying getMaterialNo getMaterial getMaterialNum getMaterialName
            updateModelDiffDL isEnvelope hideMaterial showMaterial getResTIMG
            copyJointAnimation syncJointAnimation syncMaterialAnimation'''.split(),
        # Parent owns these two material lookup helpers and the archive-name
        # model loader. Only actual actor/model pointer accessors belong here.
        'exclude_headers': [
            'u16 getMaterialNo(J3DModelData* pModelData, const char* pMaterialName)',
            'J3DMaterial* getMaterial(J3DModelData* pModelData, const char* pMaterialName)',
            'J3DModelData* getJ3DModelData(const char* pName)',
        ],
        'includes': '''Game/Util/ModelUtil.hpp
Game/Util/LiveActorUtil.hpp
Game/Util/MutexHolder.hpp
Game/Animation/XanimePlayer.hpp
Game/Animation/XanimeResource.hpp
Game/LiveActor/LiveActor.hpp
Game/LiveActor/ModelManager.hpp
Game/System/ResourceHolder.hpp
JSystem/J3DGraphAnimator/J3DJoint.hpp
JSystem/J3DGraphBase/J3DMaterial.hpp
JSystem/J3DGraphBase/J3DTexture.hpp'''.splitlines(),
    },
    'OriginalActorModelAccess': {
        'source': 'src/Game/Util/LiveActorUtil.cpp',
        'names': '''calcAnimDirect setBaseTRMtx setBaseScale getResourceHolder
            getModelResourceHolder getTexFromModel getTexFromArc getModelResName
            isExistAnim isExistBck isExistBtk isExistBrk isExistBtp isExistBpk
            isExistBva isExistTexture newDifferedDLBuffer initDLMakerFog
            isBckStopped isBtkStopped isBrkStopped isBtpStopped isBpkStopped
            isBvaStopped isBckOneTimeAndStopped isBrkOneTimeAndStopped isBckLooped
            checkPassBckFrame setBckFrameAndStop setBtkFrameAndStop setBrkFrameAndStop
            setBtpFrameAndStop setBpkFrameAndStop setBvaFrameAndStop
            setBrkFrameEndAndStop startBtkAndSetFrameAndStop
            startBrkAndSetFrameAndStop startBtpAndSetFrameAndStop
            startBtk startBrk startBtp startBpk startBva
            startBtkIfExist startBrkIfExist startBtpIfExist startBpkIfExist startBvaIfExist
            isBtkPlaying isBrkPlaying isBtpPlaying isBpkPlaying isBvaPlaying
            isBckExist isBtkExist isBrkExist isBpkExist isBtpExist isBvaExist
            stopBck stopBtk stopBrk stopBtp stopBva
            setBckRate setBtkRate setBrkRate setBvaRate
            setBckFrame setBtkFrame setBrkFrame setBtpFrame setBpkFrame setBvaFrame
            isBckPlaying getBckCtrl getBtkCtrl getBrkCtrl getBtpCtrl getBpkCtrl getBvaCtrl
            updateMaterial initJointTransform getJointTransform setJointTransformLocalMtx
            getBckFrame getBrkFrame getBtpFrame getBvaFrame getBckRate
            getBckFrameMax getBtkFrameMax getBrkFrameMax stopAnimFrame releaseAnimFrame'''.split(),
        'includes': '''Game/Util/LiveActorUtil.hpp
Game/Util/ModelUtil.hpp
Game/Animation/XanimeCore.hpp
Game/Animation/XanimePlayer.hpp
Game/LiveActor/LiveActor.hpp
Game/LiveActor/ModelManager.hpp
Game/LiveActor/DisplayListMaker.hpp
Game/System/ResourceHolder.hpp
JSystem/J3DGraphBase/J3DTexture.hpp'''.splitlines(),
        'preamble': 'namespace {\n    f32 sAnimRateScale = 1.0f;\n}\n',
    },
    'OriginalJointAccess': {
        'source': 'src/Game/Util/JointUtil.cpp',
        'names': '''getJoint getJointMtx getJointIndex getJointName getJointNum
            isExistJoint copyJointPos copyJointScale hideJoint hideJointAndChildren
            showJoint showJointAndChildren getJointTransX getJointTransY getJointTransZ'''.split(),
        'includes': '''Game/Util/JointUtil.hpp
Game/Util/ModelUtil.hpp
JSystem/J3DGraphBase/J3DMaterial.hpp
JSystem/J3DGraphAnimator/J3DJoint.hpp
JSystem/J3DGraphAnimator/J3DModel.hpp
JSystem/J3DGraphAnimator/J3DModelData.hpp
JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp
JSystem/J3DGraphBase/J3DShape.hpp
JSystem/JGeometry.hpp
JSystem/JUtility/JUTNameTab.hpp'''.splitlines(),
    },
    'OriginalActorAnimationStart': {
        'source': 'src/Game/Util/LiveActorUtil.cpp',
        'names': '''startAction isActionEnd isActionStart tryStartAction startAllAnim
            tryStartAllAnim startBck startBckWithInterpole startBckNoInterpole
            startBckAtFirstStep tryStartBck tryStartBckAndBtp startBckIfExist
            setAllAnimFrame setAllAnimFrameAndStop setAllAnimFrameAtEnd
            isAnyAnimStopped isAnyAnimOneTimeAndStopped'''.split(),
        'includes': '''Game/Util/LiveActorUtil.hpp
Game/Util/ModelUtil.hpp
Game/Util/SoundUtil.hpp
Game/Util/NerveUtil.hpp
Game/LiveActor/ActorAnimKeeper.hpp
Game/LiveActor/EffectKeeper.hpp
Game/LiveActor/LiveActor.hpp
Game/LiveActor/ModelManager.hpp
Game/System/ResourceHolder.hpp'''.splitlines(),
        'preamble_from_function': 'changeBckForEffectKeeper',
    },
    'OriginalEffectBckNotification': {
        'includes': ['Game/LiveActor/EffectKeeper.hpp', 'Game/Effect/SyncBckEffectChecker.hpp'],
        'literal_methods': [
            ('src/Game/LiveActor/EffectKeeper.cpp', 'void EffectKeeper::changeBck()'),
            ('src/Game/Effect/SyncBckEffectChecker.cpp', 'void SyncBckEffectChecker::reset()'),
        ],
    },
    'OriginalActorBasStart': {
        'source': 'src/Game/Util/SoundUtil.cpp',
        'names': ['startBas'],
        'includes': '''Game/Util/SoundUtil.hpp
Game/Util/LiveActorUtil.hpp
Game/LiveActor/LiveActor.hpp
Game/AudioLib/AudAnmSoundObject.hpp
Game/System/ResourceHolder.hpp'''.splitlines(),
        'root_header_fallback': True,
    },
}


def functions(text, names):
    pattern = re.compile(r'^    ([A-Za-z_][^;\n{}()]*?\b(' + '|'.join(names) + r')\([^;{}]*?\))\s*\{', re.M)
    result = []
    for match in pattern.finditer(text):
        end = text.index('\n    }', match.end()) + len('\n    }')
        result.append((match.group(1), text[match.start():end], text.count('\n', 0, match.start()) + 1))
    found = {re.search(r'\b(\w+)\(', header).group(1) for header, _, _ in result}
    assert set(names) <= found, set(names) - found
    return result


def symbol_rows(path, defined_only=False):
    args = [NM, '--extern-only', '--format=posix']
    if defined_only:
        args.append('--defined-only')
    text = subprocess.check_output([*args, str(path)], text=True)
    member = None
    for line in text.splitlines():
        if line.endswith(':'):
            member = line[:-1]
        fields = line.split()
        if len(fields) >= 2:
            yield fields[0], fields[1], member


def main():
    STAGE.mkdir(parents=True, exist_ok=True)
    interrupt_header = STAGE / 'revolution/os/OSInterrupt.h'
    interrupt_header.parent.mkdir(parents=True, exist_ok=True)
    interrupt_header.write_text('#pragma once\n#include <dolphin/os/OSInterrupt.h>\n')
    # Preserve the full sound-object class layout. Its unrelated inline system
    # query can be defined out of line when that real system owner is imported;
    # the BAS notification needs neither AudSystem nor AudAudience headers.
    sound_header = ROOT / 'include/Game/AudioLib/AudSoundObject.hpp'
    sound_text = sound_header.read_text()
    sound_text = sound_text.replace('#include "Game/AudioLib/AudSystem.hpp"\n', '')
    sound_text = sound_text.replace('#include "Game/AudioLib/AudWrap.hpp"\n', '')
    inline_query = '''    bool isEnableStartSound(JAISoundID soundID) {
        return AudWrap::getSystem()->isEnableStartSound(soundID);
    }'''
    assert inline_query in sound_text
    sound_text = sound_text.replace(inline_query, '    bool isEnableStartSound(JAISoundID soundID);')
    target = STAGE / 'Game/AudioLib/AudSoundObject.hpp'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(sound_text)
    source_hashes = {}
    imports = []
    for header in ['Game/LiveActor/EffectKeeper.hpp', 'Game/LiveActor/ActorAnimKeeper.hpp',
                   'Game/Effect/SyncBckEffectChecker.hpp']:
        source = ROOT / 'include' / header
        target = STAGE / header
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(source.read_bytes())
        source_hashes[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
    for unit, config in UNITS.items():
        if 'literal_methods' in config:
            output = ''.join('#include "' + header + '"\n' for header in config['includes']) + '\n'
            for relative, signature in config['literal_methods']:
                path = ROOT / relative
                source = path.read_text()
                start = source.index(signature + ' {')
                end = source.index('\n}', start) + len('\n}')
                body = source[start:end]
                output += body + '\n\n'
                source_hashes[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
                imports.append({'unit': unit, 'header': signature, 'source': relative,
                                'line': source.count('\n', 0, start) + 1,
                                'literal_sha256': hashlib.sha256(body.encode()).hexdigest()})
            (STAGE / (unit + '.cpp')).write_text(output)
            continue
        path = ROOT / config['source']
        source = path.read_text()
        source_hashes[config['source']] = hashlib.sha256(path.read_bytes()).hexdigest()
        selected = [(header, body, line) for header, body, line in functions(source, config['names'])
                    if header not in config.get('exclude_headers', [])]
        output = ''.join('#include "' + header + '"\n' for header in config['includes'])
        preamble = config.get('preamble', '')
        if 'preamble_from_function' in config:
            name = config['preamble_from_function']
            # Root keeps this notification helper TU-local and marks it NO_INLINE.
            start = source.index('    void ' + name + '(')
            end = source.index('\n    }', start) + len('\n    }')
            preamble += 'namespace {\n' + source[start:end] + '\n}\n'
        output += '\n' + preamble + '\nnamespace MR {\n\n'
        for header, body, line in selected:
            output += body + '\n\n'
            imports.append({'unit': unit, 'header': header, 'source': config['source'],
                            'line': line, 'literal_sha256': hashlib.sha256(body.encode()).hexdigest()})
        output += '}  // namespace MR\n'
        (STAGE / (unit + '.cpp')).write_text(output)
    # This value is original TU-local source state, not a new playback override.
    assert '    f32 sAnimRateScale = 1.0f;' in (ROOT / UNITS['OriginalActorModelAccess']['source']).read_text()
    base = next(row['arguments'] for row in json.loads((PC / 'compile_commands.json').read_text())
                if row['file'] == 'src/Game/System/ResourceHolder.cpp')
    args = []
    skip = False
    for arg in base:
        if skip:
            skip = False
        elif arg == '-o':
            skip = True
        elif arg not in ('-c', 'src/Game/System/ResourceHolder.cpp'):
            args.append(arg)
    args[1:1] = ['-I' + str(STAGE), '-I' + str(ROOT / 'build/original-model-manager-native-20260903')]

    def compile_one(unit):
        extra = []
        if UNITS[unit].get('root_header_fallback'):
            extra = ['-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'libs/JSystem/include')]
        command = [*args, *extra, '-c', str(STAGE / (unit + '.cpp')), '-o', str(BUILD / (unit + '.o'))]
        result = subprocess.run(command, cwd=PC, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (HERE / (unit + '.compile.log')).write_text(result.stdout)
        print(unit + ': ' + str(result.returncode), flush=True)
        return {'unit': unit, 'command': command, 'returncode': result.returncode}

    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(compile_one, UNITS))
    (HERE / 'access-native-evidence.json').write_text(json.dumps({
        'source_sha256': source_hashes, 'imports': imports, 'results': results}, indent=2) + '\n')
    assert all(row['returncode'] == 0 for row in results), 'See per-unit compile logs'
    providers = {}
    for name in ['smg-pc-game', 'smg-pc-common', 'smg-pc-render']:
        library = PC / 'build/macosx/arm64/debug' / ('lib' + name + '.a')
        for symbol, _, member in symbol_rows(library, True):
            providers.setdefault(symbol, []).append({'archive': name, 'member': member})
    definitions, undefined = {}, set()
    for unit in UNITS:
        for symbol, kind, _ in symbol_rows(BUILD / (unit + '.o')):
            if kind == 'U':
                undefined.add(symbol)
            elif kind == 'T':
                definitions[symbol] = unit
    symbols = sorted(set(definitions) | undefined)
    demangled = subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-cxxfilt', '--strip-underscore', *symbols], text=True).splitlines()
    pretty = dict(zip(symbols, demangled))
    collisions = [{'symbol': symbol, 'readable': pretty[symbol], 'unit': unit,
                   'existing_providers': providers[symbol]}
                  for symbol, unit in definitions.items() if symbol in providers and symbol.startswith('__ZN2MR')]
    external = [{'symbol': symbol, 'readable': pretty[symbol], 'existing_providers': providers.get(symbol, [])}
                for symbol in sorted(undefined - definitions.keys())]
    (HERE / 'access-symbol-evidence.json').write_text(json.dumps({'collisions': collisions, 'external': external}, indent=2) + '\n')
    print('Literal imports: ' + str(len(imports)))
    print('Existing MR provider collisions: ' + str(len(collisions)))
    for row in external:
        if not row['existing_providers'] and not row['readable'].startswith(('ModelManager::', 'DisplayListMaker::')):
            print('External not supplied by current game/common/render archives: ' + row['readable'])


if __name__ == '__main__':
    main()
