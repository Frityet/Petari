#pragma once

#include <memory>
class JMapInfo;

#include <revolution/types.h>

class PadRumbleInfo {
public:
    s32 mStartFrame;    // 0x0
    const char* mName;  // 0x4
};

class DemoPadRumbler {
public:
    DemoPadRumbler(const char*);
    ~DemoPadRumbler();
    void update(s32);

    s32 mNumPadRumbleEntries;          // 0x0
    PadRumbleInfo* mPadRumbleEntries;  // 0x4
    s32 _8;

private:
    // Original records borrow strings from this native parser until retirement.
    std::unique_ptr<JMapInfo> mNativeParser;
};
