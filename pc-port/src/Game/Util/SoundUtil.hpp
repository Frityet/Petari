#pragma once

#include <revolution/types.h>

namespace MR {
    void startCurrentStageBGM();
    void startStageBGM(const char* pName, bool isPrepare);
    bool isPlayingStageBgmName(const char* pName);
    bool isStopOrFadeoutBgmName(const char* pName);
    bool isPreparedStageBgm();
    void unlockStageBGM();
    void stopStageBGM(s32 fadeFrames);
    void setStageBGMState(s32 state, u32 changeFrames);
    void startSystemSE(const char* pName, s32, s32);
    void startSystemLevelSE(const char* pName, s32, s32);
    void stopSystemSE(const char* pName, u32 delay);
    void startSystemME(const char* pName);
    void startCSSound(const char* pName, s32, s32);
    void startCSSound(const char* pName, const char*, s32);
}  // namespace MR
