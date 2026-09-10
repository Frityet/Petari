#pragma once

#include <revolution/os.h>

class JASCriticalSection {
public:
    JASCriticalSection() : mInterruptState(OSDisableInterrupts()) {};
    ~JASCriticalSection() {
        OSRestoreInterrupts(mInterruptState);
    }
    /* 0x00 */ BOOL mInterruptState;
};
