#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class RushEndInfo {
public:
    RushEndInfo(LiveActor*, u32, const TVec3f&, bool, u32);

    u32 mMagic;
    u32 mType;
    TVec3f mVec;
    bool mUseVec;
    u8 _15[3];
    u32 mTimer;
    LiveActor* mActor;
    u32 mFlags;
};
