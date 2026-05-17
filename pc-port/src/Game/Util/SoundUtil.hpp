#pragma once

#include <revolution/types.h>

namespace MR {
    void startStageBGM(const char* pName, bool isPrepare);
    bool isPreparedStageBgm();
    void unlockStageBGM();
    void stopStageBGM(s32 fadeFrames);
    void setStageBGMState(s32 state, u32 changeFrames);
    void startSystemSE(const char* pName, s32, s32);
    void startSystemLevelSE(const char* pName, s32, s32);
    void stopSystemSE(const char* pName, u32 delay);
    void startSystemME(const char* pName);
    void startCSSound(const char* pName, s32, s32);
}  // namespace MR
