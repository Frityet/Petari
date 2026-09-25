#pragma once

#include <aurora/ppc_bitfield.hpp>

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
    /* 0x20 */ union {
        u32 _20;
        struct {
            AURORA_PPC_BITFIELD_GROUP(u32, (_0, 4), (mDamageType, 4), (_8, 24))
        } mFlags;
    };
};
