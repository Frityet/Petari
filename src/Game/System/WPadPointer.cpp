#include "Game/System/WPadPointer.hpp"
#include "Game/Util.hpp"
#include <JSystem/JUtility/JUTVideo.hpp>

// arrays are generating a constructor for some reason
WPadPointer::WPadPointer(const WPad* pPad) {
    mPad = pPad;
    mPosPlayRadius = 0.03f;
    mPosSensitivity = 0.5f;
    mHoriPlayRadius = 0.0;
    mHoriSensitivity = 1.0f;
    mDistPlayRadius = 0.0f;
    mDistSensitivity = 1.0f;
    _2C = 0;
    mDistDisplay = 0.0f;
    _34 = 0;
    _38 = 0;
    _3C = 0;
    mEnablePastCount = 0;
    mIsPointInScreen = 0;
    _45 = 0;
    mPointingPosArray = new TVec2f[0x78];
    mHorizonArray = new TVec2f[0x78];
    _C = 120;
    reset();
}

void WPadPointer::reset() {
    for (s32 i = 0; i < _C; i++) {
        TVec2f* cur = &mPointingPosArray[i];
        cur->x = 0.0f;
        cur->y = 0.0f;
        cur = &mHorizonArray[i];
        cur->x = 0.0f;
        cur->y = 0.0f;
    }

    _34 = 0;
    mDistDisplay = 0.0f;
    _38 = 0;
    mEnablePastCount = 0;
    _2C = 0;
    mIsPointInScreen = false;
    _45 = 0;
    KPADSetPosParam(mPad->mChannel, mPosPlayRadius, mPosSensitivity);
    KPADSetHoriParam(mPad->mChannel, mHoriPlayRadius, mHoriSensitivity);
    KPADSetDistParam(mPad->mChannel, mDistPlayRadius, mDistSensitivity);
}

void WPadPointer::setSensorBarLevel(f32 lvl) {
    KPADSetSensorHeight(mPad->mChannel, lvl);
}

void WPadPointer::update() {
    KPADStatus* pStatus = mPad->getKPadStatus(0);
    if (pStatus == nullptr) {
        reset();
        return;
    }

    _45 = 0;
    mDistDisplay = pStatus->dist;
    _34 = pStatus->dpd_valid_fg;
    s32 count = mPad->getValidStatusCount();
    mEnablePastCount = 0;
    if (count > _C) {
        count = _C;
    }

    bool isAnyDPDValid = false;
    for (s32 i = count - 1; i >= 0; i--) {
        KPADStatus* pStatus = mPad->getKPadStatus(i);
        if (pStatus->dpd_valid_fg > 0) {
            isAnyDPDValid = true;
        }
        if (pStatus->dpd_valid_fg < 2) {
            _38 = 0;
            _3C++;
        } else {
            if ((!mIsPointInScreen && static_cast<s32>(_38) >= 5) || (mIsPointInScreen && static_cast<s32>(_3C) <= 10)) {
                mPointingPosArray[mEnablePastCount].x = pStatus->pos.x;
                mPointingPosArray[mEnablePastCount].y = pStatus->pos.y;
                mHorizonArray[mEnablePastCount].x = pStatus->horizon.x;
                mHorizonArray[mEnablePastCount].y = pStatus->horizon.y;
                if (pStatus->speed > 0.0001f) {
                    _45 = 1;
                }
                mEnablePastCount++;
            }
            _3C = 0;
            _38++;
        }
    }

    mIsPointInScreen = mEnablePastCount != 0;
    if (isAnyDPDValid) {
        _2C = 0;
    } else if (static_cast<s32>(_2C) < 20) {
        _2C++;
    }
}

void WPadPointer::getPointingPos(TVec2f* pOut) const {
    if (mIsPointInScreen != 0) {
        pOut->set(mPointingPosArray[mEnablePastCount - 1]);
    } else {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
    }
}

void WPadPointer::getHorizonVec(TVec2f* pOut) const {
    if (mIsPointInScreen != 0) {
        pOut->set(mHorizonArray[mEnablePastCount - 1]);
    } else {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
    }
}

void WPadPointer::getPastPointingPos(TVec2f* pOut, s32 idx) const {
    pOut->set(mPointingPosArray[mEnablePastCount - 1 - idx]);
}

u32 WPadPointer::getEnablePastCount() const {
    return mEnablePastCount;
}

void WPadPointer::getPointingPosBasedOnScreen(TVec2f* pOut) const {
    pOut->x = (0.5f + (0.5f * mPointingPosArray->x)) * (int)MR::getScreenWidth();
    pOut->y = (0.5f + (0.5f * mPointingPosArray->y)) * (int)(JUTVideo::getManager()->getRenderMode()->efbHeight);
}
