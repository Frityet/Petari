#include "Game/Util/SoundUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {

    void startCurrentStageBGM() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_stage_bgm(runtime->current_stage_name());
        }
    }

    void startStageBGM(const char* pName, bool) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_stage_bgm(pName);
        }
    }

    bool isPlayingStageBgmName(const char* pName) {
        if (pName == nullptr) {
            return false;
        }

        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            return runtime->current_stage_bgm_name() == pName;
        }

        return false;
    }

    bool isStopOrFadeoutBgmName(const char* pName) {
        return !isPlayingStageBgmName(pName);
    }

    bool isPreparedStageBgm() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            return runtime->is_stage_bgm_prepared();
        }

        return false;
    }

    void unlockStageBGM() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->unlock_stage_bgm();
        }
    }

    void stopStageBGM(s32 fadeFrames) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->stop_stage_bgm(fadeFrames);
        }
    }

    void setStageBGMState(s32 state, u32 changeFrames) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->set_stage_bgm_state(state, changeFrames);
        }
    }

    void startSystemSE(const char* pName, s32, s32) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_system_sound(pName);
        }
    }

    void startSystemLevelSE(const char* pName, s32, s32) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_system_sound(pName);
        }
    }

    void stopSystemSE(const char*, u32) {
    }

    void startAtmosphereSE(const char* pName, s32, s32) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_system_sound(pName);
        }
    }

    void submitLevelSE() {
    }

    void permitLevelSE() {
    }

    void startSystemME(const char* pName) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_system_sound(pName);
        }
    }

    void startCSSound(const char* pName, s32, s32) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_cs_sound(pName);
        }
    }

    void startCSSound(const char* pName, const char*, s32) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->start_cs_sound(pName);
        }
    }

}  // namespace MR
