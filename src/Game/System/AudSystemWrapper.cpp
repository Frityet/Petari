#include "Game/System/AudSystemWrapper.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudParams.hpp"
#include "Game/AudioLib/AudSceneMgr.hpp"
#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include "Game/AudioLib/AudSoundObject.hpp"
#include "Game/AudioLib/AudSoundObjHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
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
    return mInitializePhase == InitializePhase::Initialized ? mSceneMgr.get() : nullptr;
}

AudBgmMgr* AudSystemWrapper::getBgmMgr() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mBgmMgr.get() : nullptr;
}

AudSoundObject* AudSystemWrapper::getSystemSeObject() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mSystemSeObject.get() : nullptr;
}

AudSoundObjHolder* AudSystemWrapper::getSoundObjHolder() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mSoundObjHolder.get() : nullptr;
}

void AudSystemWrapper::setTriggerSePermitted(bool permitted) noexcept {
    mTriggerSePermitted = permitted;
}

void AudSystemWrapper::setLevelSePermitted(bool permitted) noexcept {
    mLevelSePermitted = permitted;
}

bool AudSystemWrapper::isSePermitted() const noexcept {
    return mTriggerSePermitted && mLevelSePermitted;
}

void AudSystemWrapper::requestResourceForInitialize() {
    if (mInitializePhase != InitializePhase::Created) {
        aurora::throw_host_exception< std::logic_error >("Audio initialization was already requested");
    }
    // Stream metadata and name conversion share the original sound archive.
    // The absent DSP, rhythm and speaker owners have no bank requests to enqueue.
    MR::loadAsyncToMainRAM("/AudioRes/SMR.szs", nullptr, _8, JKRDvdRipper::ALLOC_DIRECTION_BACKWARD);
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
    if (!mSmrRes) {
        aurora::throw_host_exception< std::runtime_error >("Audio initialization received no name resource");
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

        mStreamMixer = std::make_shared< aurora::audio::PcmAudioMixer >();
        mStreamMixer->open_default_playback();
        mStreamMgr = std::make_unique< JAIStreamMgr >(true);
        mStreamMgr->bindNativeOutput(mStreamMixer, [archive = mStreamArchive](JAISoundID id) {
            const auto metadata = archive->resolve_sound(static_cast< u32 >(id));
            if (metadata.kind != aurora::audio::JAudioSoundKind::Stream)
                aurora::throw_host_exception< std::invalid_argument >("JAI stream request refers to a non-stream resource");
            auto recipe = aurora::audio::decode_jaudio_stream(readAudioFile(metadata.stream_path), metadata.channel_control);
            recipe.voice.gain_multiplier = metadata.volume / 127.0f;
            std::fprintf(stderr, "[original-audio] Prepared stream id=%08x rate=%u channels=%u samples=%u loop=%d path=%s\n",
                static_cast< u32 >(id), recipe.sample_rate, recipe.channel_count, recipe.sample_count, recipe.looping, metadata.stream_path.c_str());
            return recipe;
        });

        auto* heap = JKRHeap::findFromRoot(this);
        const MR::CurrentHeapRestorer current(heap);
        const aurora::allocation::ClientAllocationScope game({true, true});
        mSceneMgr = std::make_unique< AudSceneMgr >(nullptr);
        mBgmMgr = std::make_unique< AudBgmMgr >();
        mSoundObjHolder = std::make_unique< AudSoundObjHolder >(heap, AudParams::numInspectableSoundObj);
        // The wrapper's finalizer owns this object. Host object storage keeps
        // JKRDisposer from destroying it first during bulk heap retirement;
        // its original constructor still allocates arrays in the selected heap.
        void* storage;
        {
            const aurora::allocation::HostAllocationScope objectStorage;
            storage = ::operator new(sizeof(AudSoundObject));
        }
        try {
            mSystemSeObject.reset(new (storage) AudSoundObject(nullptr, 10, heap));
        } catch (...) {
            ::operator delete(storage);
            throw;
        }
        mInitializePhase = InitializePhase::Initialized;
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
    mSystemSeObject.reset();
    mSoundObjHolder.reset();
    mBgmMgr.reset();
    mStreamMgr.reset();
    if (mStreamMixer) {
        mStreamMixer->close_default_playback();
        const auto stats = mStreamMixer->stats();
        std::fprintf(stderr, "[original-audio] Playback frames=%llu nonzero_samples=%llu device_callbacks=%llu\n",
            static_cast< unsigned long long >(stats.mixed_frames), static_cast< unsigned long long >(stats.nonzero_samples),
            static_cast< unsigned long long >(stats.device_callbacks));
    }
    mStreamMixer.reset();
    mStreamArchive.reset();
    mSceneMgr.reset();
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
    // Stream playback does not provide the sequencer's rhythm owner.
}

void AudSystemWrapper::movement() {
    if (mInitializePhase != InitializePhase::Initialized) {
        return;
    }
    mBgmMgr->movement();
    mStreamMgr->calc();
    mStreamMgr->mixOut();
    mSoundObjHolder->update();
}

void AudSystemWrapper::stopAllSound(u32 frames) {
    if (mStreamMgr) mStreamMgr->stop(frames);
}

bool AudSystemWrapper::isLoadDoneWaveDataAtSystemInit() const {
    return mInitializePhase == InitializePhase::Initialized && mSceneMgr->isLoadDoneSystemInit();
}

void AudSystemWrapper::loadStaticWaveData() {
    if (mInitializePhase == InitializePhase::Initialized) {
        mSceneMgr->loadStaticResource();
        mStaticWaveRequested = true;
    }
}

bool AudSystemWrapper::isLoadDoneStaticWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mStaticWaveRequested && mSceneMgr->isLoadDoneStaticResource();
}

void AudSystemWrapper::loadStageWaveData(const char* sceneName, const char* stageName, bool isPlayerLuigi) {
    if (mInitializePhase != InitializePhase::Initialized) {
        return;
    }
    if (isPlayerLuigi) {
        mSceneMgr->setPlayerModeLuigi();
    } else {
        mSceneMgr->setPlayerModeMario();
    }
    mScenarioWaveRequested = false;
    mSceneMgr->loadStageResource(sceneName, stageName);
    mStageWaveRequested = true;
}

bool AudSystemWrapper::isLoadDoneStageWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mStageWaveRequested && mSceneMgr->isLoadDoneStageResource();
}

void AudSystemWrapper::loadScenarioWaveData(const char* sceneName, const char* stageName, s32 scenarioNo) {
    if (mInitializePhase == InitializePhase::Initialized) {
        mSceneMgr->loadScenarioResource(sceneName, stageName, scenarioNo);
        mScenarioWaveRequested = true;
    }
}

bool AudSystemWrapper::isLoadDoneScenarioWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mScenarioWaveRequested && mSceneMgr->isLoadDoneScenarioResource();
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
