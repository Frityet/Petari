from pathlib import Path
import hashlib,json,re
root=Path('notes/compat-owner-removal-round21-20260925/audio-game')
p=Path('src/Game/AudioLib/AudSystem.cpp');raw=p.read_bytes();q=root/'before'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(raw)
m=json.loads((root/'owned-manifest.json').read_text());m['paths'].append({'path':str(p),'existed_before':True,'before_sha256':hashlib.sha256(raw).hexdigest()});(root/'owned-manifest.json').write_text(json.dumps(m,indent=2)+'\n')
s=raw.decode();_,original=s.split('#else\n',1)
def donor(name):
 pat=re.compile(r'^.*\bAudSystem::'+name+r'\([^\n]*\)(?: const)? \{',re.M)
 match=pat.search(original);assert match,name
 start=match.start();brace=original.index('{',match.start());level=1;i=brace+1
 while level:
  if original[i]=='{':level+=1
  if original[i]=='}':level-=1
  i+=1
 return original[start:i]+'\n'
cheap=['isSubmitSeByVolumeSetting','isHomeButtonMenuActive','isPauseMenuActive','getNumOfPlaying','getPlayCountMin','setMicMtx','getMicPos','setFarCamera','setSeVolumeSet','recoverSeVolumeSet','setSeVolumeSetLevel','clearAllLimitedSound','isRegisteredLimitedSound','updateLimitedSound','isAlreadyPlayingSoundNear']
head='''#if defined(TARGET_PC)
#include "Game/AudioLib/AudSystem.hpp"
#include "Game/AudioLib/AudSoundId.hpp"
#include "Game/AudioLib/AudSystemVolumeController.hpp"
#include "Game/System/AudSystemWrapper.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>
#include <string>

// No native AudSystem is constructed while output is disabled. Restore its
// ordinary field/list operations; hardware/rhythm entry points reject access
// until their actual owners exist.
AudSystem* AudSystem::msBasic;

namespace {
    [[noreturn]] void unavailable(const char* operation) {
        aurora::throw_host_exception<std::logic_error>(
            std::string("The native audio output does not provide ") + operation + ".");
    }
}

'''
head+='\n'.join(donor(n) for n in cheap)+'\n'
enable=donor('isEnableStartSound').replace('    if (mIsReset) {','    if (AudSystemWrapper::isOutputDisabled()) {\n        return false;\n    }\n    if (mIsReset) {')
head+=enable+'\n'
head+='AudChordInfo* AudSystem::getChordInfo() {\n    unavailable("the original rhythm/chord owner");\n}\n\n'
head+='void AudSystem::registerLimitedSound(JAISoundID, s32) {\n    unavailable("the original limited-sound playback owner");\n}\n\n'
for name,operation in [('preProcessToReset','audio reset preparation'),('pause','audio pause'),('unpause','audio resume'),('enterHomeButtonMenu','home-menu audio'),('exitHomeButtonMenu','home-menu audio'),('enterPauseMenu','pause-menu audio'),('exitPauseMenu','pause-menu audio'),('doDvdErrorProcess','DVD-error audio'),('exitDvdErrorProcess','DVD-error audio'),('initSceneVolume','scene output initialization')]:
 head+=f'void AudSystem::{name}() {{\n    unavailable("{operation}");\n}}\n\n'
p.write_text(head+'#else\n'+original)
print('Restored',len(cheap)+1,'original field/list operations on AudSystem; explicit absent output owner methods retained')
