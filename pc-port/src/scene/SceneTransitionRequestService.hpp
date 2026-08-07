#pragma once

#include "scene/StageHostService.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    enum class SceneTransitionTriggerKind {
        NameObjDeadAfterAlive,
    };

    struct SceneTransitionTrigger {
        SceneTransitionTriggerKind kind = SceneTransitionTriggerKind::NameObjDeadAfterAlive;
        std::string name;
    };

    struct ConfiguredSceneTransition {
        SceneTransitionTrigger trigger;
        StageHostRequest request;
    };

    class SceneTransitionTriggerTracker final {
    public:
        explicit SceneTransitionTriggerTracker(SceneTransitionTrigger trigger);

        [[nodiscard]] bool observe_name_obj(std::string_view name, bool dead);
        [[nodiscard]] bool has_fired() const;

    private:
        SceneTransitionTrigger _trigger;
        bool _seen_alive = false;
        bool _fired = false;
    };

    class SceneTransitionRequestService final {
    public:
        explicit SceneTransitionRequestService(smgpc::runtime::RuntimeContext &runtime);
        ~SceneTransitionRequestService();

        SceneTransitionRequestService(const SceneTransitionRequestService &) = delete;
        SceneTransitionRequestService &operator=(const SceneTransitionRequestService &) = delete;

        [[nodiscard]] static StageHostRequest make_initial_stage_request();
        [[nodiscard]] const StageHostRequest &initial_stage_request() const;
        void update();
        [[nodiscard]] std::optional<StageHostRequest> take_pending_request();

#ifndef NDEBUG
        [[nodiscard]] static std::optional<ConfiguredSceneTransition> configured_transition_from_environment();
#endif

    private:
        void request_transition(const StageHostRequest &request, std::string_view source);

        smgpc::runtime::RuntimeContext &_runtime;
        StageHostRequest _initial_stage_request;
        StageHostRequest _after_loading_request;
        std::optional<StageHostRequest> _debug_key_request;
        std::optional<StageHostRequest> _pending_request;
#ifndef NDEBUG
        std::optional<ConfiguredSceneTransition> _configured_transition;
        std::optional<SceneTransitionTriggerTracker> _configured_trigger;
#endif
    };

}  // namespace smgpc::scene
