#include "Game/System/WPadHVSwing.hpp"

WPadHVSwing::WPadHVSwing(const WPad* pPad, u32 type)
    : mPad(pPad), _4(type), _8(1.0f), mIsSwing(false), _D(false), _10(0), _14(0), mIsTriggerSwing(false), _1C(0.4f), _20(false), _21(false), _24(0), _28(0) {
}
