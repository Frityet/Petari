#include "Game/Util/DrawUtil.hpp"

#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "RendererService.hpp"
#include "compat/J3dSystemCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <dolphin/mtx.h>

namespace MR {
    void fillSilhouetteColor() {
        captureScreenAlpha(0);
        loadScreenAlphaTexture(0, GX_TEXMAP0);
    }

    void drawInitFor2DModel() {
        Mtx identity;
        PSMTXIdentity(identity);
        smgpc::compat::load_j3d_view_matrix(identity);

        auto* renderer = smgpc::render::try_current_aurora_renderer();
        if (renderer == nullptr) {
            return;
        }
        renderer->prepare_model_3d_for_2d({
            .screen_width = static_cast<f32>(MR::getScreenWidth()),
            .screen_height = static_cast<f32>(MR::getScreenHeight()),
        });
    }

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
