#include "scene/SceneTransitionRequestService.hpp"

#include "Game/System/GameDataHolder.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"
#include "runtime/RuntimeContext.hpp"

#include <charconv>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
    constexpr auto cTriggerPrefix = std::string_view{"name_obj_dead_after_alive:"};

    [[nodiscard]] bool is_current_data_mario() {
        auto *file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        return file != nullptr && file->mGameDataHolder != nullptr && file->mGameDataHolder->isDataMario();
    }

#ifndef NDEBUG
    [[nodiscard]] std::optional<std::string> environment_string(const char *name) {
        const auto *value = std::getenv(name);
        if (value == nullptr || value[0] == '\0') {
            return std::nullopt;
        }
        return std::string(value);
    }

    [[nodiscard]] s32 parse_s32_environment(const char *name, s32 default_value) {
        const auto value = environment_string(name);
        if (!value.has_value()) {
            return default_value;
        }

        auto parsed = s32{};
        const auto *begin = value->data();
        const auto *end = begin + value->size();
        const auto result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc{} || result.ptr != end) {
            throw std::runtime_error(std::string("Invalid signed integer in ") + name + ": " + *value);
        }
        return parsed;
    }

    [[nodiscard]] bool parse_bool_environment(const char *name, bool default_value) {
        const auto value = environment_string(name);
        if (!value.has_value()) {
            return default_value;
        }
        if (*value == "1" || *value == "true") {
            return true;
        }
        if (*value == "0" || *value == "false") {
            return false;
        }
        throw std::runtime_error(std::string("Invalid boolean in ") + name + ": " + *value);
    }

    [[nodiscard]] smgpc::scene::StageHostRequest configured_target_from_environment() {
        const auto stage_name = environment_string("SMGPC_SCENE_TRANSITION_STAGE");
        if (!stage_name.has_value()) {
            throw std::runtime_error("SMGPC_SCENE_TRANSITION_TRIGGER requires SMGPC_SCENE_TRANSITION_STAGE");
        }

        return smgpc::scene::StageHostRequest{
            .scene_name = environment_string("SMGPC_SCENE_TRANSITION_SCENE").value_or("Game"),
            .stage_name = *stage_name,
            .object_name = environment_string("SMGPC_SCENE_TRANSITION_OBJECT").value_or(""),
            .actor_name = environment_string("SMGPC_SCENE_TRANSITION_ACTOR").value_or(""),
            .scenario_no = parse_s32_environment("SMGPC_SCENE_TRANSITION_SCENARIO", 1),
            .start_id = parse_s32_environment("SMGPC_SCENE_TRANSITION_START_ID", 0),
            .start_zone_id = parse_s32_environment("SMGPC_SCENE_TRANSITION_START_ZONE_ID", 0),
            .appear_after_init = parse_bool_environment("SMGPC_SCENE_TRANSITION_APPEAR_AFTER_INIT", false),
            .fail_unsupported_placement = parse_bool_environment("SMGPC_SCENE_TRANSITION_FAIL_UNSUPPORTED_PLACEMENT", false),
        };
    }
#endif
}  // namespace

namespace smgpc::scene {

    SceneTransitionTriggerTracker::SceneTransitionTriggerTracker(SceneTransitionTrigger trigger) : _trigger(std::move(trigger)) {
        if (_trigger.name.empty()) {
            throw std::invalid_argument("Scene transition trigger name must not be empty");
        }
    }

    bool SceneTransitionTriggerTracker::observe_name_obj(std::string_view name, bool dead) {
        if (_fired || name != _trigger.name) {
            return false;
        }

        switch (_trigger.kind) {
        case SceneTransitionTriggerKind::NameObjDeadAfterAlive:
            if (!dead) {
                _seen_alive = true;
                return false;
            }
            if (_seen_alive) {
                _fired = true;
                return true;
            }
            return false;
        }

        throw std::logic_error("Unknown scene transition trigger kind");
    }

    bool SceneTransitionTriggerTracker::has_fired() const {
        return _fired;
    }

    SceneTransitionRequestService::SceneTransitionRequestService(smgpc::runtime::RuntimeContext &runtime)
        : _runtime(runtime),
          _initial_stage_request(make_initial_stage_request()),
          _after_loading_request(StageHostRequest{
              .scene_name = "Game",
              .stage_name = "PeachCastleGardenGalaxy",
              .object_name = "PrologueDirector",
              .scenario_no = 1,
              .appear_after_init = true,
          }) {
#ifndef NDEBUG
        _configured_transition = configured_transition_from_environment();
        if (_configured_transition.has_value()) {
            _configured_trigger.emplace(_configured_transition->trigger);
            _debug_key_request = _configured_transition->request;
        }
#endif
    }

    SceneTransitionRequestService::~SceneTransitionRequestService() = default;

    StageHostRequest SceneTransitionRequestService::make_initial_stage_request() {
        return StageHostRequest{
            .scene_name = "Game",
            .stage_name = "FileSelect",
            .scenario_no = 1,
        };
    }

    const StageHostRequest &SceneTransitionRequestService::initial_stage_request() const {
        return _initial_stage_request;
    }

    void SceneTransitionRequestService::update() {
        if (_runtime.sequence_requests().consume_change_stage_in_game_after_loading_game_data_request() && is_current_data_mario()) {
            request_transition(_after_loading_request, "after_loading_game_data");
        }

#ifndef NDEBUG
        if (_runtime.consume_pending_debug_scene_transition_request() && _debug_key_request.has_value()) {
            request_transition(*_debug_key_request, "debug_key");
        }

        if (_configured_transition.has_value() && _configured_trigger.has_value() && !_configured_trigger->has_fired()) {
            for (const auto &entry : _runtime.scheduler().snapshot()) {
                if (_configured_trigger->observe_name_obj(entry.name, entry.dead)) {
                    request_transition(_configured_transition->request, "configured_trigger");
                    break;
                }
            }
        }
#endif
    }

    std::optional<StageHostRequest> SceneTransitionRequestService::take_pending_request() {
        auto request = std::move(_pending_request);
        _pending_request.reset();
        return request;
    }

#ifndef NDEBUG
    std::optional<ConfiguredSceneTransition> SceneTransitionRequestService::configured_transition_from_environment() {
        const auto trigger_value = environment_string("SMGPC_SCENE_TRANSITION_TRIGGER");
        if (!trigger_value.has_value()) {
            return std::nullopt;
        }
        if (!trigger_value->starts_with(cTriggerPrefix) || trigger_value->size() == cTriggerPrefix.size()) {
            throw std::runtime_error("SMGPC_SCENE_TRANSITION_TRIGGER must be name_obj_dead_after_alive:<name>");
        }

        return ConfiguredSceneTransition{
            .trigger = SceneTransitionTrigger{
                .kind = SceneTransitionTriggerKind::NameObjDeadAfterAlive,
                .name = trigger_value->substr(cTriggerPrefix.size()),
            },
            .request = configured_target_from_environment(),
        };
    }
#endif

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
