#include "Game/Util/ScreenUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void deactivateDefaultGameLayout() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector deactivated the default game layout");
        }
    }
}  // namespace MR
