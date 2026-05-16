#include "Game/Util/StarPointerUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void startStarPointerModeTitle(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector started title star-pointer mode");
        }
    }
}  // namespace MR
