#include "Game/System/GameSystemFunction.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadRumble.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include <revolution/wpad.h>

namespace GameSystemFunction {
    void onPauseBeginAllRumble() {
        for (s32 chan = WPAD_CHAN0; chan < WPAD_CHAN0 + MR::getWPadMaxCount(); chan++) {
            MR::getWPad(chan)->_18->stop();
            MR::getWPad(chan)->_1C->registInstance();
            MR::getWPad(chan)->_1C->stop();
        }
    }

    void onPauseEndAllRumble() {
        for (s32 chan = WPAD_CHAN0; chan < WPAD_CHAN0 + MR::getWPadMaxCount(); chan++) {
            MR::getWPad(chan)->_18->registInstance();
            MR::getWPad(chan)->_1C->stop();
        }
    }
}
