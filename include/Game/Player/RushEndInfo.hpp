#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class RushEndInfo {
public:
    RushEndInfo(LiveActor*, u32, const TVec3f&, bool, u32);

    u32 mMagic;       // 0x0
    u32 mType;        // 0x4
    TVec3f mPosition; // 0x8
    bool mIsUnknown;  // 0x14
    u8 _15[3];
    u32 mFlags;         // 0x18
    LiveActor* mActor;  // 0x1C
    u32 mUnused;        // 0x20
};
