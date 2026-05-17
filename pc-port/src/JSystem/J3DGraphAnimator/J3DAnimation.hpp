#pragma once

#include <revolution/types.h>

class J3DFrameCtrl {
public:
    J3DFrameCtrl() = default;

    /* 0x04 */ u8 mAttribute = 0;
    /* 0x05 */ u8 mState = 0;
    /* 0x06 */ s16 mStart = 0;
    /* 0x08 */ s16 mEnd = 0;
    /* 0x0A */ s16 mLoop = 0;
    /* 0x0C */ f32 mRate = 1.0F;
    /* 0x10 */ f32 mFrame = 0.0F;
};
