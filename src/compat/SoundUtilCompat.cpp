#include <aurora/exception.hpp>
#include "Game/Util/GamePadUtil.hpp"
#include "Game/GameAudio/AudStageBgmWrap.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/GameAudio/AudBgmConductor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"

#include "compat/AudioFacadeCompat.hpp"
#include "compat/StageSessionState.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>
#include <string>

namespace {
    [[nodiscard]] smgpc::runtime::RuntimeContext &
    require_audio_runtime(const char *operation) {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                std::string(operation) +
                " requires an active RuntimeContext audio backend");
        }
        return *runtime;
    }

    [[nodiscard]] std::string_view require_sound_name(
        const char *name, const char *operation) {
        if (name == nullptr || name[0] == '\0') {
            aurora::throw_host_exception<std::invalid_argument>(
                std::string(operation) + " requires a retail sound name");
        }
        return name;
    }
}  // namespace

namespace MR {
    void setStageBGMStateBit(u32 bit) {
        AudBgmConductor* conductor = getSceneObj< AudBgmConductor >(SceneObj_AudBgmConductor);
        if (conductor != nullptr) {
            conductor->setStateBit(bit);
        }
    }

    void clearBgmQueue() {
        AudWrap::getBgmMgr()->clearLastBGM(0);
        AudWrap::getBgmMgr()->clearLastBGM(1);
        AudWrap::getBgmMgr()->clearNextBGM(0);
        AudWrap::getBgmMgr()->clearNextBGM(1);
    }

    void startSpinHitSound(const LiveActor* pActor) {
        startCSSound("CS_SPIN_HIT", nullptr, 0);
    }

    JAISoundHandle *startSystemSE(const char *pName, s32 parameter1,
                                  s32 parameter2) {
        const auto name = require_sound_name(pName, "System-SE playback");
        if (name == "SE_SY_READ_RIDDLE_S" && MR::isPlayerDead()) {
            return nullptr;
        }
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        auto *logical_audio =
            smgpc::compat::try_active_audio_event_service();
        if (logical_audio != nullptr &&
            (runtime == nullptr || logical_audio != &runtime->audio())) {
            logical_audio->start_system_sound(name);
            return nullptr;
        }
        return require_audio_runtime("System-SE playback")
            .start_system_sound(name, parameter1, parameter2);
    }

    JAISoundHandle *startSystemLevelSE(const char *pName, s32 parameter1,
                                       s32 parameter2) {
        return require_audio_runtime("System level-SE playback")
            .start_system_level_sound(
                require_sound_name(pName, "System level-SE playback"),
                parameter1, parameter2);
    }

    void stopSystemSE(const char *pName, u32 delay) {
        require_audio_runtime("System-SE stop")
            .stop_system_sound(
                require_sound_name(pName, "System-SE stop"), delay);
    }

    JAISoundHandle *startAtmosphereSE(const char *pName, s32 parameter1,
                                      s32 parameter2) {
        return require_audio_runtime("Atmosphere-SE playback")
            .start_atmosphere_sound(
                require_sound_name(pName, "Atmosphere-SE playback"),
                parameter1, parameter2);
    }

    JAISoundHandle *startAtmosphereLevelSE(const char *pName, s32 parameter1,
                                           s32 parameter2) {
        return require_audio_runtime("JAudio atmosphere level-sound playback")
            .start_atmosphere_level_sound(
                require_sound_name(
                    pName, "JAudio atmosphere level-sound playback"),
                parameter1, parameter2);
    }

    JAISoundHandle *startSound(const LiveActor *actor, const char *pName,
                               s32 parameter1, s32 parameter2) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Actor sound playback requires an actor identity");
        }
        const auto name = require_sound_name(pName, "Actor sound playback");
        smgpc::compat::require_active_audio_event_service().start_actor_sound(
            actor, actor->getName() != nullptr ? actor->getName() : "", name,
            parameter1, parameter2);
        // Positional JAudio mixing remains optional. A null handle truthfully
        // reports that this logical request has no concrete backend voice.
        return nullptr;
    }

    JAISoundHandle *startLevelSound(const LiveActor *actor, const char *pName,
                                    s32 parameter1, s32 parameter2,
                                    s32 parameter3) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Actor level-sound playback requires an actor identity");
        }
        const auto name =
            require_sound_name(pName, "Actor level-sound playback");
        smgpc::compat::require_active_audio_event_service()
            .start_actor_level_sound(
                actor, actor->getName() != nullptr ? actor->getName() : "",
                name, parameter1, parameter2, parameter3);
        return nullptr;
    }

    bool hasME() {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr && runtime->j_audio_playback().has_me();
    }

    void startSystemME(const char *pName) {
        if (!hasME() || pName == nullptr) {
            return;
        }

        smgpc::runtime::RuntimeContext::instance().start_system_me(pName);
    }

    JAISoundHandle *startStageBGM(const char *pName, bool prepared) {
        return require_audio_runtime("Stage-BGM playback")
            .start_stage_bgm(
                require_sound_name(pName, "Stage-BGM playback"), prepared);
    }

    void limitedSound(const char *pName, s32 limit) {
        smgpc::compat::require_active_audio_event_service()
            .register_limited_sound(
                require_sound_name(pName, "Limited-sound registration"),
                limit);
    }

    JAISoundHandle *startSubBGM(const char *pName, bool prepared) {
        return require_audio_runtime("Sub-BGM playback").start_sub_bgm(
            require_sound_name(pName, "Sub-BGM playback"), prepared);
    }

    void stopSubBGM(u32 fadeFrames) {
        require_audio_runtime("Sub-BGM stop").stop_sub_bgm(fadeFrames);
    }

    void stopStageBGM(u32 fadeFrames) {
        require_audio_runtime("Stage-BGM stop")
            .stop_stage_bgm(static_cast<s32>(fadeFrames));
    }

    void unlockStageBGM() {
        require_audio_runtime("Stage-BGM prepared unlock")
            .unlock_stage_bgm();
    }

    bool isPlayingStageBgm() {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr &&
               runtime->j_audio_playback().has_active_bgm(smgpc::runtime::BgmLane::Stage) &&
               !runtime->j_audio_playback().is_bgm_stopping(smgpc::runtime::BgmLane::Stage);
    }

    bool isPlayingStageBgmID(u32 id) {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr ||
            !runtime->j_audio_playback().has_active_bgm(smgpc::runtime::BgmLane::Stage) ||
            runtime->j_audio_playback().is_bgm_stopping(smgpc::runtime::BgmLane::Stage)) {
            return false;
        }
        const auto current_id = runtime->j_audio_playback().bgm_id(smgpc::runtime::BgmLane::Stage);
        if (!current_id.has_value()) {
            aurora::throw_host_exception<std::logic_error>("An active stage BGM is missing its resolved raw ID.");
        }
        return *current_id == id;
    }

    bool isPlayingStageBgmName(const char *pName) {
        if (pName == nullptr) {
            return false;
        }
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr ||
            !runtime->j_audio_playback().has_active_bgm(smgpc::runtime::BgmLane::Stage) ||
            runtime->j_audio_playback().is_bgm_stopping(smgpc::runtime::BgmLane::Stage)) {
            return false;
        }
        const auto wanted = runtime->j_audio_playback().find_sound_id(pName);
        if (!wanted.has_value()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Stage-BGM name is absent from the retail JAudio table: " +
                std::string(pName));
        }
        return runtime->j_audio_playback().bgm_id(smgpc::runtime::BgmLane::Stage) == wanted;
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
        require_audio_runtime("Stage-BGM track-state transition")
            .set_stage_bgm_state(state, changeFrames);
    }

    void setCubeBgmChangeInvalid() {
        smgpc::compat::require_active_audio_event_service().set_cube_bgm_change_invalid(true);
    }

    bool isCubeBgmChangeInvalid() {
        return smgpc::compat::require_active_audio_event_service().is_cube_bgm_change_invalid();
    }

    void setSoundVolumeSetting(s32 volumeSet, u32 maxSteps) {
        require_audio_runtime("SE volume preset")
            .j_audio_playback().set_sound_volume_setting(volumeSet, maxSteps);
    }

    void recoverSoundVolumeSetting(u32 maxSteps) {
        require_audio_runtime("SE volume preset recovery")
            .j_audio_playback().recover_sound_volume_setting(maxSteps);
    }

    void submitTrigSE() {
        require_audio_runtime("Trigger-SE submission")
            .j_audio_playback().set_trigger_sound_permitted(false);
    }

    void permitTrigSE() {
        require_audio_runtime("Trigger-SE permission")
            .j_audio_playback().set_trigger_sound_permitted(true);
    }

    void submitLevelSE() {
        require_audio_runtime("Level-SE submission").submit_level_sound();
    }

    void permitLevelSE() {
        require_audio_runtime("Level-SE permission").permit_level_sound();
    }

    void submitSE() {
        submitTrigSE();
        submitLevelSE();
    }

    void permitSE() {
        permitTrigSE();
        permitLevelSE();
    }

    bool isPermitSE() {
        return require_audio_runtime("SE permission query")
            .j_audio_playback().is_sound_permitted();
    }

    void startCSSound(const char *, const char *pSEName, s32) {
        // AudSpeakerWrap::isPlayable(-1) is false without a controller-speaker
        // backend. Retail then plays only the optional ordinary-SE substitute.
        if (pSEName != nullptr) {
            (void)startSystemSE(pSEName, -1, -1);
        }
    }

    void startCurrentStageBGM() {
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        if (runtime == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Current-stage BGM playback requires an active RuntimeContext");
        }
        const auto &session = smgpc::compat::require_active_stage_session();
        const auto sound_id = AudStageBgmWrap::changeStageNameToSoundID(
            session.scene_name().c_str(), session.stage_name().c_str(),
            session.scenario_no());
        if (!sound_id.isAnonymous()) {
            (void)runtime->start_stage_bgm(static_cast<u32>(sound_id), false);
        }
    }
}  // namespace MR

namespace MR {
    void startCSSound2P(const char* pCSName, const char* pSEName) {
        if (isConnectedWPad(1)) {
            startCSSound(pCSName, pSEName, 1);
        }
    }

    void start2PJumpAssistSound() {
        if (hasME()) {
            startSystemME("ME_2P_ASSIST_JUMP");
        } else {
            startSystemSE("SE_SY_2P_ASSIST_JUMP");
        }

        startCSSound2P("CS_DPD_JUMP", nullptr);
    }

    void start2PJumpAssistJustSound() {
        if (hasME()) {
            startSystemME("ME_2P_ASSIST_JUMP_L");
        } else {
            startSystemSE("SE_SY_2P_ASSIST_JUMP_L");
        }

        startCSSound2P("CS_DPD_JUMP_HIGH", nullptr);
    }
}

namespace MR {
    void startDPDHitSound() {
        if (hasME()) {
            startSystemME("ME_DPD_HIT");
        } else {
            startSystemSE("SE_SY_DPD_HIT");
        }

        startCSSound2P("CS_DPD_HIT", nullptr);
    }
}  // namespace MR
