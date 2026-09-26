#include "Game/AudioLib/AudWrap.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudRemixMgr.hpp"
#include "Game/AudioLib/AudSoundInfo.hpp"
#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include "Game/AudioLib/AudSystem.hpp"
#include "Game/RhythmLib/AudRhythmWrap.hpp"
#include "Game/System/AudSystemWrapper.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>
#include <string>

namespace {
    template < typename T >
    T* requireOwner(T* owner, const char* name) {
        if (owner == nullptr) {
            aurora::throw_host_exception< std::logic_error >(std::string("Audio owner is unavailable: ") + name);
        }
        return owner;
    }

    AudSystemWrapper* getWrapper() {
        return requireOwner(AudSystemWrapper::getCurrent(), "AudSystemWrapper");
    }
}  // namespace

namespace AudWrap {
    AudSystem* getSystem() {
        return requireOwner(AudSystem::msBasic, "AudSystem");
    }

    AudSoundInfo* getSoundInfo() {
        return AudSoundInfo::getInstance();
    }

    AudSceneMgr* getSceneMgr() {
        return requireOwner(getWrapper()->getSceneMgr(), "AudSceneMgr");
    }

    AudBgmMgr* getBgmMgr() {
        return requireOwner(getWrapper()->getBgmMgr(), "AudBgmMgr");
    }

    AudBgm* getStageBgm() {
        return getBgmMgr()->mBgm[AudBgmMgr::BgmType_Stage];
    }

    AudBgm* getSubBgm() {
        return getBgmMgr()->mBgm[AudBgmMgr::BgmType_Sub];
    }

    JAISoundHandle* startStageBgm(u32 soundID, bool lock) {
        return getBgmMgr()->start(AudBgmMgr::BgmType_Stage, soundID, lock);
    }

    JAISoundHandle* startSubBgm(u32 soundID, bool lock) {
        return getBgmMgr()->start(AudBgmMgr::BgmType_Sub, soundID, lock);
    }

    void setNextIdStageBgm(u32 soundID) {
        getBgmMgr()->setNextBGM(AudBgmMgr::BgmType_Stage, soundID);
    }

    JAISoundHandle* startLastStageBgm() {
        return getBgmMgr()->startLastBGM(AudBgmMgr::BgmType_Stage);
    }

    AudSoundObject* getSystemSeObject() {
        return requireOwner(getWrapper()->getSystemSeObject(), "system sound object");
    }

    AudSoundObject* getAtmosphereSeObject() {
        return getSystem()->mAtmosphereSeObject;
    }

    AudSoundObjHolder* getSoundObjHolder() {
        return requireOwner(getWrapper()->getSoundObjHolder(), "AudSoundObjHolder");
    }

    AudRhythmMeSystem* getRhythmMeSystem() {
        return getSystem()->mRhythmMeSystem;
    }

    AudMeObject* getSystemMeObject() {
        return getSystem()->mSystemMeObject;
    }

    AudRemixMgr* getRemixMgr() {
        return requireOwner(getWrapper()->getRemixMgr(), "AudRemixMgr");
    }

    AudRemixSequencer* getRemixSequencer() {
        return getRemixMgr()->mRemixSeq;
    }

    AudSoundObject* getRemixSeqObject() {
        return getRemixMgr()->mSoundObj;
    }

    AudSoundNameConverter* getSoundNameConverter() {
        return AudSoundNameConverter::get();
    }
};  // namespace AudWrap
