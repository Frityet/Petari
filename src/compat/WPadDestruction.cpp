#include "Game/System/WPad.hpp"
#include "Game/System/WPadPointer.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPadButton.hpp"
#include "Game/System/WPadHVSwing.hpp"
#include "Game/System/WPadRumble.hpp"
#include "Game/System/WPadStick.hpp"
#include "Game/System/WPadLeaveWatcher.hpp"
#include "Game/System/WPadInfoChecker.hpp"

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
