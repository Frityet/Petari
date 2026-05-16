#include "Game/Util/GamePadUtil.hpp"

#include "compat/DecompIntegration.hpp"
#include "compat/GamePadCompat.hpp"

namespace {

// SMGPC_INTEGRATION_BEGIN
SMGPC_STUB(src/Game/System/WPadAcceleration.cpp);
SMGPC_STUB(src/Game/System/WPadHVSwing.cpp);
// SMGPC_INTEGRATION_END

}  // namespace

namespace MR {

bool testCorePadButtonA(int channel) {
    (void)channel;
    return smgpc::game::compat::test_core_pad_button_a();
}

bool testCorePadButtonB(int channel) {
    (void)channel;
    return smgpc::game::compat::test_core_pad_button_b();
}

bool testCorePadTriggerA(int channel) {
    (void)channel;
    return smgpc::game::compat::test_core_pad_trigger_a();
}

bool testCorePadTriggerUp(int channel) {
    (void)channel;
    return smgpc::game::compat::test_core_pad_trigger_up();
}

bool testCorePadTriggerDown(int channel) {
    (void)channel;
    return smgpc::game::compat::test_core_pad_trigger_down();
}

bool testCorePadTriggerLeft(int channel) {
    (void)channel;
    return smgpc::game::compat::test_core_pad_trigger_left();
}

bool testCorePadTriggerRight(int channel) {
    (void)channel;
    return smgpc::game::compat::test_core_pad_trigger_right();
}

bool testSubPadStickTriggerUp(int channel) {
    return testCorePadTriggerUp(channel);
}

bool testSubPadStickTriggerDown(int channel) {
    return testCorePadTriggerDown(channel);
}

bool testSubPadStickTriggerLeft(int channel) {
    return testCorePadTriggerLeft(channel);
}

bool testSubPadStickTriggerRight(int channel) {
    return testCorePadTriggerRight(channel);
}

bool testSystemPadTriggerDecide() {
    return testCorePadTriggerA(WPAD_CHAN0);
}

bool testSystemTriggerB() {
    return smgpc::game::compat::test_system_trigger_b();
}

bool testDPDMenuPadDecideTrigger() {
    return testCorePadTriggerA(WPAD_CHAN0);
}

}  // namespace MR
