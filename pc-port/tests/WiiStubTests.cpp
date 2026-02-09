#include "platform/WiiStubs.hpp"
#include "tests/TestHarness.hpp"

namespace {

$pc_port_test(WiiStubsAreDeterministic) {
    const pcport::WiiStubStatus& status = pcport::WiiStubs::GetStatus();

    $pc_port_require_eq(status.miiCount, 0);
    $pc_port_require_eq(status.homeButtonEnabled, false);
    $pc_port_require_eq(status.nwc24Available, false);

    for (int i = 0; i < 16; ++i) {
        $pc_port_require_eq(pcport::WiiStubs::GetMiiCount(), 0);
        $pc_port_require_eq(pcport::WiiStubs::IsHomeButtonEnabled(), false);
        $pc_port_require_eq(pcport::WiiStubs::IsNwc24Available(), false);
        $pc_port_require_eq(pcport::WiiStubs::GetSystemTicks(), status.fixedTicks);
    }
}

}  // namespace
