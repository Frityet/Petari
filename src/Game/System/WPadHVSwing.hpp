#pragma once

#include <revolution.h>

class WPad;

class WPadHVSwing {
public:
    WPadHVSwing(const WPad*, u32);

    void updateSwing();
    void updateCentrifugal();
    void update();

    const WPad* mPad;  // 0x0
    u32 _4;
    f32 _8;
    bool mIsSwing;  // 0xC
    bool _D;
    s32 _10;
    s32 _14;
    bool mIsTriggerSwing;  // 0x18
    f32 _1C;
    bool _20;
    bool _21;
    s32 _24;
    s32 _28;
};
