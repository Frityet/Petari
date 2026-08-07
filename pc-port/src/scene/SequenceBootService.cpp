#include "scene/SequenceBootService.hpp"

#include "Game/Util/ActorSensorUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>

namespace smgpc::scene {
    SequenceBootService::SequenceBootService(smgpc::runtime::RuntimeContext &runtime, SceneTransitionRequestService &scene_transitions,
                                             StageHostService &stage_host)
        : _runtime(runtime), _scene_transitions(scene_transitions), _stage_host(stage_host) {
    }

    SequenceBootService::~SequenceBootService() = default;

    void SequenceBootService::request_boot_to_initial_stage() {
        if (_boot_requested) {
            return;
        }

        _boot_requested = true;
        const auto &request = _scene_transitions.initial_stage_request();
        _boot_stage_name = request.stage_name;
        _runtime.set_current_sequence_scene_name(request.scene_name);
        _runtime.set_next_sequence_scene_name(request.stage_name);
        _runtime.set_current_stage_name(request.stage_name);
#ifndef NDEBUG
        const auto detail = "scene=" + request.scene_name + ";stage=" + request.stage_name +
                            ";scenario=" + std::to_string(request.scenario_no);
        _runtime.emit_sequence_state_trace_event("stage_requested", "requested_stage=" + request.stage_name +
                                                                        ";scenario=" + std::to_string(request.scenario_no));
        _runtime.emit_semantic_trace_event("sequence", "boot_stage_requested", detail);
#endif
        _stage_host.request_stage(request);
    }

    void SequenceBootService::update_after_runtime_frame() {
        if (!_boot_requested) {
            return;
        }

        _stage_host.update_scene_requests();

        if (!_autorush_begin_sent) {
            MR::sendMsgToAllLiveActor(ACTMES_AUTORUSH_BEGIN, nullptr);
            _autorush_begin_sent = true;
#ifndef NDEBUG
            _runtime.emit_sequence_state_trace_event("autorush_begin_sent", "message=ACTMES_AUTORUSH_BEGIN");
            _runtime.emit_semantic_trace_event("sequence", "autorush_begin_sent", "ACTMES_AUTORUSH_BEGIN");
#endif
        }

        update_stage_transition_requests();
        _stage_host.update_scene_requests();
    }

    bool SequenceBootService::is_boot_requested() const {
        return _boot_requested;
    }

    bool SequenceBootService::is_initial_stage_host_active() const {
        return !_boot_stage_name.empty() && _stage_host.has_active_stage(_boot_stage_name);
    }

    bool SequenceBootService::has_sent_autorush_begin() const {
        return _autorush_begin_sent;
    }

    void SequenceBootService::update_stage_transition_requests() {
        _scene_transitions.update();
        if (auto request = _scene_transitions.take_pending_request()) {
            _stage_host.request_stage(*request);
        }
    }

}  // namespace smgpc::scene
