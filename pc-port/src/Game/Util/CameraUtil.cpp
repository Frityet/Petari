#include "Game/Util/CameraUtil.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void resetCameraMan() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().reset_camera_man();
        }
    }

    void pauseOnCameraDirector() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().pause_on_camera_director();
        }
    }

    void pauseOffCameraDirector() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->camera_system().pause_off_camera_director();
        }
    }
}  // namespace MR
