from pathlib import Path

def edit(path,replacements):
 p=Path(path);s=p.read_text()
 for a,b in replacements:
  assert a in s,(path,a)
  s=s.replace(a,b)
 p.write_text(s)
def include(path,line):
 p=Path(path);s=p.read_text()
 if line+'\n' not in s:p.write_text(line+'\n'+s)

p='src/Game/AudioLib/AudWrap.cpp'
include(p,'#include "Game/System/AudSystemWrapper.hpp"')
include(p,'#include <aurora/exception.hpp>')
include(p,'#include <stdexcept>')
include(p,'#include <string>')
edit(p,[('namespace AudWrap {','''namespace {
    template <typename T>
    T* requireOwner(T* owner, const char* name) {
        if (owner == nullptr) {
            aurora::throw_host_exception<std::logic_error>(std::string("Audio owner is unavailable: ") + name);
        }
        return owner;
    }

    AudSystemWrapper* getWrapper() {
        return requireOwner(AudSystemWrapper::getCurrent(), "AudSystemWrapper");
    }
}

namespace AudWrap {'''),
('return AudSystem::msBasic;', 'return requireOwner(AudSystem::msBasic, "AudSystem");'),
('return getSystem()->mSceneMgr;', 'return requireOwner(getWrapper()->getSceneMgr(), "AudSceneMgr");'),
('return &getSystem()->mBgmMgr;', 'return requireOwner(getWrapper()->getBgmMgr(), "AudBgmMgr");'),
('return getSystem()->mSystemSeObject;', 'return requireOwner(getWrapper()->getSystemSeObject(), "system sound object");'),
('return getSystem()->mSoundObjHolder;', 'return requireOwner(getWrapper()->getSoundObjHolder(), "AudSoundObjHolder");')])

p='src/Game/Util/SoundUtil.cpp'
edit(p,[('#include "compat/DisabledObjectAudioService.hpp"\n',''),
('''        if (auto* output = aurora::audio::disabled_object_audio_service()) {
            output->register_limited_sound(id, param2);
            return;
        }''','''        if (AudSystemWrapper::isOutputDisabled()) {
            return;
        }'''),
('''        if (auto* output = aurora::audio::disabled_object_audio_service()) {
            output->set_sound_volume_setting(param1, param2);
            return;
        }''','''        if (AudSystemWrapper::isOutputDisabled()) {
            return;
        }'''),
('''        if (auto* output = aurora::audio::disabled_object_audio_service()) {
            output->recover_sound_volume_setting(param1);
            return;
        }''','''        if (AudSystemWrapper::isOutputDisabled()) {
            return;
        }'''),
('auto* output = aurora::audio::disabled_object_audio_service()', 'auto* output = AudSystemWrapper::getCurrent()'),
('output->set_trigger_sound_permitted(', 'output->setTriggerSePermitted('),
('output->set_level_sound_permitted(', 'output->setLevelSePermitted('),
('output->is_sound_permitted()', 'output->isSePermitted()')])

for p in ['src/Game/Screen/GamePauseSequence.cpp','src/Game/Screen/THPSimplePlayerWrapper.cpp','src/Game/Scene/GameScenePauseControl.cpp','src/Game/System/GameSystemErrorWatcher.cpp']:
 edit(p,[('#include "compat/DisabledObjectAudio.hpp"','#include "Game/System/AudSystemWrapper.hpp"'),
 ('if constexpr (aurora::audio::DisabledObjectAudio::enabled())', 'if (!AudSystemWrapper::isOutputDisabled())')])
 if p.endswith('THPSimplePlayerWrapper.cpp'):
  edit(p,[('mAudioExist = aurora::audio::DisabledObjectAudio::enabled();','mAudioExist = !AudSystemWrapper::isOutputDisabled();')])

p='src/Game/Scene/GameSceneScenarioOpeningCameraState.cpp'
include(p,'#include "Game/System/AudSystemWrapper.hpp"')
edit(p,[('            AudWrap::getSystem()->set830(30);','            if (!AudSystemWrapper::isOutputDisabled()) {\n                AudWrap::getSystem()->set830(30);\n            }')])
p='src/Game/Screen/HomeButtonLayout.cpp'
include(p,'#include "Game/System/AudSystemWrapper.hpp"')
s=Path(p).read_text()
for call in ['AudWrap::getSystem()->enterHomeButtonMenu();','AudWrap::getSystem()->preProcessToReset();','AudWrap::getSystem()->exitHomeButtonMenu();','SpkSystem::reconnect(-1);']:
 lines=s.splitlines(True);s=''
 for line in lines:
  if line.strip()==call:
   indent=line[:len(line)-len(line.lstrip())];s+=indent+'if (!AudSystemWrapper::isOutputDisabled()) {\n'+indent+'    '+call+'\n'+indent+'}\n'
  else:s+=line
Path(p).write_text(s)

for name in ['AudioFacadeCompat.cpp','AudioFacadeCompat.hpp','DisabledObjectAudio.cpp','DisabledObjectAudio.hpp','OriginalObjectSoundState.cpp','OriginalAudioVolumeController.cpp']:
 Path('src/compat',name).unlink()
print('Canonical Game audio boundaries applied; six compat files removed')
