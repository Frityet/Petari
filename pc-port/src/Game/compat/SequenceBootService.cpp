#include "Game/compat/SequenceBootService.hpp"

#include "Game/System/StorySequenceExecutor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <string>

namespace smgpc::game {
    SequenceBootService::SequenceBootService(RuntimeContext &runtime, StorySequenceService &story_sequence, StageHostService &stage_host)
        : _runtime(runtime), _story_sequence(story_sequence), _stage_host(stage_host) {
    }

    SequenceBootService::~SequenceBootService() = default;

    void SequenceBootService::request_boot_to_initial_stage() {
        if (_boot_requested) {
            return;
        }

        _boot_requested = true;
        const auto request = StorySequenceExecutor::makeInitialStageRequest();
        _boot_stage_name = request.mStageName;
        _runtime.set_current_sequence_scene_name(request.mSceneName);
        _runtime.set_next_sequence_scene_name(request.mStageName);
        _runtime.set_current_stage_name(request.mStageName);
#ifndef NDEBUG
        const auto detail = "scene=" + request.mSceneName + ";stage=" + request.mStageName +
                            ";scenario=" + std::to_string(request.mScenarioNo);
        _runtime.emit_sequence_state_trace_event("stage_requested", "requested_stage=" + request.mStageName +
                                                                        ";scenario=" + std::to_string(request.mScenarioNo));
        _runtime.emit_semantic_trace_event("sequence", "boot_stage_requested", detail);
#endif
        _stage_host.request_stage(StageHostRequest{
            .scene_name = request.mSceneName,
            .stage_name = request.mStageName,
            .object_name = request.mObjectName,
            .actor_name = request.mActorName,
            .scenario_no = request.mScenarioNo,
            .appear_after_init = request.mAppearAfterInit,
        });
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
        _story_sequence.update_after_loading_request();
        if (auto request = _story_sequence.take_pending_stage_request()) {
            _stage_host.request_stage(*request);
        }
    }

}  // namespace smgpc::game
