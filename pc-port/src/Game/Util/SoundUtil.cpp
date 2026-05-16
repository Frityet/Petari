#include "Game/Util/SoundUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {

void startStageBGM(const char* pName, bool) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->start_stage_bgm(pName);
    }
}

bool isPreparedStageBgm() {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        return runtime->is_stage_bgm_prepared();
    }

    return false;
}

void unlockStageBGM() {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->unlock_stage_bgm();
    }
}

void stopStageBGM(s32 fadeFrames) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->stop_stage_bgm(fadeFrames);
    }
}

void startSystemSE(const char* pName, s32, s32) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->start_system_sound(pName);
    }
}

void startCSSound(const char* pName, s32, s32) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->start_cs_sound(pName);
    }
}

}  // namespace MR

