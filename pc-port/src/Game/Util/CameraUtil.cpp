#include "Game/Util/CameraUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void resetCameraMan() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector reset CameraMan for title mode");
        }
    }
}  // namespace MR
