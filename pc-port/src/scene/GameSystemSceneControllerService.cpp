#include "scene/GameSystemSceneControllerService.hpp"

#include "runtime/RuntimeContext.hpp"
#include "scene/SceneLifecycleService.hpp"

#include <string>

namespace smgpc::compat {
    namespace {
        [[nodiscard]] std::string phase_name(SceneControllerPhase phase) {
            switch (phase) {
            case SceneControllerPhase::NotInitialized:
                return "NotInitialized";
            case SceneControllerPhase::WaitingForRequest:
                return "WaitingForRequest";
            case SceneControllerPhase::RequestQueued:
                return "RequestQueued";
            case SceneControllerPhase::DestroyScene:
                return "DestroyScene";
            case SceneControllerPhase::InitializeScene:
                return "InitializeScene";
            case SceneControllerPhase::ReadyToStartScene:
                return "ReadyToStartScene";
            case SceneControllerPhase::Normal:
                return "Normal";
            }
            return "Unknown";
        }
    }  // namespace

    GameSystemSceneControllerService::GameSystemSceneControllerService(RuntimeContext &runtime, SceneLifecycleService &scene_lifecycle)
        : _runtime(runtime), _scene_lifecycle(scene_lifecycle) {
        set_phase(SceneControllerPhase::WaitingForRequest);
    }

    GameSystemSceneControllerService::~GameSystemSceneControllerService() = default;

    void GameSystemSceneControllerService::request_change_scene(const StageHostRequest &request) {
        _pending_scene = SceneControlInfoCompat{
            .scene_name = request.scene_name,
            .stage_name = request.stage_name,
            .scenario_no = request.scenario_no,
            .selected_scenario_no = request.scenario_no,
            .object_name = request.object_name,
            .actor_name = request.actor_name,
            .appear_after_init = request.appear_after_init,
        };
        set_phase(SceneControllerPhase::RequestQueued);
#ifndef NDEBUG
        const auto detail = "next_scene=" + request.scene_name + ";next_stage=" + request.stage_name +
                            ";scenario=" + std::to_string(request.scenario_no);
        _runtime.emit_sequence_state_trace_event("scene_change_requested", detail);
        _runtime.emit_semantic_trace_event("scene_controller", "scene_change_requested", detail);
#endif
    }

    void GameSystemSceneControllerService::check_request_and_change_scene() {
        if (!_pending_scene.has_value()) {
            return;
        }

        if (_scene_lifecycle.has_active_stage(_pending_scene->stage_name) &&
            _scene_lifecycle.active_scenario_no() == _pending_scene->scenario_no) {
            _pending_scene.reset();
            set_phase(SceneControllerPhase::Normal);
            return;
        }

        apply_pending_scene();
    }

    void GameSystemSceneControllerService::destroy_scene() {
        if (_scene_lifecycle.active_scene() == nullptr) {
            return;
        }

        set_phase(SceneControllerPhase::DestroyScene);
#ifndef NDEBUG
        const auto detail = "scene=" + std::string(_scene_lifecycle.active_scene_name()) + ";stage=" +
                            std::string(_scene_lifecycle.active_stage_name());
        _runtime.emit_sequence_state_trace_event("scene_destroyed", detail);
        _runtime.emit_semantic_trace_event("scene_controller", "scene_destroyed", detail);
#endif
        _scene_lifecycle.destroy_scene();
        set_phase(SceneControllerPhase::WaitingForRequest);
    }

    bool GameSystemSceneControllerService::has_pending_request() const {
        return _pending_scene.has_value();
    }

    bool GameSystemSceneControllerService::has_active_stage(std::string_view stage_name) const {
        return _scene_lifecycle.has_active_stage(stage_name);
    }

    std::string_view GameSystemSceneControllerService::active_scene_name() const {
        return _scene_lifecycle.active_scene_name();
    }

    std::string_view GameSystemSceneControllerService::active_stage_name() const {
        return _scene_lifecycle.active_stage_name();
    }

    s32 GameSystemSceneControllerService::active_scenario_no() const {
        return _scene_lifecycle.active_scenario_no();
    }

    SceneControllerPhase GameSystemSceneControllerService::phase() const {
        return _phase;
    }

    const std::optional<SceneControlInfoCompat> &GameSystemSceneControllerService::pending_scene() const {
        return _pending_scene;
    }

    void GameSystemSceneControllerService::apply_pending_scene() {
        set_phase(SceneControllerPhase::DestroyScene);
        _scene_lifecycle.destroy_scene();

        set_phase(SceneControllerPhase::InitializeScene);
        const auto request = pending_request();
        _scene_lifecycle.request_stage(request);
        _pending_scene.reset();

        set_phase(SceneControllerPhase::ReadyToStartScene);
        set_phase(SceneControllerPhase::Normal);
#ifndef NDEBUG
        const auto detail = "current_scene=" + request.scene_name + ";current_stage=" + request.stage_name +
                            ";scenario=" + std::to_string(request.scenario_no);
        _runtime.emit_sequence_state_trace_event("scene_change_applied", detail);
        _runtime.emit_semantic_trace_event("scene_controller", "scene_change_applied", detail);
#endif
    }

    void GameSystemSceneControllerService::set_phase(SceneControllerPhase phase) {
        if (_phase == phase) {
            return;
        }

        _phase = phase;
#ifndef NDEBUG
        _runtime.emit_sequence_state_trace_event("scene_controller_phase", "phase=" + phase_name(phase));
#endif
    }

    StageHostRequest GameSystemSceneControllerService::pending_request() const {
        const auto &pending = *_pending_scene;
        return StageHostRequest{
            .scene_name = pending.scene_name,
            .stage_name = pending.stage_name,
            .object_name = pending.object_name,
            .actor_name = pending.actor_name,
            .scenario_no = pending.scenario_no,
            .appear_after_init = pending.appear_after_init,
        };
    }

}  // namespace smgpc::compat
