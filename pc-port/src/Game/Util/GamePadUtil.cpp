#include "Game/Util/GamePadUtil.hpp"

#include "runtime/RuntimeContext.hpp"

namespace MR {
    namespace {

        constexpr auto ANY_NON_HOME_BUTTON_MASK = static_cast< u32 >(KPAD_BUTTON_MASK & ~WPAD_BUTTON_HOME);
        constexpr auto STICK_TRIGGER_THRESHOLD = 0.5F;

        [[nodiscard]] const smgpc::runtime::WpadService* wpad_service() {
            if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
                return &runtime->wpad();
            }

            return nullptr;
        }

        [[nodiscard]] bool held(s32 channel, u32 mask) {
            const auto* wpad = wpad_service();
            return wpad != nullptr && wpad->is_button_held(channel, mask);
        }

        [[nodiscard]] bool triggered(s32 channel, u32 mask) {
            const auto* wpad = wpad_service();
            return wpad != nullptr && wpad->is_button_triggered(channel, mask);
        }

        [[nodiscard]] bool released(s32 channel, u32 mask) {
            const auto* wpad = wpad_service();
            return wpad != nullptr && wpad->is_button_released(channel, mask);
        }

        void set_vec2(TVec2f* pPos, const smgpc::runtime::WpadPointerState& pointer) {
            if (pPos == nullptr) {
                return;
            }

            pPos->x = pointer.x;
            pPos->y = pointer.y;
        }

        void set_vec3(TVec3f* pVec, const smgpc::runtime::WpadVec3State& value) {
            if (pVec == nullptr) {
                return;
            }

            pVec->x = value.x;
            pVec->y = value.y;
            pVec->z = value.z;
        }

    }  // namespace

    void getCorePadPointingPosBasedOnScreen(TVec2f* pPos, s32 channel) {
        getCorePadPointingPos(pPos, channel);
    }

    void getCorePadPointingPos(TVec2f* pPos, s32 channel) {
        const auto* wpad = wpad_service();
        set_vec2(pPos, wpad == nullptr ? smgpc::runtime::WpadPointerState{} : wpad->pointer(channel));
    }

    void getCorePadPastPointingPos(TVec2f* pPos, s32 idx, s32 channel) {
        const auto* wpad = wpad_service();
        set_vec2(pPos, wpad == nullptr || idx < 0 ? smgpc::runtime::WpadPointerState{} : wpad->past_pointer(channel, static_cast< u32 >(idx)));
    }

    s32 getCorePadEnablePastCount(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad == nullptr ? 0 : static_cast< s32 >(wpad->pointer_history_count(channel));
    }

    bool isCorePadPointInScreen(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad != nullptr && wpad->pointer(channel).valid;
    }

    f32 getCorePadDistanceToDisplay(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad == nullptr ? 0.0F : wpad->distance_to_display(channel);
    }

    void getCorePadAcceleration(TVec3f* pAccel, s32 channel) {
        const auto* wpad = wpad_service();
        set_vec3(pAccel, wpad == nullptr ? smgpc::runtime::WpadVec3State{} : wpad->core_acceleration(channel));
    }

    bool testCorePadButtonUp(s32 channel) {
        return held(channel, WPAD_BUTTON_UP);
    }

    bool testCorePadButtonDown(s32 channel) {
        return held(channel, WPAD_BUTTON_DOWN);
    }

    bool testCorePadButtonLeft(s32 channel) {
        return held(channel, WPAD_BUTTON_LEFT);
    }

    bool testCorePadButtonRight(s32 channel) {
        return held(channel, WPAD_BUTTON_RIGHT);
    }

    bool testCorePadButtonA(s32 channel) {
        return held(channel, WPAD_BUTTON_A);
    }

    bool testCorePadButtonB(s32 channel) {
        return held(channel, WPAD_BUTTON_B);
    }

    bool testCorePadButtonPlus(s32 channel) {
        return held(channel, WPAD_BUTTON_PLUS);
    }

    bool testCorePadButtonMinus(s32 channel) {
        return held(channel, WPAD_BUTTON_MINUS);
    }

    bool testSubPadButtonC(s32 channel) {
        return held(channel, WPAD_BUTTON_C);
    }

    bool testSubPadButtonZ(s32 channel) {
        return held(channel, WPAD_BUTTON_Z);
    }

    bool testPadButtonAnyWithoutHome(s32 channel) {
        return held(channel, ANY_NON_HOME_BUTTON_MASK);
    }

    bool testCorePadTriggerUp(s32 channel) {
        return triggered(channel, WPAD_BUTTON_UP);
    }

    bool testCorePadTriggerDown(s32 channel) {
        return triggered(channel, WPAD_BUTTON_DOWN);
    }

    bool testCorePadTriggerLeft(s32 channel) {
        return triggered(channel, WPAD_BUTTON_LEFT);
    }

    bool testCorePadTriggerRight(s32 channel) {
        return triggered(channel, WPAD_BUTTON_RIGHT);
    }

    bool testCorePadTriggerA(s32 channel) {
        return triggered(channel, WPAD_BUTTON_A);
    }

    bool testCorePadTriggerB(s32 channel) {
        return triggered(channel, WPAD_BUTTON_B);
    }

    bool testCorePadTriggerPlus(s32 channel) {
        return triggered(channel, WPAD_BUTTON_PLUS);
    }

    bool testCorePadTriggerMinus(s32 channel) {
        return triggered(channel, WPAD_BUTTON_MINUS);
    }

    bool testCorePadTriggerAnyWithoutHome(s32 channel) {
        return triggered(channel, ANY_NON_HOME_BUTTON_MASK);
    }

    bool testCorePadTriggerHome(s32 channel) {
        return triggered(channel, WPAD_BUTTON_HOME);
    }

    bool testSubPadTriggerC(s32 channel) {
        return triggered(channel, WPAD_BUTTON_C);
    }

    bool testSubPadTriggerZ(s32 channel) {
        return triggered(channel, WPAD_BUTTON_Z);
    }

    bool testSubPadReleaseZ(s32 channel) {
        return released(channel, WPAD_BUTTON_Z);
    }

    bool isCorePadSwing(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad != nullptr && wpad->is_core_swing(channel);
    }

    bool isCorePadSwingTrigger(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad != nullptr && wpad->is_core_swing_triggered(channel);
    }

    f32 getSubPadStickX(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad == nullptr ? 0.0F : wpad->sub_stick(channel).x;
    }

    f32 getSubPadStickY(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad == nullptr ? 0.0F : wpad->sub_stick(channel).y;
    }

    bool testSubPadStickTriggerUp(s32 channel) {
        return getSubPadStickY(channel) > STICK_TRIGGER_THRESHOLD;
    }

    bool testSubPadStickTriggerDown(s32 channel) {
        return getSubPadStickY(channel) < -STICK_TRIGGER_THRESHOLD;
    }

    bool testSubPadStickTriggerLeft(s32 channel) {
        return getSubPadStickX(channel) < -STICK_TRIGGER_THRESHOLD;
    }

    bool testSubPadStickTriggerRight(s32 channel) {
        return getSubPadStickX(channel) > STICK_TRIGGER_THRESHOLD;
    }

    void getSubPadAcceleration(TVec3f* pAccel, s32 channel) {
        const auto* wpad = wpad_service();
        set_vec3(pAccel, wpad == nullptr ? smgpc::runtime::WpadVec3State{} : wpad->sub_acceleration(channel));
    }

    bool isSubPadSwing(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad != nullptr && wpad->is_sub_swing(channel);
    }

    bool isPadSwing(s32 channel) {
        return isCorePadSwing(channel) || isSubPadSwing(channel);
    }

    bool testSystemPadTriggerDecide() {
        return testCorePadTriggerA(WPAD_CHAN0);
    }

    bool testSystemTriggerA() {
        return testCorePadTriggerA(WPAD_CHAN0);
    }

    bool testSystemTriggerB() {
        return testCorePadTriggerB(WPAD_CHAN0);
    }

    bool testDPDMenuPadDecideTrigger() {
        return testCorePadTriggerA(WPAD_CHAN0);
    }

    bool testFpViewStartTrigger() {
        return testCorePadTriggerUp(WPAD_CHAN0);
    }

    bool testFpViewOutTrigger() {
        return testCorePadTriggerDown(WPAD_CHAN0) || testCorePadTriggerB(WPAD_CHAN0);
    }

    f32 getPlayerStickX() {
        return getSubPadStickX(WPAD_CHAN0);
    }

    f32 getPlayerStickY() {
        return getSubPadStickY(WPAD_CHAN0);
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
        return getSubPadStickX(channel) != 0.0F || getSubPadStickY(channel) != 0.0F;
    }

    void calcWorldStickDirectionXZ(f32* pDirX, f32* pDirZ, s32 channel) {
        if (pDirX != nullptr) {
            *pDirX = getSubPadStickX(channel);
        }
        if (pDirZ != nullptr) {
            *pDirZ = getSubPadStickY(channel);
        }
    }

    void calcWorldStickDirectionXZ(TVec3f* pDir, s32 channel) {
        if (pDir == nullptr) {
            return;
        }

        pDir->x = getSubPadStickX(channel);
        pDir->y = 0.0F;
        pDir->z = getSubPadStickY(channel);
    }

    u32 getWPadMaxCount() {
        return static_cast< u32 >(WPAD_MAX_CONTROLLERS);
    }

    bool isConnectedWPad(s32 channel) {
        const auto* wpad = wpad_service();
        return wpad != nullptr && wpad->is_connected(channel);
    }

    bool isOperatingWPad(s32 channel) {
        return isConnectedWPad(channel);
    }

}  // namespace MR

namespace WPadFunction {

    WPadRumble* getWPadRumble(s32) {
        return nullptr;
    }

}  // namespace WPadFunction
