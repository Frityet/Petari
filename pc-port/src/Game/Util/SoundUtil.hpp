#pragma once

#include <revolution/types.h>

class LiveActor;

namespace MR {
    void startSound(const LiveActor* pActor, const char* pName, s32 param3 = -1, s32 param4 = -1);
    void startLevelSound(const LiveActor* pActor, const char* pName, s32 param3 = -1, s32 param4 = -1, s32 param5 = -1);
    void startCurrentStageBGM();
    void startStageBGM(const char* pName, bool isPrepare);
    bool isPlayingStageBgm();
    bool isPlayingStageBgmName(const char* pName);
    bool isStopOrFadeoutBgmName(const char* pName);
    bool isPreparedStageBgm();
    void unlockStageBGM();
    void stopStageBGM(s32 fadeFrames);
    void setStageBGMState(s32 state, u32 changeFrames);
    void startSystemSE(const char* pName, s32 param2 = -1, s32 param3 = -1);
    void startSystemLevelSE(const char* pName, s32, s32);
    void stopSystemSE(const char* pName, u32 delay);
    void startAtmosphereSE(const char* pName, s32, s32);
    void submitLevelSE();
    void permitLevelSE();
    void startSystemME(const char* pName);
    bool hasME();
    void startCSSound(const char* pName, s32, s32);
    void startCSSound(const char* pName, const char*, s32);
}  // namespace MR
