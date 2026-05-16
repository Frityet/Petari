#pragma once

#include <revolution/types.h>

namespace MR {
    void startStageBGM(const char* pName, bool isPrepare);
    bool isPreparedStageBgm();
    void unlockStageBGM();
    void stopStageBGM(s32 fadeFrames);
    void startSystemSE(const char* pName, s32, s32);
    void startCSSound(const char* pName, s32, s32);
}

