#pragma once

#include "scene/StageHostService.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

class StorySequenceExecutor;

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    class SceneTransitionRequestService final {
    public:
        explicit SceneTransitionRequestService(smgpc::runtime::RuntimeContext &runtime);
        ~SceneTransitionRequestService();

        SceneTransitionRequestService(const SceneTransitionRequestService &) = delete;
        SceneTransitionRequestService &operator=(const SceneTransitionRequestService &) = delete;

        [[nodiscard]] const StageHostRequest &initial_stage_request() const;
        void update();
        void notify_scene_started(std::string_view stage_name, s32 scenario_no);
        [[nodiscard]] std::optional<StageHostRequest> take_pending_request();

    private:
        [[nodiscard]] StageHostRequest execute_initial_story_move();
        [[nodiscard]] StageHostRequest execute_after_loading_story_move();
        void request_transition(const StageHostRequest &request, std::string_view source);

        smgpc::runtime::RuntimeContext &_runtime;
        std::unique_ptr<StorySequenceExecutor> _story_sequence;
        StageHostRequest _initial_stage_request;
        s32 _active_story_scenario_no = 0;
        std::optional<std::string> _story_scene_start_stage;
        std::optional<StageHostRequest> _pending_request;
    };

}  // namespace smgpc::scene
