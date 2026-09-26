#include "Game/System/AudSystemWrapper.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudParams.hpp"
#include "Game/AudioLib/AudRemixMgr.hpp"
#include "Game/AudioLib/AudSceneMgr.hpp"
#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include "Game/AudioLib/AudSoundObject.hpp"
#include "Game/AudioLib/AudSoundObjHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "resource/AudioInfoResource.hpp"
#include "runtime/JasAudioDriver.hpp"
#include "Game/AudioLib/AudSystem.hpp"
#include "Game/AudioLib/AudMicWrap.hpp"
#include "Game/RhythmLib/AudRhythmMeSystem.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JKernel/JKRMemArchive.hpp>
#include <JSystem/JKernel/JKRSolidHeap.hpp>
#include <JSystem/JAudio2/JAIStreamMgr.hpp>
#include <aurora/j_audio_stream.hpp>
#include <dolphin/dvd.h>
#include <cstdio>
#include <limits>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <aurora/j_audio_sound_archive.hpp>
#include <new>
#include <stdexcept>

namespace {
    std::vector< u8 > readAudioFile(const std::string& path) {
        const aurora::allocation::HostAllocationScope host;
        DVDFileInfo file{};
        if (!DVDOpen(path.c_str(), &file))
            aurora::throw_host_exception< std::runtime_error >("Cannot open audio resource: " + path);
        struct CloseFile { DVDFileInfo* file; ~CloseFile() { DVDClose(file); } } close{&file};
        if (file.length > std::numeric_limits< s32 >::max())
            aurora::throw_host_exception< std::runtime_error >("Audio resource exceeds the DVD read range: " + path);
        std::vector< u8 > bytes(file.length);
        if (DVDReadPrio(&file, bytes.data(), static_cast< s32 >(bytes.size()), 0, 1) != bytes.size())
            aurora::throw_host_exception< std::runtime_error >("Cannot read complete audio resource: " + path);
        return bytes;
    }

    void retireWrapper(void* object) noexcept {
        static_cast< AudSystemWrapper* >(object)->~AudSystemWrapper();
    }
}

AudSystemWrapper::AudSystemWrapper(JKRSolidHeap* audioHeap, JKRHeap* resourceHeap)
    : mAudSystem(nullptr), _4(audioHeap), _8(resourceHeap), mSmrRes(nullptr), mJaiSeqRes(nullptr), mJaiCordRes(nullptr),
      mJaiMeRes(nullptr), mJaiRemixSeqRes(nullptr), mSpkHeap(nullptr), mSpkRes(nullptr), _28(false), _29(false), _2A(false) {
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

AudSystemWrapper* AudSystemWrapper::getCurrent() noexcept {
    auto* system = SingletonHolder< GameSystem >::get();
    return system && system->mObjHolder ? system->mObjHolder->mAudioSystem : nullptr;
}

bool AudSystemWrapper::isOutputDisabled() {
    auto* wrapper = getCurrent();
    return !wrapper || !wrapper->mAudSystem;
}

AudSceneMgr* AudSystemWrapper::getSceneMgr() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mAudSystem->mSceneMgr : nullptr;
}

AudBgmMgr* AudSystemWrapper::getBgmMgr() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? &mAudSystem->mBgmMgr : nullptr;
}

AudSoundObject* AudSystemWrapper::getSystemSeObject() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mAudSystem->mSystemSeObject : nullptr;
}

AudSoundObjHolder* AudSystemWrapper::getSoundObjHolder() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mAudSystem->mSoundObjHolder : nullptr;
}

AudRemixMgr* AudSystemWrapper::getRemixMgr() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mAudSystem->mRemixMgr : nullptr;
}

void AudSystemWrapper::setTriggerSePermitted(bool permitted) noexcept {
    mTriggerSePermitted = permitted;
    if (mAudSystem) mAudSystem->_82B = !permitted;
}

void AudSystemWrapper::setLevelSePermitted(bool permitted) noexcept {
    mLevelSePermitted = permitted;
    if (mAudSystem) mAudSystem->_82C = !permitted;
}

bool AudSystemWrapper::isSePermitted() const noexcept {
    return mTriggerSePermitted && mLevelSePermitted;
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
        const auto size = JKRHeap::getSize(mSmrRes, JKRHeap::findFromRoot(mSmrRes));
        if (size <= 0) {
            aurora::throw_host_exception< std::logic_error >("Audio name resource requires a bounded original heap allocation");
        }
        const aurora::allocation::HostAllocationScope host;
        mStreamArchive = std::make_shared< aurora::audio::JAudioSoundArchive >(
            std::span< const u8 >(static_cast< const u8* >(mSmrRes), static_cast< std::size_t >(size)),
            [](std::string_view name) -> std::vector< u8 > {
                return readAudioFile("/AudioRes/Waves/" + std::string(name));
            });
        mSoundNameBytes = mStreamArchive->native_sound_name_table();
        if (mSoundNameBytes.size() < 16) {
            aurora::throw_host_exception< std::runtime_error >("Audio initialization received no sound-name table");
        }
        mSoundNameTable.init(mSoundNameBytes.data());
        AudSoundNameConverter::validateTable(&mSoundNameTable);
        createSoundNameConverter();

        mNativeAudioArchive = mStreamArchive->native_runtime_archive();
        mInfoResources = std::make_unique<smgpc::resource::AudioInfoResources>(mJaiCordRes, mJaiMeRes, mJaiRemixSeqRes);
        {
            const MR::CurrentHeapRestorer current(_4);
            const aurora::allocation::ClientAllocationScope game({true, true});
            mAudSystem = AudNewAudSystem(_4, mNativeAudioArchive.data(), mJaiSeqRes, mJaiCordRes, mJaiMeRes, mJaiRemixSeqRes);
        }
        mStreamMixer = std::make_shared< aurora::audio::PcmAudioMixer >();
        mStreamMixer->open_default_playback();
        mAudSystem->getStreamMgr().bindNativeOutput(mStreamMixer, [archive = mStreamArchive](JAISoundID id) {
            const auto metadata = archive->resolve_sound(static_cast< u32 >(id));
            if (metadata.kind != aurora::audio::JAudioSoundKind::Stream)
                aurora::throw_host_exception< std::invalid_argument >("JAI stream request refers to a non-stream resource");
            auto recipe = aurora::audio::decode_jaudio_stream(readAudioFile(metadata.stream_path), metadata.channel_control);
            recipe.voice.gain_multiplier = metadata.volume / 127.0f;
            std::fprintf(stderr, "[original-audio] Prepared stream id=%08x rate=%u channels=%u samples=%u loop=%d path=%s\n",
                static_cast< u32 >(id), recipe.sample_rate, recipe.channel_count, recipe.sample_count, recipe.looping, metadata.stream_path.c_str());
            return recipe;
        });

        mInitializePhase = InitializePhase::Initialized;
        AudMicWrap::setMicEnv();
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
    if (mInitializePhase != InitializePhase::Received || mSoundNameBytes.empty()) {
        aurora::throw_host_exception< std::logic_error >("Audio name publication requires its received sound-name table");
    }
    AudSoundNameConverter::validateTable(&mSoundNameTable);
    const MR::CurrentHeapRestorer current(JKRHeap::findFromRoot(this));
    const aurora::allocation::ClientAllocationScope game({true, true});
    mPreviousNameTable = JAUSoundNameTable::sInstance;
    JAUSoundNameTable::sInstance = &mSoundNameTable;
    try {
        mSoundNameConverter = std::make_unique< AudSoundNameConverter >();
    } catch (...) {
        JAUSoundNameTable::sInstance = mPreviousNameTable;
        throw;
    }
    mPreviousNameConverter = AudSingletonHolder< AudSoundNameConverter >::exchange(mSoundNameConverter.get());
}

void AudSystemWrapper::releaseResources() noexcept {
    const aurora::allocation::HostAllocationScope host;
    if (mAudSystem) {
        mAudSystem->stopSync();
        delete mAudSystem;
        mAudSystem = nullptr;
        AudSystem::msBasic = nullptr;
    }
    smgpc::audio::shutdown_dsp();
    mInfoResources.reset();
    mNativeAudioArchive.clear();
    if (mStreamMixer) {
        mStreamMixer->close_default_playback();
        const auto stats = mStreamMixer->stats();
        std::fprintf(stderr, "[original-audio] Playback frames=%llu nonzero_samples=%llu device_callbacks=%llu\n",
            static_cast< unsigned long long >(stats.mixed_frames), static_cast< unsigned long long >(stats.nonzero_samples),
            static_cast< unsigned long long >(stats.device_callbacks));
    }
    mStreamMixer.reset();
    mStreamArchive.reset();
    if (mSoundNameConverter && AudSingletonHolder< AudSoundNameConverter >::get() == mSoundNameConverter.get()) {
        AudSingletonHolder< AudSoundNameConverter >::exchange(mPreviousNameConverter);
    }
    mSoundNameConverter.reset();
    if (JAUSoundNameTable::sInstance == &mSoundNameTable) {
        JAUSoundNameTable::sInstance = mPreviousNameTable;
    }
    mPreviousNameConverter = nullptr;
    mPreviousNameTable = nullptr;
    mSoundNameTable.init(nullptr);
    mSoundNameBytes.clear();
}

void AudSystemWrapper::updateRhythm() {
    if (mAudSystem) mAudSystem->mRhythmMeSystem->rhythmProc();
}

void AudSystemWrapper::movement() {
    if (mInitializePhase != InitializePhase::Initialized) {
        return;
    }
    mAudSystem->frameWork();
    smgpc::audio::advance_dsp(1.0 / 60.0);
}

void AudSystemWrapper::stopAllSound(u32 frames) {
    if (mAudSystem) mAudSystem->stop(frames);
}

bool AudSystemWrapper::isLoadDoneWaveDataAtSystemInit() const {
    return mInitializePhase == InitializePhase::Initialized && mAudSystem->mSceneMgr->isLoadDoneSystemInit();
}

void AudSystemWrapper::loadStaticWaveData() {
    if (mInitializePhase == InitializePhase::Initialized) {
        mAudSystem->mSceneMgr->loadStaticResource();
        mStaticWaveRequested = true;
    }
}

bool AudSystemWrapper::isLoadDoneStaticWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mStaticWaveRequested && mAudSystem->mSceneMgr->isLoadDoneStaticResource();
}

void AudSystemWrapper::loadStageWaveData(const char* sceneName, const char* stageName, bool isPlayerLuigi) {
    if (mInitializePhase != InitializePhase::Initialized) {
        return;
    }
    if (isPlayerLuigi) {
        mAudSystem->mSceneMgr->setPlayerModeLuigi();
    } else {
        mAudSystem->mSceneMgr->setPlayerModeMario();
    }
    mScenarioWaveRequested = false;
    mAudSystem->mSceneMgr->loadStageResource(sceneName, stageName);
    mStageWaveRequested = true;
}

bool AudSystemWrapper::isLoadDoneStageWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mStageWaveRequested && mAudSystem->mSceneMgr->isLoadDoneStageResource();
}

void AudSystemWrapper::loadScenarioWaveData(const char* sceneName, const char* stageName, s32 scenarioNo) {
    if (mInitializePhase == InitializePhase::Initialized) {
        mAudSystem->mSceneMgr->loadScenarioResource(sceneName, stageName, scenarioNo);
        mScenarioWaveRequested = true;
    }
}

bool AudSystemWrapper::isLoadDoneScenarioWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mScenarioWaveRequested && mAudSystem->mSceneMgr->isLoadDoneScenarioResource();
}

bool AudSystemWrapper::isPermitToReset() const {
    return !_2A;
}

void AudSystemWrapper::prepareReset() {
    if (mInitializePhase != InitializePhase::Initialized) {
        _29 = true;
    } else {
        mResetRequested = true;
    }
}

void AudSystemWrapper::requestReset(bool) {
    prepareReset();
    stopAllSound(10);
}

bool AudSystemWrapper::isResetDone() {
    return _29 || mResetRequested || mInitializePhase != InitializePhase::Initialized;
}

void AudSystemWrapper::resumeReset() {
    _29 = false;
    mResetRequested = false;
}
