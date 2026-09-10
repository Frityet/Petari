#include "Game/System/WPadHVSwing.hpp"
#include "Game/System/WPad.hpp"

WPadHVSwing::WPadHVSwing(const WPad* pPad, u32 type)
    : mPad(pPad), _4(type), _8(1.0f), mIsSwing(false), _D(false), _10(0), _14(0), mIsTriggerSwing(false), _1C(0.4f), _20(false), _21(false), _24(0), _28(0) {
}

void WPadHVSwing::updateSwing() {
    TVec3f pastAcceleration;
    TVec3f acceleration;
    if (!mPad->getPastAcceleration(&pastAcceleration, 20, _4) || !mPad->getAcceleration(&acceleration, _4)) {
        mIsSwing = false;
        return;
    }

    mIsSwing = acceleration.distance(pastAcceleration) >= _8;
    if (!mIsSwing) {
        _14++;
    } else {
        _14 = 0;
    }

    if (_D) {
        if (_14 > 6) {
            _D = false;
        }
    } else if (mIsSwing) {
        _D = true;
    }

    if (_D) {
        _10++;
    } else {
        _10 = 0;
    }
}

void WPadHVSwing::updateCentrifugal() {
    TVec3f pastAcceleration;
    TVec3f acceleration;
    if (!mPad->getPastAcceleration(&pastAcceleration, 20, _4) || !mPad->getAcceleration(&acceleration, _4)) {
        _20 = false;
        _21 = false;
        _24 = 0;
        _28 = 0;
    }

    f32 sum = 0.0f;
    f32 peak = 0.0f;
    if (mPad->getEnableAccelPastCount(_4) >= 15) {
        for (s32 i = 1; i < 15; i++) {
            TVec3f newerAcceleration;
            TVec3f olderAcceleration;
            mPad->getPastAcceleration(&newerAcceleration, i - 1, _4);
            mPad->getPastAcceleration(&olderAcceleration, i, _4);
            sum += newerAcceleration.y - olderAcceleration.y;
            if (peak < sum) {
                peak = sum;
            }
        }
    }

    _20 = peak > _1C;
    if (!_20) {
        _28++;
    } else {
        _28 = 0;
    }

    if (_21) {
        if (_28 > 8) {
            _21 = false;
        }
    } else if (_20) {
        _21 = true;
    }

    if (_21) {
        _24++;
    } else {
        _24 = 0;
    }
}

void WPadHVSwing::update() {
    updateSwing();
    updateCentrifugal();
    mIsTriggerSwing = false;
    if (_10 == 1) {
        mIsTriggerSwing = true;
    } else if (_24 >= 30 && (_24 - 30) % 15 == 0 && _28 < 15) {
        mIsTriggerSwing = true;
    }
}
