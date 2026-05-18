#include "Game/compat/StorySequenceService.hpp"

#include "Game/compat/RuntimeContext.hpp"

namespace smgpc::game {
    namespace {

        constexpr auto cPrologueDemoName = std::string_view{"Prologue"};
        constexpr auto cPrologueEventName = std::string_view{"cDemoPrologue"};
        constexpr auto cPrologueStageName = std::string_view{"PeachCastleGardenGalaxy"};

    }  // namespace

    StorySequenceService::StorySequenceService(RuntimeContext &runtime) : _runtime(runtime) {
    }

    StorySequenceService::~StorySequenceService() = default;

    void StorySequenceService::update_after_loading_request() {
        if (_after_loading_request_consumed || !_runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested()) {
            return;
        }

        _after_loading_request_consumed = true;
        prepare_prologue_after_loading();
    }

    std::optional<StageHostRequest> StorySequenceService::take_pending_stage_request() {
        auto request = std::move(_pending_stage_request);
        _pending_stage_request.reset();
        return request;
    }

    bool StorySequenceService::is_story_demo_active(std::string_view demo_name) const {
        return _active_demo_name == demo_name;
    }

    std::string_view StorySequenceService::active_demo_name() const {
        return _active_demo_name;
    }

    std::string_view StorySequenceService::active_event_name() const {
        return _active_event_name;
    }

    void StorySequenceService::prepare_prologue_after_loading() {
        _active_demo_name = cPrologueDemoName;
        _active_event_name = cPrologueEventName;
        _runtime.set_current_sequence_scene_name("Game");
        _runtime.set_next_sequence_scene_name(cPrologueStageName);
        _runtime.set_current_stage_name(cPrologueStageName);
#ifndef NDEBUG
        _runtime.emit_sequence_state_trace_event("story_demo_prepared",
                                                 "event=cDemoPrologue;demo=Prologue;stage=PeachCastleGardenGalaxy;scenario=1");
        _runtime.emit_semantic_trace_event("story", "story_demo_prepared",
                                           "event=cDemoPrologue;demo=Prologue;stage=PeachCastleGardenGalaxy;scenario=1");
#endif
        _pending_stage_request = StageHostRequest{
            .scene_name = "Game",
            .stage_name = std::string(cPrologueStageName),
            .object_name = "PrologueDirector",
            .actor_name = {},
            .scenario_no = 1,
            .appear_after_init = true,
        };
    }

}  // namespace smgpc::game
