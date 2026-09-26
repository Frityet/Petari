from pathlib import Path
import importlib.util
spec=importlib.util.spec_from_file_location('audit','scripts/source_provider_audit.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

def restore(path):
    data=(Path('decomp/src')/path).read_text()
    (Path('src')/path).write_text(data)

def bounds(text,anchor):
    start=text.index(anchor)
    depth=0; entered=False
    for m in audit.TOKEN.finditer(text,start):
        token=m.group()
        if token=='{': depth+=1;entered=True
        elif token=='}':
            depth-=1
            if entered and depth==0:return start,m.end()
    raise ValueError(anchor)

def functions(path,anchors):
    dest=Path('src')/path; native=dest.read_text();donor=(Path('decomp/src')/path).read_text()
    for anchor in anchors:
        a,b=bounds(native,anchor);c,d=bounds(donor,anchor)
        native=native[:a]+donor[c:d]+native[b:]
    native=native.replace('#if defined(TARGET_PC)\n#include "Game/System/AudSystemWrapper.hpp"\n#endif\n','')
    native=native.replace('#include "Game/System/AudSystemWrapper.hpp"\n','')
    dest.write_text(native)

for path in ['Game/AudioLib/AudBgm.cpp','Game/AudioLib/AudBgmMgr.cpp','Game/AudioLib/AudMicWrap.cpp',
             'Game/GameAudio/AudCameraWatcher.cpp','Game/AreaObj/SoundEmitterSphere.cpp','Game/AreaObj/SoundEmitterCube.cpp',
             'Game/Util/SoundUtil.cpp','Game/RhythmLib/AudMeObject.cpp']:
    restore(path)
p=Path('src/Game/Util/SoundUtil.cpp');t=p.read_text()
t=t.replace('startSystemSE(id);','startSystemSE(id, -1, -1);').replace('startSound(pActor, id);','startSound(pActor, id, -1, -1);')
t=t.replace('void limitedSound(const char* pName, s32 param2) NO_INLINE {','void limitedSound(const char* pName, s32 param2) {')
p.write_text(t)
p=Path('src/Game/RhythmLib/AudMeObject.cpp');t=p.read_text().replace('#include "Game/RhythmLib/AudMeObject.hpp"','#include "Game/RhythmLib/AudMeObject.hpp"\n#include "JSystem/JKernel/JKRHeap.hpp"');p.write_text(t)
for path,anchors in {
 'Game/GameAudio/AudEffectDirector.cpp':['void AudEffectDirector::setAudioEffectParam('],
 'Game/Scene/GameScenePauseControl.cpp':['void GameScenePauseControl::exeNormal('],
 'Game/Screen/GamePauseSequence.cpp':['void GamePauseSequence::startPause('],
 'Game/Screen/HomeButtonLayout.cpp':['void HomeButtonLayout::exeActive('],
 'Game/System/GameSystemErrorWatcher.cpp':['void GameSystemErrorWatcher::exeNoError(','void GameSystemErrorWatcher::exeErrorWindowOut('],
 'Game/Scene/GameSceneScenarioOpeningCameraState.cpp':['void GameSceneScenarioOpeningCameraState::exePlay('],
 'Game/AudioLib/AudSoundObject.cpp':['bool AudSoundObject::isLimitedSound('],
}.items():functions(path,anchors)
p=Path('src/Game/Screen/THPSimplePlayerWrapper.cpp');t=p.read_text()
t=t.replace('#include "Game/System/AudSystemWrapper.hpp"\n','')
for call in ['JASDriver::registerMixCallback(nullptr, (JASMixMode)3);','JASDriver::registerMixCallback(THPSimplePlayerStaticAudio::audioCallback, (JASMixMode)3);']:
    old='    if (!AudSystemWrapper::isOutputDisabled()) {\n        '+call+'\n    }'
    assert old in t;t=t.replace(old,'    '+call)
t=t.replace('mAudioExist = !AudSystemWrapper::isOutputDisabled();','mAudioExist = 1;');p.write_text(t)
p=Path('src/Game/AudioLib/AudSoundObject.cpp');t=p.read_text();a,b=bounds(t,'bool AudSoundObject::isEnableStartSound(');p.write_text((t[:a]+t[b:]).rstrip()+'\n')
p=Path('src/Game/AudioLib/AudSoundObject.hpp');t=p.read_text();donor=Path('decomp/include/Game/AudioLib/AudSoundObject.hpp').read_text();a,b=bounds(donor,'bool isEnableStartSound(')
t=t.replace('bool isEnableStartSound(JAISoundID soundID);',donor[a:b]);t=t.replace('#pragma once\n','#pragma once\n\n#include "Game/AudioLib/AudSystem.hpp"\n#include "Game/AudioLib/AudWrap.hpp"');p.write_text(t)
p=Path('src/Game/System/AudSystemWrapper.cpp');t=p.read_text();a=t.index('AudSystemWrapper* AudSystemWrapper::getCurrent()');b=t.index('void AudSystemWrapper::requestResourceForInitialize()',a);t=t[:a]+t[b:];p.write_text(t)
p=Path('src/Game/System/AudSystemWrapper.hpp');t=p.read_text();a=t.index('    static AudSystemWrapper* getCurrent()');b=t.index('#endif',a);t=t[:a]+t[b:];t=t.replace('    bool mTriggerSePermitted = true;\n    bool mLevelSePermitted = true;\n','');p.write_text(t)
