#include "Game/System/WPadRumble.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadRumbleData.hpp"
#include "Game/Util/GamePadUtil.hpp"

void WPadRumble::pause() {
    WPADControlMotor(mPad->mChannel, WPAD_MOTOR_STOP);
}

void WPadRumble::stop() {
    _8 = false;

    pause();

    for (u8 i = 0; i < ARRAY_SIZE(mChannel); i++) {
        mChannel[i].clear();
    }
}

WPadRumble* WPadRumble::getRumbleInstance() const {
    return sInstanceForCallback[mPad->mChannel];
}

WPadRumble* WPad::getRumbleInstance() const {
    return _18->getRumbleInstance();
}
