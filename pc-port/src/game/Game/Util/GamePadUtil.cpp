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

}  // namespace MR
