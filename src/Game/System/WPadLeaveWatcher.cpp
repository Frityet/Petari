#include "Game/System/WPadLeaveWatcher.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPadButton.hpp"
#include "Game/System/WPadPointer.hpp"
#include "Game/System/WPadStick.hpp"

namespace {
    static const s32 sLeaveLongTime = 3600;
    bool sWatchPointer = false;
    bool sWatchCoreAcceleration = true;
    bool sWatchSubAcceleration = true;
    bool sWatchButton = true;
    bool sWatchStick = true;
};  // namespace

WPadLeaveWatcher::WPadLeaveWatcher(WPad* pPad) : mPad(pPad), mStep(0), mIsSuspend(false) {
}

void WPadLeaveWatcher::update() {
    if (mIsSuspend) {
        return;
    }

    if ((mPad->mPointer->_45 && sWatchPointer) || (!mPad->mCorePadAccel->isStationary() && sWatchCoreAcceleration) ||
        (!mPad->mSubPadAccel->isStationary() && sWatchSubAcceleration) || (mPad->mButton->isChangeAnyState() && sWatchButton) ||
        (mPad->mStick->isChanged() && sWatchStick)) {
        mStep = 0;
    } else if (mStep < sLeaveLongTime) {
        mStep++;
    }
}

void WPadLeaveWatcher::start() {
    mIsSuspend = false;
}

void WPadLeaveWatcher::stop() {
    mIsSuspend = true;
}

void WPadLeaveWatcher::restart() {
    mStep = 0;
    mIsSuspend = false;
}
