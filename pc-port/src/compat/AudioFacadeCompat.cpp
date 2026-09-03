#include "compat/AudioFacadeCompat.hpp"

#include "Game/AudioLib/AudBgm.hpp"
#include "Game/AudioLib/AudBgmKeeper.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudBgmRhythmStrategy.hpp"
#include "Game/AudioLib/AudBgmVolumeController.hpp"
#include "Game/AudioLib/AudFader.hpp"
#include "Game/AudioLib/AudTrackController.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"

#include <JSystem/JAudio2/JAISound.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
    thread_local smgpc::runtime::AudioEventService *s_audio_override = nullptr;

    AudBgmMgr s_bgm_manager;
    JAISoundHandle s_stage_handle;

    [[noreturn]] void unavailable(const char *operation) {
        throw std::logic_error(std::string("The concrete JAudio backend does not provide ") + operation + ".");
    }

    [[nodiscard]] AudBgm *allocate_bgm(u32 sound_id) {
        const auto type = (sound_id & 0x10000U) == 0U ? AudBgmKeeper::BgmType_Single : AudBgmKeeper::BgmType_Multi;
        auto *bgm = s_bgm_manager.mKeeper.get(type);
        if (bgm == nullptr) {
            throw std::logic_error("The retail-shaped BGM keeper has no free stage BGM object.");
        }
        return bgm;
    }

    [[nodiscard]] AudBgm *allocate_bound_stage_bgm(u32 sound_id) {
        auto *bgm = allocate_bgm(sound_id);
        if ((sound_id & 0x10000U) != 0U) {
            s_bgm_manager.mKeeper.release(bgm);
            unavailable("multi-BGM facade binding");
        }
        bgm->_18 = static_cast<s32>(sound_id);
        bgm->setVolumeController(
            &s_bgm_manager.mVolumeController[AudBgmMgr::BgmType_Stage]);
        static_cast<AudSingleBgm *>(bgm)->startTrackControl();
        return bgm;
    }

    void release_stage_bgm_object() {
        if (s_bgm_manager.mBgm[AudBgmMgr::BgmType_Stage] != nullptr) {
            s_bgm_manager.mKeeper.release(s_bgm_manager.mBgm[AudBgmMgr::BgmType_Stage]);
            s_bgm_manager.mBgm[AudBgmMgr::BgmType_Stage] = nullptr;
        }
    }

    [[nodiscard]] smgpc::runtime::RuntimeContext *try_concrete_audio_runtime() {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            return nullptr;
        }
        auto *active_audio = s_audio_override != nullptr ? s_audio_override : &runtime->audio();
        return active_audio == &runtime->audio() ? runtime : nullptr;
    }

    [[nodiscard]] smgpc::runtime::RuntimeContext &
    require_concrete_audio_runtime(const char *operation) {
        auto *runtime = try_concrete_audio_runtime();
        if (runtime == nullptr) {
            throw std::logic_error(
                std::string(operation) +
                " requires an active RuntimeContext audio backend; an event-only binding cannot play sound");
        }
        return *runtime;
    }

    void synchronize_stage_handle(
        smgpc::runtime::JAudioPlaybackService &playback) {
        const auto token = playback.stage_bgm_backend_token();
        if (token == 0U) {
            s_stage_handle.releaseSound();
            return;
        }
        s_stage_handle.attachBackend(&playback, token);
    }

    void require_detached_track_controller(
        const AudTrackController &controller, const char *operation) {
        if (controller.mHandle != nullptr &&
            controller.mHandle->isSoundAttached()) {
            unavailable(operation);
        }
    }
}  // namespace

namespace smgpc::compat {

    smgpc::runtime::AudioEventService *try_active_audio_event_service() {
        if (s_audio_override != nullptr) {
            return s_audio_override;
        }
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr ? &runtime->audio() : nullptr;
    }

    smgpc::runtime::AudioEventService &require_active_audio_event_service() {
        auto *audio = try_active_audio_event_service();
        if (audio == nullptr) {
            throw std::logic_error("Audio state is unavailable without an active runtime or explicit service binding.");
        }
        return *audio;
    }

    ScopedAudioEventServiceOverride::ScopedAudioEventServiceOverride(smgpc::runtime::AudioEventService &service)
        : _previous(std::exchange(s_audio_override, &service)) {
        try {
            synchronize_audio_facade_state();
        } catch (...) {
            release_stage_bgm_object();
            s_stage_handle.releaseSound();
            s_audio_override = _previous;
            try {
                auto *restored = try_active_audio_event_service();
                if (restored != nullptr && restored->is_stage_bgm_identity_resolved()) {
                    synchronize_audio_facade_state();
                }
            } catch (...) {
                release_stage_bgm_object();
                s_stage_handle.releaseSound();
            }
            throw;
        }
    }

    ScopedAudioEventServiceOverride::~ScopedAudioEventServiceOverride() {
        release_stage_bgm_object();
        s_stage_handle.releaseSound();
        s_audio_override = _previous;
        try {
            auto *restored = try_active_audio_event_service();
            if (restored != nullptr && restored->is_stage_bgm_identity_resolved()) {
                synchronize_audio_facade_state();
            }
        } catch (...) {
            release_stage_bgm_object();
            s_stage_handle.releaseSound();
        }
    }

    void synchronize_audio_facade_state() {
        auto &audio = require_active_audio_event_service();
        release_stage_bgm_object();
        s_stage_handle.releaseSound();
        for (auto index = 0; index < 2; ++index) {
            s_bgm_manager.mBgm[index] = nullptr;
            s_bgm_manager.mNextBGM[index] = static_cast<u32>(-1);
            s_bgm_manager.mCurrentBGM[index] = static_cast<u32>(-1);
            s_bgm_manager.mLastBGM[index] = static_cast<u32>(-1);
        }
        if (audio.last_stage_bgm_id().has_value()) {
            s_bgm_manager.mLastBGM[AudBgmMgr::BgmType_Stage] = *audio.last_stage_bgm_id();
        }
        if (audio.current_stage_bgm_id().has_value()) {
            s_bgm_manager.mCurrentBGM[AudBgmMgr::BgmType_Stage] = *audio.current_stage_bgm_id();
        }
        auto *runtime = try_concrete_audio_runtime();
        const auto backend_active =
            runtime != nullptr &&
            runtime->j_audio_playback().has_active_stage_bgm();
        if (audio.has_active_stage_bgm() && !backend_active) {
            if (runtime == nullptr) {
                throw std::logic_error(
                    "Active stage-BGM state has no concrete RuntimeContext backend");
            }
            throw std::logic_error(
                "Logical stage-BGM state has no matching concrete backend voice");
        }
        if (backend_active) {
            auto &playback = runtime->j_audio_playback();
            const auto id = playback.stage_bgm_id();
            if (!id.has_value()) {
                throw std::logic_error(
                    "Concrete stage-BGM backend has no retail sound ID");
            }
            if (audio.has_active_stage_bgm()) {
                if (audio.current_stage_bgm_id() != id) {
                    throw std::logic_error(
                        "Logical stage-BGM identity disagrees with its concrete backend voice");
                }
            } else if (!playback.is_stage_bgm_stopping()) {
                throw std::logic_error(
                    "Concrete stage-BGM voice is active without logical start/stop state");
            }
            s_bgm_manager.mCurrentBGM[AudBgmMgr::BgmType_Stage] = *id;
            s_bgm_manager.mBgm[AudBgmMgr::BgmType_Stage] =
                allocate_bound_stage_bgm(*id);
            synchronize_stage_handle(playback);
        }
    }

    void advance_audio_facade_state() {
        if (try_concrete_audio_runtime() == nullptr) {
            return;
        }
        s_bgm_manager.movement();
    }

}  // namespace smgpc::compat

AudFader::AudFader() : mCurrentVolume(1.0F), mFinalVolume(1.0F), mStepVolume(0.0F) {
}

void AudFader::set(f32 desired_volume, s32 fade_time) {
    if (fade_time < 0) {
        throw std::invalid_argument("An audio fade cannot use negative frames.");
    }
    mFinalVolume = desired_volume;
    if (fade_time == 0) {
        mCurrentVolume = desired_volume;
        mStepVolume = 0.0F;
        return;
    }
    mStepVolume = (desired_volume - mCurrentVolume) / static_cast<f32>(fade_time);
}

void AudFader::update() {
    if (mStepVolume == 0.0F) {
        return;
    }
    mCurrentVolume += mStepVolume;
    if ((mStepVolume > 0.0F && mCurrentVolume >= mFinalVolume) ||
        (mStepVolume < 0.0F && mCurrentVolume <= mFinalVolume)) {
        mCurrentVolume = mFinalVolume;
        mStepVolume = 0.0F;
    }
}

AudTrackController::AudTrackController() : mTrackNo(-1), mHandle(nullptr), mFader(), mVolume(1.0F), mAutoMute(false) {
}

void AudTrackController::start(JAISoundHandle *handle) {
    mHandle = handle;
    mAutoMute = false;
}

void AudTrackController::stop() {
    mHandle = nullptr;
    mAutoMute = false;
}

void AudTrackController::mute() {
    require_detached_track_controller(*this, "active JAudio track muting");
    mFader.set(0.0F, 0);
}

void AudTrackController::unmute() {
    require_detached_track_controller(*this, "active JAudio track unmuting");
    mFader.set(1.0F, 0);
}

void AudTrackController::setMuteState(u8 state, s32 fade_time, bool auto_mute) {
    require_detached_track_controller(
        *this, "active JAudio track-mute transitions");
    if (state == 0U) {
        mFader.set(0.0F, fade_time);
    } else if (state == 1U) {
        mFader.set(1.0F, fade_time);
    } else if (state == 2U) {
        mute();
    } else if (state == 3U) {
        unmute();
    } else {
        throw std::invalid_argument("Unknown retail track-mute state.");
    }
    mAutoMute = auto_mute;
}

void AudTrackController::update() {
    require_detached_track_controller(
        *this, "active JAudio track-fader updates");
    mFader.update();
}

void AudTrackController::muteIfVolumeZero() {
    if (mAutoMute && mFader.getVolume() == 0.0F) {
        mute();
    }
}

AudBgm::AudBgm() : _4(nullptr), mRhythmStrategy(), mRhythmHandle(nullptr), _18(0), mTrackController() {
    mRhythmStrategy.mBgmIdx = -1;
    mRhythmStrategy.mBgm = nullptr;
    for (auto index = 0; index < 16; ++index) {
        mTrackController[index].mTrackNo = index;
    }
}

void AudBgm::setVolumeController(AudBgmVolumeController *controller) {
    _4 = controller;
}

AudBgmRhythmStrategy *AudBgm::getRhythmStrategy() {
    return &mRhythmStrategy;
}

void AudBgm::resetAuxVolume() {
    unavailable("BGM auxiliary-volume reset");
}

AudSingleBgm::AudSingleBgm() : AudBgm() {
    init();
}

void AudSingleBgm::init() {
    mRhythmHandle = nullptr;
    _18 = -1;
    initTrackController();
}

JAISoundHandle *AudSingleBgm::start(u32 sound_id, bool prepared) {
    _18 = static_cast<s32>(sound_id);
    auto &runtime = require_concrete_audio_runtime("Stage-BGM start");
    auto *backend_handle = runtime.start_stage_bgm(sound_id, prepared);
    if (backend_handle == nullptr || !backend_handle->isSoundAttached()) {
        throw std::logic_error(
            "Stage-BGM runtime returned no concrete backend handle");
    }
    synchronize_stage_handle(runtime.j_audio_playback());
    startTrackControl();
    return &s_stage_handle;
}

void AudSingleBgm::stop(u32 fade_frames) {
    auto &runtime = require_concrete_audio_runtime("Stage-BGM stop");
    runtime.stop_stage_bgm(static_cast<s32>(fade_frames));
    synchronize_stage_handle(runtime.j_audio_playback());
    stopTrackControl();
}

bool AudSingleBgm::isPreparedPlay() {
    return require_concrete_audio_runtime("Stage-BGM prepared query")
        .j_audio_playback()
        .is_stage_bgm_prepared();
}

void AudSingleBgm::playAfterPrepared() {
    require_concrete_audio_runtime("Stage-BGM unlock").unlock_stage_bgm();
}

void AudSingleBgm::movement() {
    updateTrackControl();
}

void AudSingleBgm::moveVolume(f32, u32) {
    unavailable("stage-BGM volume movement");
}

void AudSingleBgm::moveVolumeForNoteFairy(f32, u32) {
    unavailable("Note Fairy stage-BGM volume movement");
}

void AudSingleBgm::changeTrackMuteState(s32 state, s32 frames) {
    if (frames < 0) {
        throw std::invalid_argument(
            "A stage-BGM track-state transition cannot use negative frames");
    }
    require_concrete_audio_runtime("Stage-BGM track-state transition")
        .set_stage_bgm_state(state, static_cast<u32>(frames));
}

JAISoundHandle *AudSingleBgm::getHandle() {
    auto &runtime = require_concrete_audio_runtime("Stage-BGM handle query");
    synchronize_stage_handle(runtime.j_audio_playback());
    // Retail keeps the handle object's address through detach so the BGM
    // keeper can observe completion and release the owning AudBgm object.
    return &s_stage_handle;
}

JAISoundHandle *AudSingleBgm::getRhythmHandle() {
    return mRhythmHandle;
}

bool AudSingleBgm::isSoundAttached() const {
    auto &runtime = require_concrete_audio_runtime("Stage-BGM attachment query");
    synchronize_stage_handle(runtime.j_audio_playback());
    return s_stage_handle.isSoundAttached();
}

void AudSingleBgm::pause(bool paused) {
    require_concrete_audio_runtime("Stage-BGM pause")
        .j_audio_playback()
        .pause_stage_bgm(paused);
}

bool AudSingleBgm::isStopping() const {
    const auto &playback =
        require_concrete_audio_runtime("Stage-BGM stopping query")
            .j_audio_playback();
    return !playback.has_active_stage_bgm() ||
           playback.is_stage_bgm_stopping();
}

bool AudSingleBgm::isPaused() const {
    return require_concrete_audio_runtime("Stage-BGM pause query")
        .j_audio_playback()
        .is_stage_bgm_paused();
}

JAISoundID AudSingleBgm::getSoundID() const {
    const auto id = require_concrete_audio_runtime("Stage-BGM ID query")
                        .j_audio_playback()
                        .stage_bgm_id();
    if (!id.has_value()) {
        throw std::logic_error("The active stage BGM has no resolved raw sound ID.");
    }
    return JAISoundID(*id);
}

void AudSingleBgm::sendToSyncStream() {
}

void AudSingleBgm::rejectFromSyncStream() {
}

void AudSingleBgm::initTrackController() {
    for (auto &controller : mTrackController) {
        controller.stop();
    }
}

void AudSingleBgm::startTrackControl() {
    for (auto &controller : mTrackController) {
        controller.start(&s_stage_handle);
    }
}

void AudSingleBgm::stopTrackControl() {
    for (auto &controller : mTrackController) {
        controller.stop();
    }
}

void AudSingleBgm::updateTrackControl() {
    for (auto &controller : mTrackController) {
        if (controller.mHandle != nullptr &&
            controller.mHandle->isSoundAttached() &&
            controller.mFader.mStepVolume == 0.0F) {
            continue;
        }
        controller.update();
    }
}

AudMultiBgm::AudMultiBgm() : AudBgm(), mFader(), _1F4(0U), _1F8(-1), _1FC(0U) {
    init();
}

void AudMultiBgm::init() {
    mRhythmHandle = nullptr;
    initTrackController();
}

JAISoundHandle *AudMultiBgm::start(u32 sound_id, bool prepared) {
    (void)sound_id;
    (void)prepared;
    unavailable("multi-BGM sequence/stream synchronization");
}

void AudMultiBgm::stop(u32 fade_frames) {
    (void)fade_frames;
    unavailable("multi-BGM stop without sequence/stream synchronization");
}

bool AudMultiBgm::isPreparedPlay() {
    unavailable("multi-BGM prepared state");
}

void AudMultiBgm::playAfterPrepared() {
    unavailable("multi-BGM prepared unlock");
}

void AudMultiBgm::movement() {
    updateTrackControl();
}

void AudMultiBgm::moveVolume(f32, u32) {
    unavailable("multi-BGM volume movement");
}

void AudMultiBgm::moveVolumeForNoteFairy(f32, u32) {
    unavailable("multi-BGM Note Fairy volume movement");
}

void AudMultiBgm::changeTrackMuteState(s32 state, s32 frames) {
    (void)state;
    (void)frames;
    unavailable("multi-BGM track-state transition");
}

JAISoundHandle *AudMultiBgm::getHandle() {
    unavailable("multi-BGM handle access");
}

JAISoundHandle *AudMultiBgm::getRhythmHandle() {
    return mRhythmHandle;
}

bool AudMultiBgm::isSoundAttached() const {
    unavailable("multi-BGM attachment query");
}

void AudMultiBgm::pause(bool) {
    unavailable("multi-BGM pause state");
}

bool AudMultiBgm::isStopping() const {
    unavailable("multi-BGM stopping query");
}

bool AudMultiBgm::isPaused() const {
    unavailable("multi-BGM pause query");
}

JAISoundID AudMultiBgm::getSoundID() const {
    unavailable("multi-BGM sound-ID query");
}

void AudMultiBgm::sendToSyncStream() {
    unavailable("multi-BGM sync-stream submission");
}

void AudMultiBgm::rejectFromSyncStream() {
    unavailable("multi-BGM sync-stream rejection");
}

void AudMultiBgm::initTrackController() {
    for (auto &controller : mTrackController) {
        controller.stop();
    }
}

void AudMultiBgm::startTrackControl() {
    for (auto &controller : mTrackController) {
        controller.start(&s_stage_handle);
    }
}

void AudMultiBgm::updateTrackControl() {
    for (auto &controller : mTrackController) {
        controller.update();
    }
}

void *AudMultiBgm::prepare(u32) {
    unavailable("multi-BGM prepare handle");
}

bool AudMultiBgm::isPrepared() {
    return isPreparedPlay();
}

void AudMultiBgm::unlock() {
    playAfterPrepared();
}

void AudMultiBgm::updateSyncProcess() {
    unavailable("multi-BGM sync-process update");
}

void AudMultiBgm::pauseSyncProcess() {
    unavailable("multi-BGM sync pause");
}

void AudMultiBgm::setStreamVolume(f32, f32) {
    unavailable("multi-BGM stream volume");
}

AudBgmKeeper::AudBgmKeeper() : mSingleBgm(), mMultiBgm(), mSingleBgmActiveFlags(0U), mMultiBgmActiveFlags(0U) {
}

AudBgm *AudBgmKeeper::get(BgmType type) {
    return type == BgmType_Single ? static_cast<AudBgm *>(getValidSingleBgm()) : static_cast<AudBgm *>(getValidMultiBgm());
}

void AudBgmKeeper::release(AudBgm *bgm) {
    for (auto index = 0; index < 2; ++index) {
        const auto mask = static_cast<u8>(1U << index);
        if (&mSingleBgm[index] == bgm) {
            mSingleBgm[index].init();
            mSingleBgmActiveFlags &= static_cast<u8>(~mask);
            return;
        }
        if (&mMultiBgm[index] == bgm) {
            mMultiBgm[index].init();
            mMultiBgmActiveFlags &= static_cast<u8>(~mask);
            return;
        }
    }
    throw std::invalid_argument("The BGM object does not belong to this retail-shaped keeper.");
}

AudSingleBgm *AudBgmKeeper::getValidSingleBgm() {
    for (auto index = 0; index < 2; ++index) {
        const auto mask = static_cast<u8>(1U << index);
        if ((mSingleBgmActiveFlags & mask) == 0U) {
            mSingleBgmActiveFlags |= mask;
            return &mSingleBgm[index];
        }
    }
    return nullptr;
}

AudMultiBgm *AudBgmKeeper::getValidMultiBgm() {
    for (auto index = 0; index < 2; ++index) {
        const auto mask = static_cast<u8>(1U << index);
        if ((mMultiBgmActiveFlags & mask) == 0U) {
            mMultiBgmActiveFlags |= mask;
            return &mMultiBgm[index];
        }
    }
    return nullptr;
}

bool AudBgmRhythmStrategy::set(AudBgm *, s32 bgm_index) {
    static_cast<void>(bgm_index);
    unavailable("BGM rhythm-strategy binding");
}

void AudBgmRhythmStrategy::reject() {
    mBgmIdx = -1;
    mBgm = nullptr;
}

bool AudBgmRhythmStrategy::setDominant() {
    unavailable("dominant BGM rhythm selection");
}

bool AudBgmRhythmStrategy::isDominant() const {
    unavailable("dominant BGM rhythm query");
}

AudBgmMgr::AudBgmMgr() : mBgm{}, mNextBGM{}, mCurrentBGM{}, mLastBGM{}, mKeeper(), mVolumeController(), _8FC(false) {
    for (auto index = 0; index < 2; ++index) {
        mBgm[index] = nullptr;
        mNextBGM[index] = static_cast<u32>(-1);
        mCurrentBGM[index] = static_cast<u32>(-1);
        mLastBGM[index] = static_cast<u32>(-1);
    }
}

void AudBgmMgr::movement() {
    for (auto index = 0; index < 2; ++index) {
        if (mBgm[index] != nullptr) {
            mBgm[index]->movement();
        }
        releaseStoppingBgm(index);
    }
}

JAISoundHandle *AudBgmMgr::start(s32 bgm_index, u32 sound_id, bool prepared) {
    if (bgm_index != BgmType_Stage) {
        unavailable("sub-BGM start");
    }
    release_stage_bgm_object();
    mBgm[bgm_index] = allocate_bgm(sound_id);
    mBgm[bgm_index]->setVolumeController(&mVolumeController[bgm_index]);
    JAISoundHandle *handle = nullptr;
    try {
        handle = mBgm[bgm_index]->start(sound_id, prepared);
    } catch (...) {
        release_stage_bgm_object();
        throw;
    }
    if (handle == nullptr) {
        release_stage_bgm_object();
        throw std::logic_error("The concrete stage-BGM backend did not attach a sound handle.");
    }
    mLastBGM[bgm_index] = mCurrentBGM[bgm_index];
    mCurrentBGM[bgm_index] = sound_id;
    return handle;
}

void AudBgmMgr::setNextBGM(s32 bgm_index, u32 sound_id) {
    (void)bgm_index;
    (void)sound_id;
    unavailable("the retail next-BGM scheduler");
}

void AudBgmMgr::clearNextBGM(s32 bgm_index) {
    mNextBGM[bgm_index] = static_cast<u32>(-1);
}

JAISoundHandle *AudBgmMgr::startLastBGM(s32 bgm_index) {
    if (bgm_index != BgmType_Stage || mLastBGM[bgm_index] == static_cast<u32>(-1)) {
        throw std::logic_error("No resolved last stage BGM is available.");
    }
    return start(bgm_index, mLastBGM[bgm_index], false);
}

void AudBgmMgr::clearLastBGM(s32 bgm_index) {
    mLastBGM[bgm_index] = static_cast<u32>(-1);
    if (bgm_index == BgmType_Stage) {
        smgpc::compat::require_active_audio_event_service().clear_last_stage_bgm_id();
    }
}

void AudBgmMgr::pause() {
    require_concrete_audio_runtime("BGM manager pause")
        .j_audio_playback()
        .pause_stage_bgm(true);
}

void AudBgmMgr::unpause() {
    require_concrete_audio_runtime("BGM manager unpause")
        .j_audio_playback()
        .pause_stage_bgm(false);
}

void AudBgmMgr::volDownLevel(bool) {
    unavailable("BGM manager level-volume reduction");
}

void AudBgmMgr::volDownStageBgmWhenSubBgmPlaying() {
    unavailable("sub-BGM stage-volume interruption");
}

void AudBgmMgr::startNextBgmWhenStopping(s32 bgm_index) {
    if (mNextBGM[bgm_index] == static_cast<u32>(-1) || mBgm[bgm_index] == nullptr) {
        return;
    }
    unavailable("the retail next-BGM scheduler");
}

void AudBgmMgr::releaseStoppingBgm(s32 bgm_index) {
    if (mBgm[bgm_index] == nullptr) {
        return;
    }
    auto *handle = mBgm[bgm_index]->getHandle();
    if (handle != nullptr && !handle->isSoundAttached()) {
        mKeeper.release(mBgm[bgm_index]);
        mBgm[bgm_index] = nullptr;
    }
}

bool AudBgmMgr::sendToRhythmSystem(s32) {
    unavailable("BGM rhythm-system submission");
}

void AudBgmMgr::setBgmToRhythmDominant(s32) {
    unavailable("dominant BGM rhythm selection");
}

void AudBgmMgr::stopRhythmProcess(s32) {
    unavailable("BGM rhythm-process stop");
}

namespace AudWrap {

    AudSystem *getSystem() {
        unavailable("AudSystem access");
    }

    AudSoundInfo *getSoundInfo() {
        unavailable("AudSoundInfo access");
    }

    AudSceneMgr *getSceneMgr() {
        unavailable("AudSceneMgr access");
    }

    AudBgmMgr *getBgmMgr() {
        auto &audio = smgpc::compat::require_active_audio_event_service();
        if (!audio.is_stage_bgm_identity_resolved()) {
            throw std::logic_error("The current stage-BGM identity has not been resolved.");
        }
        auto *runtime = try_concrete_audio_runtime();
        if (runtime != nullptr &&
            runtime->j_audio_playback().has_active_stage_bgm()) {
            s_bgm_manager.mCurrentBGM[AudBgmMgr::BgmType_Stage] =
                runtime->j_audio_playback().stage_bgm_id().value_or(
                    static_cast<u32>(-1));
        } else {
            s_bgm_manager.mCurrentBGM[AudBgmMgr::BgmType_Stage] =
                audio.current_stage_bgm_id().value_or(static_cast<u32>(-1));
        }
        return &s_bgm_manager;
    }

    AudBgm *getStageBgm() {
        auto &audio = smgpc::compat::require_active_audio_event_service();
        if (!audio.is_stage_bgm_identity_resolved()) {
            throw std::logic_error("The current stage-BGM identity has not been resolved.");
        }
        auto *runtime = try_concrete_audio_runtime();
        if (runtime == nullptr) {
            if (!audio.has_active_stage_bgm()) {
                return nullptr;
            }
            throw std::logic_error(
                "Active stage-BGM state has no concrete RuntimeContext backend");
        }
        auto &playback = runtime->j_audio_playback();
        if (!playback.has_active_stage_bgm()) {
            if (audio.has_active_stage_bgm()) {
                throw std::logic_error(
                    "Logical stage-BGM state has no matching concrete backend voice");
            }
            return nullptr;
        }
        const auto backend_id = playback.stage_bgm_id();
        if (!backend_id.has_value()) {
            throw std::logic_error(
                "Concrete stage-BGM backend has no retail sound ID");
        }
        if (audio.has_active_stage_bgm() &&
            backend_id != audio.current_stage_bgm_id()) {
            throw std::logic_error(
                "Logical stage-BGM identity disagrees with its concrete backend voice");
        }
        if (!audio.has_active_stage_bgm() &&
            !playback.is_stage_bgm_stopping()) {
            throw std::logic_error(
                "Concrete stage-BGM voice is active without logical start/stop state");
        }
        if (s_bgm_manager.mBgm[AudBgmMgr::BgmType_Stage] == nullptr) {
            s_bgm_manager.mBgm[AudBgmMgr::BgmType_Stage] =
                allocate_bound_stage_bgm(*backend_id);
        }
        synchronize_stage_handle(playback);
        return s_bgm_manager.mBgm[AudBgmMgr::BgmType_Stage];
    }

    AudBgm *getSubBgm() {
        unavailable("sub-BGM access");
    }

    JAISoundHandle *startStageBgm(u32 sound_id, bool prepared) {
        return s_bgm_manager.start(AudBgmMgr::BgmType_Stage, sound_id, prepared);
    }

    JAISoundHandle *startSubBgm(u32, bool) {
        unavailable("sub-BGM start");
    }

    void setNextIdStageBgm(u32 sound_id) {
        s_bgm_manager.setNextBGM(AudBgmMgr::BgmType_Stage, sound_id);
    }

    JAISoundHandle *startLastStageBgm() {
        return s_bgm_manager.startLastBGM(AudBgmMgr::BgmType_Stage);
    }

    AudSoundObject *getSystemSeObject() {
        unavailable("system-SE object access");
    }

    AudSoundObject *getAtmosphereSeObject() {
        unavailable("atmosphere-SE object access");
    }

    AudSoundObjHolder *getSoundObjHolder() {
        unavailable("sound-object holder access");
    }

    AudRhythmMeSystem *getRhythmMeSystem() {
        unavailable("rhythm-ME system access");
    }

    AudMeObject *getSystemMeObject() {
        unavailable("system-ME object access");
    }

    AudRemixMgr *getRemixMgr() {
        unavailable("remix manager access");
    }

    AudRemixSequencer *getRemixSequencer() {
        unavailable("remix sequencer access");
    }

    AudSoundObject *getRemixSeqObject() {
        unavailable("remix sequence object access");
    }

}  // namespace AudWrap
