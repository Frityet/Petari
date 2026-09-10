#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPad.hpp"
#include "Game/Util/MathUtil.hpp"
#include <revolution/wpad.h>
#include <revolution/kpad.h>

namespace {
    bool sLimitSwingDirection;
}

WPadAcceleration::WPadAcceleration(const WPad* pPad, u32 type)
    : mPad(pPad), _4(type), _8(0.0f), _C(0.15f), _10(0, 0, 0), _1C(0.0f), _20(true), _624(-1), _628(0), _62C(0, 0, 0), _638(0.0f, 0.0f, 0.0f),
      _644(128), _648(0), _64C(0) {
    KPADSetAccParam(mPad->mChannel, _8, _C);
}

bool WPadAcceleration::getAcceleration(TVec3f* pOut) const {
    if (_628 <= 0) {
        pOut->zero();
        return false;
    }

    pOut->set< f32 >(mHistory[_624]);
    return true;
}

bool WPadAcceleration::getPastAcceleration(TVec3f* pOut, s32 idx) const {
    if (idx >= _628) {
        pOut->zero();
        return false;
    }

    s32 slot = _624 - idx;
    if (slot < 0) {
        slot += 128;
    }

    pOut->set< f32 >(mHistory[slot]);
    return true;
}

bool WPadAcceleration::isStationary() const {
    return _20;
}

bool WPadAcceleration::isBalanced() const {
    return _1C < 0.018f;
}

void WPadAcceleration::update() {
    s32 cnt = mPad->getValidStatusCount();
    if (cnt <= 0) {
        TVec3f v17;

        if (_624 == -1) {
            v17.set< f32 >(0.0f, 0.0f, 0.0f);
        } else {
            v17 = mHistory[_624];
        }

        _624++;

        if (_624 >= ARRAY_SIZE(mHistory)) {
            _624 = 0;
        }

        mHistory[_624] = v17;

        if (_628 < ARRAY_SIZE(mHistory)) {
            _628++;
        }
    }

    for (s32 i = cnt - 1; i >= 0; --i) {
        KPADStatus* status = mPad->getKPadStatus(i);

        if (status == nullptr) {
            continue;
        }

        TVec3f v16;

        if (_4 == 0) {
            v16.x = -status->acc.x;
            v16.y = status->acc.z;
            v16.z = -status->acc.y;
        } else {
            if (!MR::isDeviceFreeStyle(status)) {
                continue;
            }

            v16.x = -status->ex_status.fs.acc.x;
            v16.y = status->ex_status.fs.acc.z;
            v16.z = -status->ex_status.fs.acc.y;
        }

        _624++;

        if (_624 >= ARRAY_SIZE(mHistory)) {
            _624 = 0;
        }

        mHistory[_624] = v16;

        if (_628 < ARRAY_SIZE(mHistory)) {
            _628++;
        }
    }

    _62C.zero();

    for (s32 j = 0; j < _628; ++j) {
        TVec3f v15;
        getPastAcceleration(&v15, j);
        _62C += v15;
    }

    if (_628 > 0) {
        f32 v18 = _628;

        _62C.x *= (1.0f / v18);
        _62C.y *= (1.0f / v18);
        _62C.z *= (1.0f / v18);
    }

    updateRotate();
    updateAccAverage();
    updateIsStable();
}

void WPadAcceleration::updateRotate() {
    _638.zero();
    s32 count = _644 < _628 ? _644 : _628;

    if (count < 3) {
        return;
    }

    TVec3f first;
    getPastAcceleration(&first, 0);
    _648 = 0;
    _64C = 0;
    f32 horizontal = 0.0f;
    f32 vertical = 0.0f;
    bool isHorizontal = false;
    bool isVertical = false;

    for (s32 i = 2; i < count; ++i) {
        TVec3f previous;
        TVec3f current;
        TVec3f cross;
        getPastAcceleration(&previous, i - 1);
        getPastAcceleration(&current, i);
        cross.cross(previous - first, current - first);
        _638 += cross;

        if (i < 20) {
            continue;
        }

        if (!isHorizontal && __fabs(_638.z) > 6.0f) {
            isHorizontal = true;
            _648 = _638.z > 0.0f ? 1 : -1;
            horizontal = __fabs(_638.z);
        }

        if (!isVertical && __fabs(_638.x) > 6.0f) {
            isVertical = true;
            _64C = _638.x > 0.0f ? 1 : -1;
            vertical = __fabs(_638.z);
        }
    }

    if (sLimitSwingDirection) {
        if (horizontal < vertical) {
            _648 = 0;
        }

        if (vertical < horizontal) {
            _64C = 0;
        }
    }
}

void WPadAcceleration::updateAccAverage() {
    f32 total = 0.0f;
    s32 count = 0;
    TVec3f previous(0.0f, 0.0f, 0.0f);

    if (getPastAcceleration(&previous, 0)) {
        ++count;
    }

    for (s32 i = 1; i < 32; ++i) {
        TVec3f current;
        if (!getPastAcceleration(&current, i)) {
            break;
        }

        total += current.squared(previous);
        previous = current;
        ++count;
    }

    if (count > 0) {
        _1C = total / count;
    } else {
        _1C = 0.0f;
    }
}

void WPadAcceleration::updateIsStable() {
    TVec3f v2 = mHistory[0] - _10;
    _20 = 1;

    if (__fabs(v2.x) >= 0.30f || __fabs(v2.y) >= 0.30f || __fabs(v2.z) >= 0.30f) {
        _10 = mHistory[0];
        _20 = 0;
    }
}
