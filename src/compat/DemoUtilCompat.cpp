#include "Game/Player/MarioAccess.hpp"
#include "compat/DemoUtilCompat.hpp"
#include "compat/StageSessionState.hpp"

namespace smgpc::compat {
    void release_puppetable_demo_control(bool) {
        MarioAccess::endRemoteDemo(nullptr);
    }
}  // namespace smgpc::compat

namespace MR {
    bool isPowerStarGetDemoActive() {
        return smgpc::compat::require_active_stage_session().is_power_star_get_demo_active();
    }
}  // namespace MR
