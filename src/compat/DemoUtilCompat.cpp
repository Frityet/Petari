#include "Game/Player/MarioAccess.hpp"
#include "compat/DemoUtilCompat.hpp"

namespace smgpc::compat {
    void release_puppetable_demo_control(bool) {
        MarioAccess::endRemoteDemo(nullptr);
    }
}  // namespace smgpc::compat
