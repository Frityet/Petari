#pragma once

#include <revolution/types.h>
#include <JSystem/JAudio2/JAUSoundTable.hpp>
#include <memory>
#include <vector>

class AudSystem;
class AudSceneMgr;
class AudBgmMgr;
class AudSoundObject;
class AudSoundObjHolder;
class AudSoundNameConverter;
class AudRemixMgr;
class JKRExpHeap;
class JKRHeap;
class JKRMemArchive;
class JKRSolidHeap;
class JAIStreamMgr;
namespace aurora::audio {
    class PcmAudioMixer;
    class JAudioSoundArchive;
}

class AudSystemWrapper {
public:
    AudSystemWrapper(JKRSolidHeap*, JKRHeap*);
#if defined(TARGET_PC)
    ~AudSystemWrapper();
    static AudSystemWrapper* getCurrent() noexcept;
    static bool isOutputDisabled();
    AudSceneMgr* getSceneMgr() const noexcept;
    AudBgmMgr* getBgmMgr() const noexcept;
    AudSoundObject* getSystemSeObject() const noexcept;
    AudSoundObjHolder* getSoundObjHolder() const noexcept;
    AudRemixMgr* getRemixMgr() const noexcept;
    void setTriggerSePermitted(bool) noexcept;
    void setLevelSePermitted(bool) noexcept;
    bool isSePermitted() const noexcept;
#endif

    void requestResourceForInitialize();
    void createAudioSystem();
    void createSoundNameConverter();
    void updateRhythm();
    void movement();
    void stopAllSound(u32);
    bool isLoadDoneWaveDataAtSystemInit() const;
    void loadStaticWaveData();
    bool isLoadDoneStaticWaveData() const;
    void loadStageWaveData(const char*, const char*, bool);
    bool isLoadDoneStageWaveData() const;
    void loadScenarioWaveData(const char*, const char*, s32);
    bool isLoadDoneScenarioWaveData() const;
    bool isPermitToReset() const;
    void prepareReset();
    void requestReset(bool);
    bool isResetDone();
    void resumeReset();
    void receiveResourceForInitialize();

    /* 0x00 */ AudSystem* mAudSystem;
    /* 0x04 */ JKRSolidHeap* _4;
    /* 0x08 */ JKRHeap* _8;
    /* 0x0C */ void* mSmrRes;
    /* 0x10 */ JKRMemArchive* mJaiSeqRes;
    /* 0x14 */ JKRMemArchive* mJaiCordRes;
    /* 0x18 */ JKRMemArchive* mJaiMeRes;
    /* 0x1C */ JKRMemArchive* mJaiRemixSeqRes;
    /* 0x20 */ JKRExpHeap* mSpkHeap;
    /* 0x24 */ JKRMemArchive* mSpkRes;
    /* 0x28 */ bool _28;
    /* 0x29 */ bool _29;
    /* 0x2A */ bool _2A;
#if defined(TARGET_PC)
private:
    enum class InitializePhase { Created, Requested, Received, Initialized };
    void releaseResources() noexcept;

    // Bytes precede all original borrowers; destruction releases the converter
    // and its arrays before the native table and decoded byte storage.
    std::vector< u8 > mSoundNameBytes;
    std::vector< u32 > mRemixSequenceWords;
    JAUSoundNameTable mSoundNameTable{false};
    JAUSoundNameTable* mPreviousNameTable = nullptr;
    AudSoundNameConverter* mPreviousNameConverter = nullptr;
    std::unique_ptr< AudSoundNameConverter > mSoundNameConverter;
    std::shared_ptr< aurora::audio::PcmAudioMixer > mStreamMixer;
    std::shared_ptr< aurora::audio::JAudioSoundArchive > mStreamArchive;
    std::unique_ptr< JAIStreamMgr > mStreamMgr;
    std::unique_ptr< AudSceneMgr > mSceneMgr;
    std::unique_ptr< AudBgmMgr > mBgmMgr;
    std::unique_ptr< AudSoundObjHolder > mSoundObjHolder;
    std::unique_ptr< AudSoundObject > mSystemSeObject;
    std::unique_ptr< AudRemixMgr > mRemixMgr;
    InitializePhase mInitializePhase = InitializePhase::Created;
    bool mStaticWaveRequested = false;
    bool mStageWaveRequested = false;
    bool mScenarioWaveRequested = false;
    bool mResetRequested = false;
    bool mTriggerSePermitted = true;
    bool mLevelSePermitted = true;
#endif
};
