#include "scene/SceneTransitionRequestService.hpp"

#include "Game/System/GalaxyMoveArgument.hpp"
#include "Game/System/StorySequenceExecutor.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "compat/StorySequencePlatformCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace {
    [[nodiscard]] smgpc::scene::StageHostRequest stage_request_from_story_move(const GalaxyMoveArgument &move) {
        if (move.mStageName == nullptr || move.mStageName[0] == '\0' || move.mScenarioNo < 1) {
            throw std::runtime_error("StorySequenceExecutor did not produce a concrete stage request");
        }

        return smgpc::scene::StageHostRequest{
            .scene_name = "Game",
            .stage_name = move.mStageName,
            .scenario_no = move.mScenarioNo,
            .start_id = move.mIDInfo._0,
            .start_zone_id = move.mIDInfo.mZoneID,
        };
    }
}  // namespace

namespace smgpc::scene {

    SceneTransitionRequestService::SceneTransitionRequestService(smgpc::runtime::RuntimeContext &runtime)
        : _runtime(runtime),
          _story_sequence(std::make_unique<StorySequenceExecutor>()),
          _initial_stage_request(execute_initial_story_move()) {}

    SceneTransitionRequestService::~SceneTransitionRequestService() = default;

    const StageHostRequest &SceneTransitionRequestService::initial_stage_request() const {
        return _initial_stage_request;
    }

    void SceneTransitionRequestService::update() {
        const auto scene_state = smgpc::compat::story_sequence::SceneStateBinding(
            _runtime.current_sequence_scene_name(), _runtime.current_stage_name(), _active_story_scenario_no);
        _story_sequence->update();

        if (_runtime.sequence_requests().consume_change_stage_in_game_after_loading_game_data_request()) {
            auto request = execute_after_loading_story_move();
            _story_scene_start_stage = request.stage_name;
            request_transition(request, "after_loading_game_data");
        }
    }

    void SceneTransitionRequestService::notify_scene_started(std::string_view stage_name, s32 scenario_no) {
        if (!_story_scene_start_stage.has_value() || stage_name != *_story_scene_start_stage) {
            return;
        }

        _active_story_scenario_no = scenario_no;
        const auto scene_state = smgpc::compat::story_sequence::SceneStateBinding(
            _runtime.current_sequence_scene_name(), stage_name, scenario_no);
        _story_sequence->setNerveSceneStart();
        _story_scene_start_stage.reset();
    }

    std::optional<StageHostRequest> SceneTransitionRequestService::take_pending_request() {
        auto request = std::move(_pending_request);
        _pending_request.reset();
        return request;
    }

    StageHostRequest SceneTransitionRequestService::execute_initial_story_move() {
        const auto start_info = JMapIdInfo(0, 0);
        auto move = GalaxyMoveArgument(7, nullptr, 1, &start_info);
        const auto scene_state = smgpc::compat::story_sequence::SceneStateBinding(
            _runtime.current_sequence_scene_name(), _runtime.current_stage_name(), _active_story_scenario_no);
        _story_sequence->moveGalaxy(&move, true);
        return stage_request_from_story_move(move);
    }

    StageHostRequest SceneTransitionRequestService::execute_after_loading_story_move() {
        const auto start_info = JMapIdInfo(0, 0);
        auto move = GalaxyMoveArgument(6, nullptr, 1, &start_info);
        const auto scene_state = smgpc::compat::story_sequence::SceneStateBinding(
            _runtime.current_sequence_scene_name(), _runtime.current_stage_name(), _active_story_scenario_no);
        _story_sequence->moveGalaxy(&move, true);
        return stage_request_from_story_move(move);
    }

    void SceneTransitionRequestService::request_transition(const StageHostRequest &request, std::string_view source) {
        if (_pending_request.has_value()) {
            return;
        }

        _runtime.set_current_sequence_scene_name(request.scene_name);
        _runtime.set_next_sequence_scene_name(request.stage_name);
        _runtime.set_current_stage_name(request.stage_name);
#ifndef NDEBUG
        const auto detail = "source=" + std::string(source) + ";scene=" + request.scene_name + ";stage=" + request.stage_name +
                            ";scenario=" + std::to_string(request.scenario_no) + ";start_id=" + std::to_string(request.start_id) +
                            ";start_zone_id=" + std::to_string(request.start_zone_id);
        _runtime.emit_sequence_state_trace_event("stage_requested", detail);
        _runtime.emit_semantic_trace_event("scene_transition", "stage_requested", detail);
#else
        static_cast<void>(source);
#endif
        _pending_request = request;
    }

}  // namespace smgpc::scene
