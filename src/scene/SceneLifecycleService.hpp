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
class GameScene;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::compat {
    class JkrAllocationDomain;
}

namespace smgpc::scene {

    class GameSceneBinding;
    class StageInitializationService;

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
        void draw_scene();

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
        // Destruction order: original Scene (and base/delete), then native
        // initialization services, then the retained original Game heap.
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> _active_scene_domain;
        std::unique_ptr<StageInitializationService> _active_initialization;
        std::unique_ptr<GameSceneBinding> _active_game_scene_binding;
        std::unique_ptr<GameScene> _active_scene;
    };

}  // namespace smgpc::scene
