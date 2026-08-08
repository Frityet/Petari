#include "Game/Util/GamePadUtil.hpp"

#include "runtime/RuntimeContext.hpp"

#include <aurora/wpad.hpp>

#include <cmath>

namespace {
    constexpr auto cCoreButtonMask = WPAD_BUTTON_UP | WPAD_BUTTON_DOWN | WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT | WPAD_BUTTON_A |
                                     WPAD_BUTTON_B | WPAD_BUTTON_1 | WPAD_BUTTON_2 | WPAD_BUTTON_PLUS | WPAD_BUTTON_MINUS;
    constexpr auto cAnyButtonWithoutHomeMask = cCoreButtonMask | WPAD_BUTTON_C | WPAD_BUTTON_Z;

    static_assert((cCoreButtonMask & (WPAD_BUTTON_C | WPAD_BUTTON_Z | WPAD_BUTTON_HOME)) == 0U);

    [[nodiscard]] const aurora::WpadService& wpad_service() {
        return smgpc::runtime::RuntimeContext::instance().wpad();
    }

    [[nodiscard]] bool is_held(s32 channel, u32 mask) {
        return wpad_service().is_button_held(channel, mask);
    }

    [[nodiscard]] bool is_triggered(s32 channel, u32 mask) {
        return wpad_service().is_button_triggered(channel, mask);
    }

    [[nodiscard]] bool is_released(s32 channel, u32 mask) {
        return wpad_service().is_button_released(channel, mask);
    }
}  // namespace

namespace MR {
    void getCorePadPointingPosBasedOnScreen(TVec2f* pPos, s32 channel) {
        const auto pointer = wpad_service().pointer(channel);
        pPos->x = pointer.x;
        pPos->y = pointer.y;
    }

    s32 getCorePadEnablePastCount(s32 channel) {
        return static_cast<s32>(wpad_service().pointer_history_count(channel));
    }

    bool isCorePadPointInScreen(s32 channel) {
        return wpad_service().pointer(channel).valid;
    }

    bool testCorePadButtonUp(s32 channel) {
        return is_held(channel, WPAD_BUTTON_UP);
    }

    bool testCorePadButtonDown(s32 channel) {
        return is_held(channel, WPAD_BUTTON_DOWN);
    }

    bool testCorePadButtonLeft(s32 channel) {
        return is_held(channel, WPAD_BUTTON_LEFT);
    }

    bool testCorePadButtonRight(s32 channel) {
        return is_held(channel, WPAD_BUTTON_RIGHT);
    }

    bool testCorePadButtonA(s32 channel) {
        return is_held(channel, WPAD_BUTTON_A);
    }

    bool testCorePadButtonB(s32 channel) {
        return is_held(channel, WPAD_BUTTON_B);
    }

    bool testCorePadButton1(s32 channel) {
        return is_held(channel, WPAD_BUTTON_1);
    }

    bool testCorePadButton2(s32 channel) {
        return is_held(channel, WPAD_BUTTON_2);
    }

    bool testCorePadButtonPlus(s32 channel) {
        return is_held(channel, WPAD_BUTTON_PLUS);
    }

    bool testCorePadButtonMinus(s32 channel) {
        return is_held(channel, WPAD_BUTTON_MINUS);
    }

    bool testSubPadButtonC(s32 channel) {
        return is_held(channel, WPAD_BUTTON_C);
    }

    bool testSubPadButtonZ(s32 channel) {
        return is_held(channel, WPAD_BUTTON_Z);
    }

    bool testPadButtonAnyWithoutHome(s32 channel) {
        return is_held(channel, cAnyButtonWithoutHomeMask);
    }

    bool testCorePadTriggerUp(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_UP);
    }

    bool testCorePadTriggerDown(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_DOWN);
    }

    bool testCorePadTriggerLeft(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_LEFT);
    }

    bool testCorePadTriggerRight(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_RIGHT);
    }

    bool testCorePadTriggerA(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_A);
    }

    bool testCorePadTriggerB(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_B);
    }

    bool testCorePadTrigger1(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_1);
    }

    bool testCorePadTrigger2(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_2);
    }

    bool testCorePadTriggerPlus(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_PLUS);
    }

    bool testCorePadTriggerMinus(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_MINUS);
    }

    bool testCorePadTriggerAnyWithoutHome(s32 channel) {
        return is_triggered(channel, cCoreButtonMask);
    }

    bool testCorePadTriggerHome(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_HOME);
    }

    bool testSubPadTriggerC(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_C);
    }

    bool testSubPadTriggerZ(s32 channel) {
        return is_triggered(channel, WPAD_BUTTON_Z);
    }

    bool testSubPadReleaseZ(s32 channel) {
        return is_released(channel, WPAD_BUTTON_Z);
    }

    bool isCorePadSwing(s32 channel) {
        return wpad_service().is_core_swing(channel);
    }

    bool isCorePadSwingTrigger(s32 channel) {
        return wpad_service().is_core_swing_triggered(channel);
    }

    f32 getSubPadStickX(s32 channel) {
        return wpad_service().sub_stick(channel).x;
    }

    f32 getSubPadStickY(s32 channel) {
        return wpad_service().sub_stick(channel).y;
    }

    bool testSubPadStickTriggerUp(s32 channel) {
        return (wpad_service().sub_stick_trigger(channel) & aurora::WpadStickUp) != 0U;
    }

    bool testSubPadStickTriggerDown(s32 channel) {
        return (wpad_service().sub_stick_trigger(channel) & aurora::WpadStickDown) != 0U;
    }

    bool testSubPadStickTriggerLeft(s32 channel) {
        return (wpad_service().sub_stick_trigger(channel) & aurora::WpadStickLeft) != 0U;
    }

    bool testSubPadStickTriggerRight(s32 channel) {
        return (wpad_service().sub_stick_trigger(channel) & aurora::WpadStickRight) != 0U;
    }

    bool testSystemPadTriggerDecide() {
        return testCorePadTriggerA(WPAD_CHAN0) != false;
    }

    bool testSystemTriggerA() {
        return testCorePadTriggerA(WPAD_CHAN0) != false;
    }

    bool testSystemTriggerB() {
        return testCorePadTriggerB(WPAD_CHAN0) != false;
    }

    bool testDPDMenuPadDecideTrigger() {
        return testCorePadTriggerA(WPAD_CHAN0);
    }

    bool testFpViewStartTrigger() {
        return testCorePadTriggerUp(WPAD_CHAN0);
    }

    bool testFpViewOutTrigger() {
        return testCorePadTriggerDown(WPAD_CHAN0) || testCorePadTriggerA(WPAD_CHAN0);
    }

    bool getPlayerTriggerA() {
        return testCorePadTriggerA(WPAD_CHAN0);
    }

    bool getPlayerTriggerB() {
        return testCorePadTriggerB(WPAD_CHAN0);
    }

    bool getPlayerTriggerZ() {
        return testSubPadTriggerZ(WPAD_CHAN0);
    }

    bool getPlayerTriggerC() {
        return testSubPadTriggerC(WPAD_CHAN0);
    }

    bool getPlayerLevelA() {
        return testCorePadButtonA(WPAD_CHAN0);
    }

    bool getPlayerLevelB() {
        return testCorePadButtonB(WPAD_CHAN0);
    }

    bool getPlayerLevelZ() {
        return testSubPadButtonZ(WPAD_CHAN0);
    }

    bool getPlayerLevelC() {
        return testSubPadButtonC(WPAD_CHAN0);
    }

    bool isGamePadStickOperated(s32 channel) {
        const auto stick = wpad_service().sub_stick(channel);
        return std::abs(stick.x) + std::abs(stick.y) > 0.0F;
    }

    u32 getWPadMaxCount() {
        return 2;
    }

    bool isConnectedWPad(s32 channel) {
        return wpad_service().is_connected(channel);
    }
}  // namespace MR
