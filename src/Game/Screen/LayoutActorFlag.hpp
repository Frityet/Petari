#pragma once

class LayoutActorFlag {
public:
    LayoutActorFlag() = default;

    /* 0x0 */ bool mIsDead = true;
    /* 0x1 */ bool mIsStopAnimFrame = false;
    /* 0x2 */ bool mIsHidden = false;
    /* 0x3 */ bool mIsOffCalcAnim = false;
};
