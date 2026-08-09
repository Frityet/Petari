#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class RushEndInfo {
public:
    RushEndInfo(LiveActor*, u32, const TVec3f&, bool, u32);
    
    /* 0x00 */ u32 mMagic;
    /* 0x04 */ u32 mType;
    /* 0x08 */ TVec3f mVec;
    /* 0x14 */ bool mUseVec;
    /* 0x18 */ u32 mTimer;
    /* 0x1C */ LiveActor* mActor;
    /* 0x20 */ u32 mFlags;
};
