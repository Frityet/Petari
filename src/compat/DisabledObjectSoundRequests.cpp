#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/SoundUtil.hpp"

// Native object-audio boundary: disabled voices have no concrete lifetime.
// Keep the original request/condition/return logic; replace only the SDK
// JAISound dereference with its disabled backend operation.
namespace aurora::audio {
void update_disabled_object_sound_lifetime(JAISoundHandle*, u32) noexcept {}
}
namespace MR {
    JAISoundHandle* startSoundObjectLevel(AudSoundObject* pSoundObject, JAISoundID id, s32 param3) {
        JAISoundHandle* pSoundHandle = pSoundObject->startLevelSound(id);

        if (param3 > 0 && pSoundHandle != nullptr) {
            aurora::audio::update_disabled_object_sound_lifetime(pSoundHandle, param3);
        }

        return pSoundHandle;
    }

    JAISoundHandle* startSoundObjectLevelParam(AudSoundObject* pSoundObject, JAISoundID id, s32 param3, s32 param4, s32 param5) {
        JAISoundHandle* pSoundHandle = pSoundObject->startLevelSoundParam(id, param3, param4);

        if (param5 > 0 && pSoundHandle != nullptr) {
            aurora::audio::update_disabled_object_sound_lifetime(pSoundHandle, param5);
        }

        return pSoundHandle;
    }
}
