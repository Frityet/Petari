#include "scene/SceneExecutionService.hpp"

#include "Game/Scene/SceneFunction.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"

namespace smgpc::scene {

    SceneExecutionService::SceneExecutionService(smgpc::runtime::RuntimeContext &runtime) : _runtime(runtime) {
    }

    SceneExecutionService::~SceneExecutionService() = default;

    void SceneExecutionService::execute_movement() {
        _runtime.scheduler().execute_movement();
    }

    void SceneExecutionService::execute_calc_anim_and_view() {
        auto &scheduler = _runtime.scheduler();
        scheduler.execute_calc_anim();
        scheduler.execute_calc_view_and_entry();
    }

    void SceneExecutionService::draw_3d_normal(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose) {
        auto &scheduler = _runtime.scheduler();
        scheduler.execute_draw_buffer_list_normal(renderer, camera_pose);
        scheduler.execute_draw_type(renderer, MR::DrawType_EffectDraw3D);
        scheduler.execute_draw_type(renderer, MR::DrawType_EffectDrawForBloomEffect);
        scheduler.execute_draw_type(renderer, MR::DrawType_CaptureScreenIndirect);
    }

    void SceneExecutionService::draw_2d_normal(render::AuroraRenderer &renderer) {
        _runtime.scheduler().execute_draw_list_2d_normal(renderer);
    }

}  // namespace smgpc::scene
