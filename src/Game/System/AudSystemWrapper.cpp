#include "Game/System/AudSystemWrapper.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudMeNameConverter.hpp"
#include "Game/AudioLib/AudMicWrap.hpp"
#include "Game/AudioLib/AudParams.hpp"
#include "Game/AudioLib/AudRemixMgr.hpp"
#include "Game/AudioLib/AudSceneMgr.hpp"
#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include "Game/AudioLib/AudSoundObjHolder.hpp"
#include "Game/AudioLib/AudSoundObject.hpp"
#include "Game/AudioLib/AudSpeakerWrap.hpp"
#include "Game/AudioLib/AudSystem.hpp"
#include "Game/AudioLib/CSSoundNameConverter.hpp"
#include "Game/RhythmLib/AudRhythmMeSystem.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "resource/AudioInfoResource.hpp"
#include "runtime/JasAudioDriver.hpp"
#include <JSystem/JAudio2/JAIStreamMgr.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JKernel/JKRMemArchive.hpp>
#include <JSystem/JKernel/JKRSolidHeap.hpp>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <cstdio>
#include <new>
#include <stdexcept>

namespace {
    void retireWrapper(void* object) noexcept {
        static_cast< AudSystemWrapper* >(object)->~AudSystemWrapper();
    }
}  // namespace

AudSystemWrapper::AudSystemWrapper(JKRSolidHeap* audioHeap, JKRHeap* resourceHeap)
    : mAudSystem(nullptr), _4(audioHeap), _8(resourceHeap), mSmrRes(nullptr), mJaiSeqRes(nullptr), mJaiCordRes(nullptr), mJaiMeRes(nullptr),
      mJaiRemixSeqRes(nullptr), mSpkHeap(nullptr), mSpkRes(nullptr), _28(false), _29(false), _2A(false) {
    if (!JKRHeap::findFromRoot(this) || !audioHeap || !resourceHeap) {
        aurora::throw_host_exception< std::logic_error >("An audio wrapper requires its actual original process heaps");
    }
    JKRHeap::registerFinalizer(this, retireWrapper);
}

AudSystemWrapper::~AudSystemWrapper() {
    JKRHeap::unregisterFinalizer(this);
    mInitializePhase = InitializePhase::Created;
    releaseResources();
}

void AudSystemWrapper::requestResourceForInitialize() {
    if (mInitializePhase != InitializePhase::Created) {
        aurora::throw_host_exception< std::logic_error >("Audio initialization was already requested");
    }
    MR::loadAsyncToMainRAM("/AudioRes/SMR.szs", nullptr, _8, JKRDvdRipper::ALLOC_DIRECTION_BACKWARD);
    MR::mountAsyncArchive("/AudioRes/Info/JaiRemixSeq.arc", _4);
    MR::mountAsyncArchive("/AudioRes/Seqs/JaiSeq.arc", _4);
    MR::mountAsyncArchive("/AudioRes/Info/JaiChord.arc", _4);
    MR::mountAsyncArchive("/AudioRes/Info/JaiMe.arc", _4);
    MR::mountAsyncArchive(AudSpeakerWrap::getResName(), _4);
    mInitializePhase = InitializePhase::Requested;
}

void AudSystemWrapper::receiveResourceForInitialize() {
    if (mInitializePhase == InitializePhase::Received) {
        return;
    }
    if (mInitializePhase != InitializePhase::Requested) {
        aurora::throw_host_exception< std::logic_error >("Audio name resources were not requested");
    }
    mSmrRes = MR::receiveFile("/AudioRes/SMR.szs");
    mJaiRemixSeqRes = MR::receiveArchive("/AudioRes/Info/JaiRemixSeq.arc");
    mJaiSeqRes = MR::receiveArchive("/AudioRes/Seqs/JaiSeq.arc");
    mJaiCordRes = MR::receiveArchive("/AudioRes/Info/JaiChord.arc");
    mJaiMeRes = MR::receiveArchive("/AudioRes/Info/JaiMe.arc");
    mSpkRes = MR::receiveArchive(AudSpeakerWrap::getResName());
    if (!mSmrRes) {
        aurora::throw_host_exception< std::runtime_error >("Audio initialization received no name resource");
    }
    if (!mJaiRemixSeqRes) {
        aurora::throw_host_exception< std::runtime_error >("Audio initialization received no remix archive");
    }
    mInitializePhase = InitializePhase::Received;
}

void AudSystemWrapper::createAudioSystem() {
    receiveResourceForInitialize();
    if (_29) {
        OSSuspendThread(OSGetCurrentThread());
    }
    _2A = true;
    try {
        const aurora::allocation::HostAllocationScope host;
        mPreviousNameTable = JAUSoundNameTable::getInstance();
        mInfoResources = std::make_unique< smgpc::resource::AudioInfoResources >(mJaiCordRes, mJaiMeRes, mJaiRemixSeqRes, mSpkRes);
        {
            const MR::CurrentHeapRestorer current(_4);
            const aurora::allocation::ClientAllocationScope game({true, true});
            mAudSystem = AudNewAudSystem(_4, mSmrRes, mJaiSeqRes, mJaiCordRes, mJaiMeRes, mJaiRemixSeqRes);
        }
        mSoundNameTable = JAUSoundNameTable::getInstance();
        mAudSystem->setSpeakerResource(mSpkRes);
        createSoundNameConverter();
        {
            const MR::CurrentHeapRestorer current(JKRHeap::findFromRoot(this));
            const aurora::allocation::ClientAllocationScope game({true, true});
            mMeNameConverter = std::make_unique< AudMeNameConverter >();
            mPreviousMeNameConverter = AudSingletonHolder< AudMeNameConverter >::exchange(mMeNameConverter.get());
            mSpeakerNameConverter = std::make_unique< CSSoundNameConverter >();
            mPreviousSpeakerNameConverter = AudSingletonHolder< CSSoundNameConverter >::exchange(mSpeakerNameConverter.get());
        }
        mInitializePhase = InitializePhase::Initialized;
        AudMicWrap::setMicEnv();
        smgpc::audio::start_dsp();
    } catch (...) {
        releaseResources();
        _2A = false;
        throw;
    }
    _2A = false;
    MR::removeFileConsideringLanguage("/AudioRes/SMR.szs");
    mSmrRes = nullptr;
}

void AudSystemWrapper::createSoundNameConverter() {
    if (mSoundNameConverter) {
        return;
    }
    if (mInitializePhase != InitializePhase::Received || !mSoundNameTable) {
        aurora::throw_host_exception< std::logic_error >("Audio name publication requires its received sound-name table");
    }
    AudSoundNameConverter::validateTable(mSoundNameTable);
    const MR::CurrentHeapRestorer current(JKRHeap::findFromRoot(this));
    const aurora::allocation::ClientAllocationScope game({true, true});
    mSoundNameConverter = std::make_unique< AudSoundNameConverter >();
    mPreviousNameConverter = AudSingletonHolder< AudSoundNameConverter >::exchange(mSoundNameConverter.get());
}

void AudSystemWrapper::releaseResources() noexcept {
    const aurora::allocation::HostAllocationScope host;
    if (mAudSystem) {
        mAudSystem->stopSync();
        smgpc::audio::shutdown_dsp();
        delete mAudSystem;
        mAudSystem = nullptr;
        AudSystem::msBasic = nullptr;
    }
    smgpc::audio::shutdown_dsp();
    if (mMeNameConverter)
        AudSingletonHolder< AudMeNameConverter >::exchange(mPreviousMeNameConverter);
    if (mSpeakerNameConverter)
        AudSingletonHolder< CSSoundNameConverter >::exchange(mPreviousSpeakerNameConverter);
    mMeNameConverter.reset();
    mSpeakerNameConverter.reset();
    mInfoResources.reset();
    if (mSoundNameConverter && AudSingletonHolder< AudSoundNameConverter >::get() == mSoundNameConverter.get()) {
        AudSingletonHolder< AudSoundNameConverter >::exchange(mPreviousNameConverter);
    }
    mSoundNameConverter.reset();
    if (mSoundNameTable && JAUSoundNameTable::sInstance == mSoundNameTable) {
        JAUSoundNameTable::sInstance = mPreviousNameTable;
    }
    mPreviousNameConverter = nullptr;
    mPreviousNameTable = nullptr;
    mSoundNameTable = nullptr;
}

void AudSystemWrapper::updateRhythm() {
    if (mAudSystem)
        mAudSystem->mRhythmMeSystem->rhythmProc();
}

void AudSystemWrapper::movement() {
    if (mInitializePhase != InitializePhase::Initialized) {
        return;
    }
    mAudSystem->frameWork();
    smgpc::audio::check_dsp();
}

void AudSystemWrapper::stopAllSound(u32 frames) {
    if (mAudSystem)
        mAudSystem->stop(frames);
}

bool AudSystemWrapper::isLoadDoneWaveDataAtSystemInit() const {
    if (mAudSystem == nullptr) {
        return false;
    }

    if (mAudSystem->mSceneMgr == nullptr) {
        return false;
    }

    return mAudSystem->mSceneMgr->isLoadDoneSystemInit();
}

void AudSystemWrapper::loadStaticWaveData() {
    if (mAudSystem == nullptr) {
        return;
    }

    if (mAudSystem->mSceneMgr == nullptr) {
        return;
    }

    mAudSystem->mSceneMgr->loadStaticResource();
}

bool AudSystemWrapper::isLoadDoneStaticWaveData() const {
    if (mAudSystem == nullptr) {
        return false;
    }

    if (mAudSystem->mSceneMgr == nullptr) {
        return false;
    }

    return mAudSystem->mSceneMgr->isLoadDoneStaticResource();
}

void AudSystemWrapper::loadStageWaveData(const char* pSceneName, const char* pStageName, bool isPlayerLuigi) {
    if (mAudSystem == nullptr) {
        return;
    }

    if (mAudSystem->mSceneMgr == nullptr) {
        return;
    }

    if (isPlayerLuigi) {
        mAudSystem->mSceneMgr->setPlayerModeLuigi();
    } else {
        mAudSystem->mSceneMgr->setPlayerModeMario();
    }

    mAudSystem->mSceneMgr->loadStageResource(pSceneName, pStageName);
}

bool AudSystemWrapper::isLoadDoneStageWaveData() const {
    if (mAudSystem == nullptr) {
        return false;
    }

    if (mAudSystem->mSceneMgr == nullptr) {
        return false;
    }

    return mAudSystem->mSceneMgr->isLoadDoneStageResource();
}

void AudSystemWrapper::loadScenarioWaveData(const char* pSceneName, const char* pStageName, s32 scenarioNo) {
    if (mAudSystem == nullptr) {
        return;
    }

    if (mAudSystem->mSceneMgr == nullptr) {
        return;
    }

    mAudSystem->mSceneMgr->loadScenarioResource(pSceneName, pStageName, scenarioNo);
}

bool AudSystemWrapper::isLoadDoneScenarioWaveData() const {
    if (mAudSystem == nullptr) {
        return false;
    }

    if (mAudSystem->mSceneMgr == nullptr) {
        return false;
    }

    return mAudSystem->mSceneMgr->isLoadDoneScenarioResource();
}

bool AudSystemWrapper::isPermitToReset() const {
    return !_2A;
}

void AudSystemWrapper::prepareReset() {
    if (mAudSystem == nullptr) {
        _29 = true;
    } else {
        mAudSystem->preProcessToReset();
    }
}

void AudSystemWrapper::requestReset(bool stopThreads) {
    if (mAudSystem == nullptr) {
        _29 = true;
    } else {
        mAudSystem->resetAudio(10, stopThreads);
        mAudSystem->stop(10);
    }
}

bool AudSystemWrapper::isResetDone() {
    if (_29) {
        return true;
    }

    if (mAudSystem == nullptr) {
        return true;
    }

    return mAudSystem->hasReset();
}

void AudSystemWrapper::resumeReset() {
    if (_29) {
        _29 = false;
    }

    if (mAudSystem == nullptr) {
        return;
    }

    return mAudSystem->resumeReset();
}
