#include "Game/Util/SoundUtil.hpp"

#include "compat/AudioFacadeCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>

namespace MR {
    JAISoundHandle *startSystemSE(const char *pName, s32, s32) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->start_system_sound(pName);
        }

        return nullptr;
    }

    JAISoundHandle *startSystemLevelSE(const char *pName, s32, s32) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->start_system_level_sound(pName);
        }

        return nullptr;
    }

    void stopSystemSE(const char *pName, u32 delay) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->stop_system_sound(pName, delay);
        }
    }

    JAISoundHandle *startAtmosphereSE(const char *pName, s32, s32) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->start_atmosphere_sound(pName);
        }

        return nullptr;
    }

    JAISoundHandle *startAtmosphereLevelSE(const char *, s32, s32) {
        throw std::logic_error(
            "Atmosphere level sound playback is unavailable: the PC audio backend "
            "does not implement AudWrap::getAtmosphereSeObject()->startLevelSoundParam.");
    }

    JAISoundHandle *startSound(const LiveActor *, const char *pName, s32, s32) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->start_system_sound(pName);
        }

        return nullptr;
    }

    JAISoundHandle *startLevelSound(const LiveActor *, const char *pName, s32, s32, s32) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->start_system_level_sound(pName);
        }

        return nullptr;
    }

    bool hasME() {
        return smgpc::runtime::RuntimeContext::try_instance() != nullptr;
    }

    void startSystemME(const char *pName) {
        if (!hasME() || pName == nullptr) {
            return;
        }

        smgpc::runtime::RuntimeContext::instance().start_system_me(pName);
    }

    JAISoundHandle *startStageBGM(const char *pName, bool) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->start_stage_bgm(pName);
        }

        return nullptr;
    }

    void stopStageBGM(u32 fadeFrames) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->stop_stage_bgm(fadeFrames);
        }
    }

    void unlockStageBGM() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->unlock_stage_bgm();
        }
    }

    bool isPlayingStageBgm() {
        return smgpc::compat::require_active_audio_event_service().has_active_stage_bgm();
    }

    bool isPlayingStageBgmID(u32 id) {
        const auto &audio = smgpc::compat::require_active_audio_event_service();
        if (!audio.is_stage_bgm_identity_resolved()) {
            throw std::logic_error("The current stage-BGM raw ID has not been resolved.");
        }
        if (!audio.has_active_stage_bgm()) {
            return false;
        }
        const auto current_id = audio.current_stage_bgm_id();
        if (!current_id.has_value()) {
            throw std::logic_error("An active stage BGM is missing its resolved raw ID.");
        }
        return *current_id == id;
    }

    bool isPlayingStageBgmName(const char *pName) {
        if (pName == nullptr) {
            return false;
        }
        const auto &audio = smgpc::compat::require_active_audio_event_service();
        return audio.has_active_stage_bgm() && audio.current_stage_bgm_name() == pName;
    }

    bool isStopOrFadeoutStageBgmID(u32 id) {
        return !isPlayingStageBgmID(id);
    }

    bool isStopOrFadeoutBgmName(const char *pName) {
        return !isPlayingStageBgmName(pName);
    }

    bool isPreparedStageBgm() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            return runtime->is_stage_bgm_prepared();
        }

        return false;
    }

    void setStageBGMState(s32 state, u32 changeFrames) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->set_stage_bgm_state(state, changeFrames);
        }
    }

    void setCubeBgmChangeInvalid() {
        smgpc::compat::require_active_audio_event_service().set_cube_bgm_change_invalid(true);
    }

    bool isCubeBgmChangeInvalid() {
        return smgpc::compat::require_active_audio_event_service().is_cube_bgm_change_invalid();
    }

    void submitLevelSE() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->submit_level_sound();
        }
    }

    void permitLevelSE() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->permit_level_sound();
        }
    }

    void startCSSound(const char *pName, const char *, s32) {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pName != nullptr) {
            runtime->start_cs_sound(pName);
        }
    }

    void startCurrentStageBGM() {
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->start_stage_bgm(runtime->current_stage_name());
        }
    }
}  // namespace MR
