#pragma once

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {


    class SceneExecutionService final {
    public:
        explicit SceneExecutionService(smgpc::runtime::RuntimeContext &runtime);
        ~SceneExecutionService();

        SceneExecutionService(const SceneExecutionService &) = delete;
        SceneExecutionService &operator=(const SceneExecutionService &) = delete;

        void execute_movement();
        void execute_calc_anim_and_view();
        void draw_3d_normal(render::IRendererEngine &renderer, const smgpc::camera::CameraPose &camera_pose);
        void draw_2d_normal(render::IRendererEngine &renderer);

    private:
        smgpc::runtime::RuntimeContext &_runtime;
    };

}  // namespace smgpc::scene
