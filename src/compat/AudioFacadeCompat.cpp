#include <aurora/exception.hpp>
#include "compat/DisabledObjectAudioService.hpp"
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
#include <array>
#include <stdexcept>
#include <utility>

namespace {
    thread_local smgpc::runtime::AudioEventService *s_audio_override = nullptr;

    AudBgmMgr s_bgm_manager;
    using BgmLane = smgpc::runtime::BgmLane;

    [[noreturn]] void unavailable(const char *operation) {
        aurora::throw_host_exception<std::logic_error>(std::string("The concrete JAudio backend does not provide ") + operation + ".");
    }

    [[nodiscard]] AudBgm *allocate_bgm(u32 sound_id) {
        const auto type = (sound_id & 0x10000U) == 0U ? AudBgmKeeper::BgmType_Single : AudBgmKeeper::BgmType_Multi;
        auto *bgm = s_bgm_manager.mKeeper.get(type);
        return bgm;
    }

    [[nodiscard]] std::optional<BgmLane> bound_lane(const AudBgm *bgm) {
        for (std::size_t index = 0; index < 2; ++index) {
            if (s_bgm_manager.mBgm[index] == bgm &&
                bgm->mVolumeController == &s_bgm_manager.mVolumeController[index]) {
                return static_cast<BgmLane>(index);
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] BgmLane require_lane(const AudBgm *bgm) {
        const auto lane = bound_lane(bgm);
        if (!lane.has_value()) {
            aurora::throw_host_exception<std::logic_error>("BGM object is not retained by its stage/sub owner");
        }
        return *lane;
    }

    void release_bgm_object(BgmLane lane) {
        auto &bgm = s_bgm_manager.mBgm[static_cast<std::size_t>(lane)];
        if (bgm != nullptr) {
            s_bgm_manager.mKeeper.release(bgm);
            bgm = nullptr;
        }
    }

    void release_bgm_objects() {
        release_bgm_object(BgmLane::Stage);
        release_bgm_object(BgmLane::Sub);
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
            aurora::throw_host_exception<std::logic_error>(
                std::string(operation) +
                " requires an active RuntimeContext audio backend; an event-only binding cannot play sound");
        }
        return *runtime;
    }

    void synchronize_bgm_handle(AudSingleBgm &bgm, smgpc::runtime::JAudioPlaybackService &playback) {
        const auto lane = bound_lane(&bgm);
        if (lane) playback.bind_bgm_handle(*lane, bgm.mHandle);
        else bgm.mHandle.releaseSound();
    }

    AudBgm *reconcile_bgm(BgmLane lane, smgpc::runtime::JAudioPlaybackService &playback) {
        const auto index = static_cast<std::size_t>(lane);
        auto &object = s_bgm_manager.mBgm[index];
        const auto id = playback.bgm_id(lane);
        if (!id.has_value()) {
            if (object != nullptr) {
                static_cast<AudSingleBgm *>(object)->mHandle.releaseSound();
            }
            return object;
        }
        if (object != nullptr && static_cast<u32>(static_cast<AudSingleBgm *>(object)->mSoundID) != *id) {
            release_bgm_object(lane);
        }
        if (object == nullptr) {
            object = allocate_bgm(*id);
            if (object == nullptr) {
                return nullptr;
            }
            object->setVolumeController(&s_bgm_manager.mVolumeController[index]);
            static_cast<AudSingleBgm *>(object)->mSoundID = *id;
        }
        auto* attached = playback.bgm_handle(lane);
        auto* sound = attached ? attached->mSound : nullptr;
        if (static_cast<AudSingleBgm*>(object)->mHandle.mSound != sound) {
            object->resetAuxVolume();
        }
        s_bgm_manager.mCurrentBGM[index] = *id;
        synchronize_bgm_handle(*static_cast<AudSingleBgm *>(object), playback);
        return object;
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
            aurora::throw_host_exception<std::logic_error>("Audio state is unavailable without an active runtime or explicit service binding.");
        }
        return *audio;
    }

    ScopedAudioEventServiceOverride::ScopedAudioEventServiceOverride(smgpc::runtime::AudioEventService &service)
        : _previous(std::exchange(s_audio_override, &service)) {
        try {
            synchronize_audio_facade_state();
        } catch (...) {
            release_bgm_objects();
            s_audio_override = _previous;
            try {
                auto *restored = try_active_audio_event_service();
                if (restored != nullptr && restored->is_stage_bgm_identity_resolved()) {
                    synchronize_audio_facade_state();
                }
            } catch (...) {
                release_bgm_objects();
                }
            throw;
        }
    }

    ScopedAudioEventServiceOverride::~ScopedAudioEventServiceOverride() {
        release_bgm_objects();
        s_audio_override = _previous;
        try {
            auto *restored = try_active_audio_event_service();
            if (restored != nullptr && restored->is_stage_bgm_identity_resolved()) {
                synchronize_audio_facade_state();
            }
        } catch (...) {
            release_bgm_objects();
        }
    }

    void synchronize_audio_facade_state() {
        auto &audio = require_active_audio_event_service();
        auto *runtime = try_concrete_audio_runtime();
        if (runtime == nullptr) release_bgm_objects();
        for (std::size_t index = 0; index < 2; ++index) {
            s_bgm_manager.mNextBGM[index] = static_cast<u32>(-1);
            s_bgm_manager.mCurrentBGM[index] = static_cast<u32>(-1);
            s_bgm_manager.mLastBGM[index] = static_cast<u32>(-1);
        }
        if (audio.last_stage_bgm_id().has_value()) {
            s_bgm_manager.mLastBGM[0] = *audio.last_stage_bgm_id();
        }
        if (audio.current_stage_bgm_id().has_value()) {
            s_bgm_manager.mCurrentBGM[0] = *audio.current_stage_bgm_id();
        }
        if (runtime == nullptr) {
            if (audio.has_active_stage_bgm() || audio.has_active_sub_bgm()) {
                aurora::throw_host_exception<std::logic_error>("Active BGM state has no concrete RuntimeContext backend");
            }
            return;
        }
        auto &playback = runtime->j_audio_playback();
        if (audio.has_active_stage_bgm() && !playback.has_active_bgm(BgmLane::Stage)) {
            aurora::throw_host_exception<std::logic_error>("Logical stage-BGM state has no matching concrete backend voice");
        }
        for (auto lane : {BgmLane::Stage, BgmLane::Sub}) {
            (void)reconcile_bgm(lane, playback);
            s_bgm_manager.mCurrentBGM[static_cast<std::size_t>(lane)] = playback.bgm_id(lane).value_or(static_cast<u32>(-1));
        }
    }

    void retire_audio_facade_state() {
        release_bgm_objects();
    }

    void advance_audio_facade_state() {
        if (auto *runtime = try_concrete_audio_runtime()) {
            (void)reconcile_bgm(BgmLane::Stage, runtime->j_audio_playback());
            (void)reconcile_bgm(BgmLane::Sub, runtime->j_audio_playback());
            s_bgm_manager.movement();
        }
    }

}  // namespace smgpc::compat

AudFader::AudFader() : mCurrentVolume(1.0F), mFinalVolume(1.0F), mStepVolume(0.0F) {
}

void AudFader::set(f32 desired_volume, s32 fade_time) {
    if (fade_time < 0) {
        aurora::throw_host_exception<std::invalid_argument>("An audio fade cannot use negative frames.");
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
        aurora::throw_host_exception<std::invalid_argument>("Unknown retail track-mute state.");
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

AudBgm::AudBgm() : mVolumeController(nullptr) {
}

void AudBgm::resetAuxVolume() {
    if (mVolumeController == nullptr) {
        return;
    }
    mVolumeController->moveAuxVolume(1.0f, 0);
    mVolumeController->moveNoteFairyVolume(1.0f, 0);
}

AudSingleBgm::AudSingleBgm() : AudBgm(), mHandle(), mSoundID(-1) {
    init();
}

void AudSingleBgm::init() {
    mHandle.releaseSound();
    mSoundID.setAnonymous();
    initTrackController();
}

JAISoundHandle *AudSingleBgm::start(u32 sound_id, bool prepared) {
    auto &runtime = require_concrete_audio_runtime("BGM start");
    const auto lane = require_lane(this);
    resetAuxVolume();
    mSoundID = sound_id;
    auto *handle = lane == BgmLane::Stage ? runtime.start_stage_bgm(sound_id, prepared)
                                         : runtime.start_sub_bgm(sound_id, prepared);
    synchronize_bgm_handle(*this, runtime.j_audio_playback());
    if (handle != nullptr && mVolumeController != nullptr) {
        runtime.j_audio_playback().set_bgm_bus_gain(lane, mVolumeController->getVolume());
    }
    return handle != nullptr ? &mHandle : nullptr;
}

void AudSingleBgm::stop(u32 fade_frames) {
    if (!isSoundAttached()) return;
    auto &runtime = require_concrete_audio_runtime("BGM stop");
    const auto lane = require_lane(this);
    if (lane == BgmLane::Stage) runtime.stop_stage_bgm(static_cast<s32>(fade_frames));
    else runtime.stop_sub_bgm(fade_frames);
    synchronize_bgm_handle(*this, runtime.j_audio_playback());
    if (mSoundID.getSectionID() != 2) stopTrackControl();
}

bool AudSingleBgm::isPreparedPlay() {
    return isSoundAttached() && require_concrete_audio_runtime("BGM prepared query")
        .j_audio_playback().is_bgm_prepared(require_lane(this));
}

void AudSingleBgm::playAfterPrepared() {
    if (!isSoundAttached()) return;
    auto &runtime = require_concrete_audio_runtime("BGM unlock");
    if (require_lane(this) == BgmLane::Stage) runtime.unlock_stage_bgm();
    else runtime.unlock_sub_bgm();
}

void AudSingleBgm::movement() {
    updateTrackControl();
    if (!isSoundAttached()) {
        resetAuxVolume();
        return;
    }
    if (mVolumeController != nullptr) {
        require_concrete_audio_runtime("BGM volume update").j_audio_playback()
            .set_bgm_bus_gain(require_lane(this), mVolumeController->getVolume());
    }
}

bool AudSingleBgm::moveVolume(f32 volume, u32 time) {
    if (!isSoundAttached()) return false;
    if (mVolumeController != nullptr) {
        mVolumeController->moveAuxVolume(volume, time);
        return true;
    }
    return false;
}

bool AudSingleBgm::moveVolumeForNoteFairy(f32 volume, u32 time) {
    if (!isSoundAttached()) return false;
    if (mVolumeController != nullptr) {
        mVolumeController->moveNoteFairyVolume(volume, time);
        return true;
    }
    return false;
}

void AudSingleBgm::changeTrackMuteState(s32, s32) {
    if (mSoundID.getSectionID() == 2) return;
    if (isSoundAttached()) unavailable("section-one JAS track-mute transitions");
}

JAISoundHandle *AudSingleBgm::getRhythmHandle() {
    if (isSoundAttached() && mSoundID.getSectionID() == 1) return &mHandle;
    return nullptr;
}

void AudSingleBgm::initTrackController() {
    for (s32 index = 0; index < mNumTracks; ++index) {
        mTrackController[index].stop();
        mTrackController[index].mTrackNo = index;
    }
}

void AudSingleBgm::startTrackControl() {
    for (auto &controller : mTrackController) controller.start(&mHandle);
}

void AudSingleBgm::stopTrackControl() {
    for (auto &controller : mTrackController) controller.stop();
}

void AudSingleBgm::updateTrackControl() {
    if (mSoundID.getSectionID() == 2) return;
    for (auto &controller : mTrackController) controller.update();
}

AudMultiBgm::AudMultiBgm() : AudBgm(), mHandle(), mRhythmHandle(), mFader(), _1F4(0), mBgmId(-1), mIsLocked(false) {
    init();
}

void AudMultiBgm::init() {
    mHandle.releaseSound();
    mRhythmHandle.releaseSound();
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

bool AudMultiBgm::moveVolume(f32, u32) {
    unavailable("multi-BGM volume movement");
}

bool AudMultiBgm::moveVolumeForNoteFairy(f32, u32) {
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
    return &mRhythmHandle;
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
        controller.start(&mRhythmHandle);
    }
}

void AudMultiBgm::updateTrackControl() {
    for (auto &controller : mTrackController) {
        controller.update();
    }
}

JAISoundHandle *AudMultiBgm::prepare(u32) {
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
    aurora::throw_host_exception<std::invalid_argument>("The BGM object does not belong to this retail-shaped keeper.");
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
    volDownStageBgmWhenSubBgmPlaying();
    for (int index = 0; index < 2; ++index) {
        mVolumeController[index].mIsMuted = _8FC;
        mVolumeController[index].update();
        if (mBgm[index] != nullptr) mBgm[index]->movement();
        startNextBgmWhenStopping(index);
        releaseStoppingBgm(index);
    }
}

JAISoundHandle *AudBgmMgr::start(s32 bgm_index, u32 sound_id, bool prepared) {
    if (bgm_index < 0 || bgm_index >= 2) {
        aurora::throw_host_exception<std::out_of_range>("BGM index is outside the original stage/sub lanes");
    }
    const auto lane = static_cast<BgmLane>(bgm_index);
    if (mBgm[bgm_index] != nullptr) mBgm[bgm_index]->stop(0);
    release_bgm_object(lane);
    if ((sound_id & 0x10000U) != 0) {
        // Multi-BGM needs a synchronized JAS sequence and stream. Neither a
        // replacement stream nor a detached logical success stands in for it.
        auto &runtime = require_concrete_audio_runtime("Multi-BGM disabled start");
        if (lane == BgmLane::Stage) runtime.stop_stage_bgm(0);
        else runtime.stop_sub_bgm(0);
        return nullptr;
    }
    mBgm[bgm_index] = mKeeper.get(AudBgmKeeper::BgmType_Single);
    if (mBgm[bgm_index] == nullptr) return nullptr;
    mBgm[bgm_index]->setVolumeController(&mVolumeController[bgm_index]);
    JAISoundHandle *handle;
    try {
        handle = mBgm[bgm_index]->start(sound_id, prepared);
    } catch (...) {
        release_bgm_object(lane);
        throw;
    }
    if (handle != nullptr) {
        mLastBGM[bgm_index] = mCurrentBGM[bgm_index];
        mCurrentBGM[bgm_index] = sound_id;
    }
    return handle;
}

void AudBgmMgr::setNextBGM(s32 bgm_index, u32 sound_id) {
    mNextBGM[bgm_index] = sound_id;
}

void AudBgmMgr::clearNextBGM(s32 bgm_index) {
    mNextBGM[bgm_index] = static_cast<u32>(-1);
}

JAISoundHandle *AudBgmMgr::startLastBGM(s32 bgm_index) {
    if (mLastBGM[bgm_index] == static_cast<u32>(-1)) return nullptr;
    return start(bgm_index, mLastBGM[bgm_index], false);
}

void AudBgmMgr::clearLastBGM(s32 bgm_index) {
    mLastBGM[bgm_index] = static_cast<u32>(-1);
    if (bgm_index == BgmType_Stage) {
        smgpc::compat::require_active_audio_event_service().clear_last_stage_bgm_id();
    }
}

void AudBgmMgr::pause() {
    for (auto *bgm : mBgm) if (bgm != nullptr) bgm->pause(true);
}

void AudBgmMgr::unpause() {
    for (auto *bgm : mBgm) if (bgm != nullptr) bgm->pause(false);
}

void AudBgmMgr::volDownLevel(bool immediate) {
    for (auto &controller : mVolumeController) controller.volDown(immediate);
}

void AudBgmMgr::volDownStageBgmWhenSubBgmPlaying() {
    auto *bgm = mBgm[BgmType_Sub];
    if (bgm == nullptr) return;
    auto *handle = bgm->getHandle();
    if (handle == nullptr) return;
    if (handle->isSoundAttached() && mBgm[BgmType_Stage] != nullptr) {
        mVolumeController[BgmType_Stage].interruptedByOther();
    }
    auto *rhythm = bgm->getRhythmHandle();
    if (rhythm != nullptr && rhythm->isSoundAttached()) setBgmToRhythmDominant(BgmType_Sub);
}

void AudBgmMgr::startNextBgmWhenStopping(s32 bgm_index) {
    if (mNextBGM[bgm_index] == static_cast<u32>(-1) || mBgm[bgm_index] == nullptr) return;
    if (mBgm[bgm_index]->isStopping()) {
        start(bgm_index, mNextBGM[bgm_index], false);
        clearNextBGM(bgm_index);
    }
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
        (void)smgpc::compat::require_active_audio_event_service();
        if (auto *runtime = try_concrete_audio_runtime()) {
            (void)reconcile_bgm(BgmLane::Stage, runtime->j_audio_playback());
            (void)reconcile_bgm(BgmLane::Sub, runtime->j_audio_playback());
        }
        return &s_bgm_manager;
    }

    AudBgm *getStageBgm() {
        auto &audio = smgpc::compat::require_active_audio_event_service();
        if (!audio.is_stage_bgm_identity_resolved()) {
            aurora::throw_host_exception<std::logic_error>("The current stage-BGM identity has not been resolved.");
        }
        auto *runtime = try_concrete_audio_runtime();
        if (runtime == nullptr) {
            if (audio.has_active_stage_bgm()) unavailable("active stage-BGM state without a concrete RuntimeContext backend");
            return nullptr;
        }
        auto &playback = runtime->j_audio_playback();
        if (audio.has_active_stage_bgm() && !playback.has_active_bgm(BgmLane::Stage)) {
            aurora::throw_host_exception<std::logic_error>("Logical stage-BGM state has no matching concrete backend voice");
        }
        if (audio.has_active_stage_bgm() && audio.current_stage_bgm_id() != playback.bgm_id(BgmLane::Stage)) {
            aurora::throw_host_exception<std::logic_error>("Logical stage-BGM identity disagrees with its concrete backend voice");
        }
        return reconcile_bgm(BgmLane::Stage, playback);
    }

    AudBgm *getSubBgm() {
        auto *runtime = try_concrete_audio_runtime();
        if (runtime == nullptr) {
            auto *audio = smgpc::compat::try_active_audio_event_service();
            if (audio != nullptr && audio->has_active_sub_bgm()) unavailable("active sub-BGM state without a concrete RuntimeContext backend");
            return nullptr;
        }
        return reconcile_bgm(BgmLane::Sub, runtime->j_audio_playback());
    }

    JAISoundHandle *startStageBgm(u32 sound_id, bool prepared) {
        return s_bgm_manager.start(AudBgmMgr::BgmType_Stage, sound_id, prepared);
    }

    JAISoundHandle *startSubBgm(u32 sound_id, bool prepared) {
        return s_bgm_manager.start(AudBgmMgr::BgmType_Sub, sound_id, prepared);
    }

    void setNextIdStageBgm(u32 sound_id) {
        s_bgm_manager.setNextBGM(AudBgmMgr::BgmType_Stage, sound_id);
    }

    JAISoundHandle *startLastStageBgm() {
        return s_bgm_manager.startLastBGM(AudBgmMgr::BgmType_Stage);
    }

    AudSoundObject *getSystemSeObject() {
        auto* object = aurora::audio::disabled_system_sound_object();
        if (object == nullptr) {
            unavailable("system-SE object access without its owner");
        }
        return object;
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
