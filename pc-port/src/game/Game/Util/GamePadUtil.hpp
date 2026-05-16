#pragma once

constexpr int WPAD_CHAN0 = 0;
constexpr int WPAD_CHAN1 = 1;
constexpr int WPAD_CHAN2 = 2;
constexpr int WPAD_CHAN3 = 3;

namespace MR {

[[nodiscard]] bool testCorePadButtonA(int channel);
[[nodiscard]] bool testCorePadButtonB(int channel);
[[nodiscard]] bool testCorePadTriggerA(int channel);
[[nodiscard]] bool testCorePadTriggerUp(int channel);
[[nodiscard]] bool testCorePadTriggerDown(int channel);
[[nodiscard]] bool testCorePadTriggerLeft(int channel);
[[nodiscard]] bool testCorePadTriggerRight(int channel);
[[nodiscard]] bool testSubPadStickTriggerUp(int channel);
[[nodiscard]] bool testSubPadStickTriggerDown(int channel);
[[nodiscard]] bool testSubPadStickTriggerLeft(int channel);
[[nodiscard]] bool testSubPadStickTriggerRight(int channel);
[[nodiscard]] bool testSystemPadTriggerDecide();
[[nodiscard]] bool testSystemTriggerB();
[[nodiscard]] bool testDPDMenuPadDecideTrigger();

}  // namespace MR
