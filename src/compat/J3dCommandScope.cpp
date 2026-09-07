#include "J3dCommandScope.hpp"

namespace smgpc::compat {
    J3dCommandScope::J3dCommandScope() : _interrupts(OSDisableInterrupts()) {
        OSDisableScheduler();
        OSRestoreInterrupts(_interrupts);
        _previous = __GDCurrentDL;
    }

    J3dCommandScope::~J3dCommandScope() {
        GDSetCurrent(_previous);
        OSRestoreInterrupts(_interrupts);
        OSEnableScheduler();
    }
}
