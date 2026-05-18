#pragma once

#include "Game/LiveActor/LiveActor.hpp"

#include <revolution/types.h>

struct TVec2f {
    f32 x = 0.0F;
    f32 y = 0.0F;

    [[nodiscard]] f32 squareDist(const TVec2f& value) const {
        const auto dx = x - value.x;
        const auto dy = y - value.y;
        return (dx * dx) + (dy * dy);
    }
};

namespace MR {
    void getCorePadPointingPosBasedOnScreen(TVec2f* pPos, s32 channel);
    void getCorePadPointingPos(TVec2f* pPos, s32 channel);
    void getCorePadPastPointingPos(TVec2f* pPos, s32 idx, s32 channel);
    s32 getCorePadEnablePastCount(s32 channel);
    bool isCorePadPointInScreen(s32 channel);
    f32 getCorePadDistanceToDisplay(s32 channel);
    void getCorePadAcceleration(TVec3f* pAccel, s32 channel);
    bool testCorePadButtonUp(s32 channel);
    bool testCorePadButtonDown(s32 channel);
    bool testCorePadButtonLeft(s32 channel);
    bool testCorePadButtonRight(s32 channel);
    bool testCorePadButtonA(s32 channel);
    bool testCorePadButtonB(s32 channel);
    bool testCorePadButtonPlus(s32 channel);
    bool testCorePadButtonMinus(s32 channel);
    bool testSubPadButtonC(s32 channel);
    bool testSubPadButtonZ(s32 channel);
    bool testPadButtonAnyWithoutHome(s32 channel);
    bool testCorePadTriggerUp(s32 channel);
    bool testCorePadTriggerDown(s32 channel);
    bool testCorePadTriggerLeft(s32 channel);
    bool testCorePadTriggerRight(s32 channel);
    bool testCorePadTriggerA(s32 channel);
    bool testCorePadTriggerB(s32 channel);
    bool testCorePadTriggerPlus(s32 channel);
    bool testCorePadTriggerMinus(s32 channel);
    bool testCorePadTriggerAnyWithoutHome(s32 channel);
    bool testCorePadTriggerHome(s32 channel);
    bool testSubPadTriggerC(s32 channel);
    bool testSubPadTriggerZ(s32 channel);
    bool testSubPadReleaseZ(s32 channel);
    bool isCorePadSwing(s32 channel);
    bool isCorePadSwingTrigger(s32 channel);
    f32 getSubPadStickX(s32 channel);
    f32 getSubPadStickY(s32 channel);
    bool testSubPadStickTriggerUp(s32 channel);
    bool testSubPadStickTriggerDown(s32 channel);
    bool testSubPadStickTriggerLeft(s32 channel);
    bool testSubPadStickTriggerRight(s32 channel);
    void getSubPadAcceleration(TVec3f* pAccel, s32 channel);
    bool isSubPadSwing(s32 channel);
    bool isPadSwing(s32 channel);
    bool testSystemPadTriggerDecide();
    bool testSystemTriggerA();
    bool testSystemTriggerB();
    bool testDPDMenuPadDecideTrigger();
    bool testFpViewStartTrigger();
    bool testFpViewOutTrigger();
    f32 getPlayerStickX();
    f32 getPlayerStickY();
    bool getPlayerTriggerA();
    bool getPlayerTriggerB();
    bool getPlayerTriggerZ();
    bool getPlayerTriggerC();
    bool getPlayerLevelA();
    bool getPlayerLevelB();
    bool getPlayerLevelZ();
    bool getPlayerLevelC();
    bool isGamePadStickOperated(s32 channel);
    void calcWorldStickDirectionXZ(f32* pDirX, f32* pDirZ, s32 channel);
    void calcWorldStickDirectionXZ(TVec3f* pDir, s32 channel);
    u32 getWPadMaxCount();
    bool isConnectedWPad(s32 channel);
    bool isOperatingWPad(s32 channel);
}  // namespace MR

class WPadRumble;

namespace WPadFunction {
    WPadRumble* getWPadRumble(s32 channel);
}
