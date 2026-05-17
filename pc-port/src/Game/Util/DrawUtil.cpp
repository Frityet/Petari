#include "Game/Util/DrawUtil.hpp"

#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    void fillSilhouetteColor() {
        captureScreenAlpha(0);
        loadScreenAlphaTexture(0, GX_TEXMAP0);
    }

    void activateGameSceneDraw3D() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->game_layout().activate_game_scene_draw_3d();
        }
    }

    void deactivateGameSceneDraw3D() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->game_layout().deactivate_game_scene_draw_3d();
        }
    }
}  // namespace MR
