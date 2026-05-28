#include "scene/StorySequenceService.hpp"

#include "Game/System/StorySequenceExecutor.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>

namespace smgpc::scene {

    StorySequenceService::StorySequenceService(smgpc::runtime::RuntimeContext &runtime) : _runtime(runtime) {
    }

    StorySequenceService::~StorySequenceService() = default;

    void StorySequenceService::update_after_loading_request() {
        if (_after_loading_request_consumed || !_runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested()) {
            return;
        }

        _after_loading_request_consumed = true;
        const auto route = smgpc::game::story_sequence_executor().takePendingStageRequest();
        if (!route.has_value()) {
            return;
        }

        _runtime.set_current_sequence_scene_name(route->mSceneName);
        _runtime.set_next_sequence_scene_name(route->mStageName);
        _runtime.set_current_stage_name(route->mStageName);
#ifndef NDEBUG
        const auto detail = "event=" + route->mEventName + ";demo=" + route->mDemoName + ";stage=" + route->mStageName +
                            ";scenario=" + std::to_string(route->mScenarioNo);
        _runtime.emit_sequence_state_trace_event("story_stage_prepared", detail);
        _runtime.emit_semantic_trace_event("story", "story_stage_prepared", detail);
#endif
        _pending_stage_request = StageHostRequest {
            .scene_name = route->mSceneName,
            .stage_name = route->mStageName,
            .object_name = route->mObjectName,
            .actor_name = route->mActorName,
            .scenario_no = route->mScenarioNo,
            .appear_after_init = route->mAppearAfterInit,
        };
    }

    std::optional<StageHostRequest> StorySequenceService::take_pending_stage_request() {
        auto request = std::move(_pending_stage_request);
        _pending_stage_request.reset();
        return request;
    }

}  // namespace smgpc::scene
