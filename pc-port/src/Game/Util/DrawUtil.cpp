#include "Game/Util/DrawUtil.hpp"

#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "runtime/RuntimeContext.hpp"

namespace MR {
    void activateGameSceneDraw3D() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->game_layout().activate_game_scene_draw_3d();
        }
    }

    void deactivateGameSceneDraw3D() {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->game_layout().deactivate_game_scene_draw_3d();
        }
    }
}  // namespace MR
