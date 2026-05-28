#pragma once

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "scene/StageHostService.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <revolution.h>

class NameObj;
class Scene;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    class StageHostScene;

    class SceneLifecycleService final {
    public:
        explicit SceneLifecycleService(smgpc::runtime::RuntimeContext &runtime);
        ~SceneLifecycleService();

        SceneLifecycleService(const SceneLifecycleService &) = delete;
        SceneLifecycleService &operator=(const SceneLifecycleService &) = delete;

        void request_stage(const StageHostRequest &request);
        void destroy_scene();

        void start_scene();
        void update_scene();
        void calc_anim_scene();
        void draw_3d_normal(render::IRendererEngine &renderer, const smgpc::camera::CameraPose &camera_pose);
        void draw_2d_normal(render::IRendererEngine &renderer);

        [[nodiscard]] Scene *active_scene() const;
        [[nodiscard]] NameObj *active_root() const;
        [[nodiscard]] bool has_active_stage(std::string_view stage_name) const;
        [[nodiscard]] std::string_view active_scene_name() const;
        [[nodiscard]] std::string_view active_stage_name() const;
        [[nodiscard]] s32 active_scenario_no() const;

    private:
        void create_stage_scene(const StageHostRequest &request);

        smgpc::runtime::RuntimeContext &_runtime;
        std::string _active_scene_name;
        std::string _active_stage_name;
        s32 _active_scenario_no = 0;
        std::unique_ptr<StageHostScene> _active_scene;
    };

}  // namespace smgpc::scene
