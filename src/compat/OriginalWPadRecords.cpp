#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadPointer.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPadButton.hpp"
#include "Game/System/WPadHVSwing.hpp"
#include "Game/System/WPadRumble.hpp"
#include "Game/System/WPadStick.hpp"
#include "Game/System/WPadLeaveWatcher.hpp"
#include "Game/System/WPadInfoChecker.hpp"
#include "Game/System/WPadRumbleData.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"

#define KPAD_STATUS_ARRAY_SIZE 120

WPadRumble** WPadRumble::sInstanceForCallback = nullptr;

namespace smgpc::compat {
void destroy_wpad_children(WPad& pad) noexcept {
    delete pad.mInfoChecker;
    delete pad.mLeaveWatcher;
    delete pad.mStick;
    delete pad.mSubPadSwing;
    delete pad.mSubPadAccel;
    delete pad._1C;
    delete pad._18;
    delete pad.mCorePadSwing;
    delete pad.mCorePadAccel;
    if (pad.mPointer) {
        delete[] pad.mPointer->mHorizonArray;
        delete[] pad.mPointer->mPointingPosArray;
        delete pad.mPointer;
    }
    delete pad.mButton;
}
} // namespace smgpc::compat

WPad::WPad(s32 channel)
    : mChannel(channel), mReadInfo(nullptr), mButton(nullptr), mPointer(nullptr), mCorePadAccel(nullptr), mCorePadSwing(nullptr), _18(nullptr),
      _1C(nullptr), mStick(nullptr), mSubPadAccel(nullptr), mSubPadSwing(nullptr), mLeaveWatcher(nullptr), mInfoChecker(nullptr), _34(true),
      mIsConnected(false), mIsSubPadConnected(false) {
    try {
        mButton = new WPadButton(this);
        mPointer = new WPadPointer(this);
        mCorePadAccel = new WPadAcceleration(this, WPAD_DEV_CORE);
        mCorePadSwing = new WPadHVSwing(this, WPAD_DEV_CORE);
        _18 = new WPadRumble(this);
        _1C = new WPadRumble(this);
        _18->registInstance();
        mSubPadAccel = new WPadAcceleration(this, WPAD_DEV_FREESTYLE);
        mSubPadSwing = new WPadHVSwing(this, WPAD_DEV_FREESTYLE);
        mStick = new WPadStick(this);
        mLeaveWatcher = new WPadLeaveWatcher(this);
        mInfoChecker = new WPadInfoChecker(this);
        mSubPadSwing->_8 = 2.0f;
    } catch (...) {
        smgpc::compat::destroy_wpad_children(*this);
        throw;
    }
}

void WPad::setReadInfo(WPadReadDataInfo* pReadInfo) {
    mReadInfo = pReadInfo;
}

KPADStatus* WPad::getKPadStatus(u32 index) const {
    return mReadInfo->getKPadStatus(index);
}

s32 WPad::getValidStatusCount() const {
    return mReadInfo->getValidStatusCount();
}

WPadReadDataInfo::WPadReadDataInfo() : mStatusArray(), mValidStatusCount() {
    mStatusArray = new KPADStatus[KPAD_STATUS_ARRAY_SIZE];

    MR::zeroMemory(mStatusArray, sizeof(KPADStatus) * KPAD_STATUS_ARRAY_SIZE);
}

KPADStatus* WPadReadDataInfo::getKPadStatus(u32 index) const {
    if (index >= mValidStatusCount) {
        return nullptr;
    }

    return &mStatusArray[index];
}

u32 WPadReadDataInfo::getValidStatusCount() const {
    return mValidStatusCount;
}

WPadAcceleration::WPadAcceleration(const WPad* pPad, u32 type)
    : mPad(pPad), _4(type), _8(0.0f), _C(0.15f), _10(0, 0, 0), _1C(0.0f), _20(true), _624(-1), _628(0), _62C(0, 0, 0), _638(0.0f, 0.0f, 0.0f),
      _644(128), _648(0), _64C(0) {
    KPADSetAccParam(mPad->mChannel, _8, _C);
}

WPadButton::WPadButton(const WPad* pPad)
    : mPad(pPad), mHold(0), mTrigger(0), mRelease(0), mRepeat(0), mDelaySec(1.0f / 2.4f), mPulseSec(1.0f / 6.0f) {
    KPADSetBtnRepeat(mPad->mChannel, mDelaySec, mPulseSec);
}

void WPadButton::update() {
    KPADStatus* pStatus = mPad->getKPadStatus(0);

    if (pStatus == nullptr) {
        return;
    }

    if (mHold != 0) {
        mTrigger = pStatus->trig & ~mHold & KPAD_BUTTON_MASK;
    } else {
        mTrigger = pStatus->trig;
    }

    if (pStatus->wpad_err == WPAD_ERR_NONE || pStatus->wpad_err == WPAD_ERR_BUSY) {
        mHold = pStatus->hold;
        mRelease = pStatus->release;
    }

    mRepeat = mTrigger;

    if ((pStatus->hold & KPAD_BUTTON_RPT) != 0) {
        mRepeat |= mHold;
    }
}

void RumbleChannel::clear() {
    _0 = nullptr;
    _8 = 0;
    _C = 0;
    _E = false;
    _4 = false;
    _10 = nullptr;
}

WPadRumble::WPadRumble(WPad* pPad) : mPad(pPad), _8(false), _C(1), _B0(0), _B4(0), _B8(false), _BC(0) {
    smgpc::compat::JkrHostAllocationScope host;
    if (sInstanceForCallback == nullptr) {
        sInstanceForCallback = new WPadRumble*[MR::getWPadMaxCount()];

        for (u32 i = 0; i < MR::getWPadMaxCount(); i++) {
            sInstanceForCallback[i] = nullptr;
        }

        RumbleData::initHashValue();
        RumbleData::checkHashCollision();
    }

    for (u8 i = 0; i < ARRAY_SIZE(mChannel); i++) {
        mChannel[i].clear();
    }
}

WPadRumble::~WPadRumble() {
    s32 chan = mPad->mChannel;

    if (_8) {
        _8 = false;

        WPADControlMotor(chan, WPAD_MOTOR_STOP);
    }

    sInstanceForCallback[chan] = nullptr;
}

void WPadRumble::registInstance() {
    s32 chan = mPad->mChannel;

    sInstanceForCallback[chan] = this;

    if (_8) {
        WPADControlMotor(chan, WPAD_MOTOR_RUMBLE);
    }
}

WPadLeaveWatcher::WPadLeaveWatcher(WPad* pPad) : mPad(pPad), mStep(0), mIsSuspend(false) {
}

WPadInfoChecker::WPadInfoChecker(WPad* pPad) : mPad(pPad) {
    reset();
}

void WPadInfoChecker::reset() {
    mCheckInfoFrame = 0;
    mBattery = -1;
}
