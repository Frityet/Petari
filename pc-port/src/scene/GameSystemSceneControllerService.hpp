#pragma once

#include "scene/StageHostService.hpp"

#include <optional>
#include <string>
#include <string_view>

#include <revolution.h>

namespace smgpc::compat {

    class RuntimeContext;
    class SceneLifecycleService;

    enum class SceneControllerPhase {
        NotInitialized,
        WaitingForRequest,
        RequestQueued,
        DestroyScene,
        InitializeScene,
        ReadyToStartScene,
        Normal,
    };

    struct SceneControlInfoCompat {
        std::string scene_name;
        std::string stage_name;
        s32 scenario_no = 1;
        s32 selected_scenario_no = 1;
        std::string object_name;
        std::string actor_name;
        bool appear_after_init = false;
    };

    class GameSystemSceneControllerService final {
    public:
        GameSystemSceneControllerService(RuntimeContext &runtime, SceneLifecycleService &scene_lifecycle);
        ~GameSystemSceneControllerService();

        GameSystemSceneControllerService(const GameSystemSceneControllerService &) = delete;
        GameSystemSceneControllerService &operator=(const GameSystemSceneControllerService &) = delete;

        void request_change_scene(const StageHostRequest &request);
        void check_request_and_change_scene();
        void destroy_scene();

        [[nodiscard]] bool has_pending_request() const;
        [[nodiscard]] bool has_active_stage(std::string_view stage_name) const;
        [[nodiscard]] std::string_view active_scene_name() const;
        [[nodiscard]] std::string_view active_stage_name() const;
        [[nodiscard]] s32 active_scenario_no() const;
        [[nodiscard]] SceneControllerPhase phase() const;
        [[nodiscard]] const std::optional<SceneControlInfoCompat> &pending_scene() const;

    private:
        void apply_pending_scene();
        void set_phase(SceneControllerPhase phase);
        [[nodiscard]] StageHostRequest pending_request() const;

        RuntimeContext &_runtime;
        SceneLifecycleService &_scene_lifecycle;
        SceneControllerPhase _phase = SceneControllerPhase::NotInitialized;
        std::optional<SceneControlInfoCompat> _pending_scene;
    };

}  // namespace smgpc::compat
