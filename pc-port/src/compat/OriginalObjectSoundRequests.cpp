#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace MR {
    JAISoundHandle* startSystemSE(JAISoundID id, s32 param2, s32 param3) {
        return AudWrap::getSystemSeObject()->startSoundParam(id, param2, param3);
    }

    JAISoundHandle* startSystemLevelSE(JAISoundID id, s32 param2, s32 param3) {
        return AudWrap::getSystemSeObject()->startLevelSoundParam(id, param2, param3);
    }

    void stopSystemSE(JAISoundID id, u32 param2) {
        AudWrap::getSystemSeObject()->stopSound(id, param2);
    }

    JAISoundHandle* startSound(const LiveActor* pActor, JAISoundID id, s32 param3, s32 param4) {
        pActor->mSoundObject->setMapCode(getMapSoundCodeFoot(pActor));

        if (param3 != -1) {
            return pActor->mSoundObject->startSoundParam(id, param3, param4);
        } else {
            return pActor->mSoundObject->startSound(id);
        }
    }

    JAISoundHandle* startLevelSound(const LiveActor* pActor, JAISoundID id, s32 param3, s32 param4, s32 param5) {
        pActor->mSoundObject->setMapCode(getMapSoundCodeFoot(pActor));

        if (param3 != -1) {
            return startSoundObjectLevelParam(pActor->mSoundObject, id, param3, param4, param5);
        } else {
            return startSoundObjectLevel(pActor->mSoundObject, id, param5);
        }
    }

    void stopSound(const LiveActor* pActor, JAISoundID id, u32 param3) {
        pActor->mSoundObject->stopSound(id, param3);
    }

    void setSeVersion(const LiveActor* pActor, u32 version) {
        pActor->mSoundObject->mSeVersion = version;
    }

    void setMapSondCodeGravity(const LiveActor* pActor, s32 code) {
        if (pActor->mSoundObject == nullptr) {
            return;
        }

        pActor->mSoundObject->setMapCodeExtra(code);
    }
}
