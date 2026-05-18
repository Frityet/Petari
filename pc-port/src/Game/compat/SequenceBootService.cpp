#include "Game/compat/SequenceBootService.hpp"

#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <string_view>

namespace smgpc::game {
#ifndef NDEBUG
    namespace {

        [[nodiscard]] bool has_active_layout(RuntimeContext &runtime, std::string_view layout_name) {
            const auto layouts = runtime.scheduler().debug_layout_runtime_snapshot();
            for (const auto &layout : layouts) {
                if (layout.layout_name != layout_name || layout.dead || layout.suspended) {
                    continue;
                }

                if (layout.panes.empty()) {
                    return true;
                }

                for (const auto &pane : layout.panes) {
                    if (pane.effective_visible) {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool has_system_sound(RuntimeContext &runtime, std::string_view name) {
            for (const auto &event : runtime.audio().events()) {
                if (event.kind == AudioEventKind::SystemSoundStart && event.name == name) {
                    return true;
                }
            }

            return false;
        }

    }  // namespace
#endif

    SequenceBootService::SequenceBootService(RuntimeContext &runtime) : _runtime(runtime), _story_sequence(runtime), _stage_host(runtime) {
    }

    SequenceBootService::~SequenceBootService() = default;

    void SequenceBootService::request_boot_to_file_select() {
        if (_boot_requested) {
            return;
        }

        _boot_requested = true;
        _runtime.set_current_sequence_scene_name("Game");
        _runtime.set_next_sequence_scene_name("FileSelect");
        _runtime.set_current_stage_name("FileSelect");
#ifndef NDEBUG
        _runtime.emit_sequence_state_trace_event("stage_requested", "requested_stage=FileSelect;scenario=1");
        _runtime.emit_semantic_trace_event("sequence", "boot_file_select_requested", "scene=Game;stage=FileSelect;scenario=1");
#endif
        ensure_file_select_stage_host();
    }

    void SequenceBootService::update_after_runtime_frame() {
        if (!_boot_requested) {
            return;
        }

        if (!_autorush_begin_sent) {
            MR::sendMsgToAllLiveActor(ACTMES_AUTORUSH_BEGIN, nullptr);
            _autorush_begin_sent = true;
#ifndef NDEBUG
            _runtime.emit_sequence_state_trace_event("autorush_begin_sent", "message=ACTMES_AUTORUSH_BEGIN");
            _runtime.emit_semantic_trace_event("sequence", "file_select_autorush_begin_sent", "ACTMES_AUTORUSH_BEGIN");
#endif
        }

#ifndef NDEBUG
        emit_title_semantic_anchors();
        emit_file_select_semantic_anchors();
        update_picturebook_reachability();
#endif
        update_stage_transition_requests();
    }

    bool SequenceBootService::is_boot_requested() const {
        return _boot_requested;
    }

    bool SequenceBootService::is_file_select_host_active() const {
        return _stage_host.has_active_stage("FileSelect");
    }

    bool SequenceBootService::is_picturebook_host_active() const {
        return _story_sequence.is_story_demo_active("Prologue") && _stage_host.has_active_stage("PeachCastleGardenGalaxy");
    }

    bool SequenceBootService::has_sent_autorush_begin() const {
        return _autorush_begin_sent;
    }

    bool SequenceBootService::has_picturebook_reached() const {
        return _picturebook_reached;
    }

    void SequenceBootService::ensure_file_select_stage_host() {
        _stage_host.request_stage(StageHostRequest{
            .scene_name = "Game",
            .stage_name = "FileSelect",
            .object_name = {},
            .actor_name = {},
            .scenario_no = 1,
        });
#ifndef NDEBUG
        _title_product_created_emitted = true;
#endif
    }

    void SequenceBootService::update_stage_transition_requests() {
        _story_sequence.update_after_loading_request();
        if (auto request = _story_sequence.take_pending_stage_request()) {
            _stage_host.request_stage(*request);
        }
    }

#ifndef NDEBUG
    void SequenceBootService::update_picturebook_reachability() {
        if (!_story_sequence.is_story_demo_active("Prologue") || _picturebook_reached || !has_active_layout(_runtime, "PrologueDemo")) {
            return;
        }

        _picturebook_reached = true;
        _runtime.emit_sequence_state_trace_event(
            "picturebook_reached", "event=cDemoPrologue;demo=Prologue;stage=PeachCastleGardenGalaxy;host=PrologueDirector;layout=PrologueDemo");
        _runtime.emit_semantic_trace_event(
            "picturebook", "picturebook_reached",
            "event=cDemoPrologue;demo=Prologue;stage=PeachCastleGardenGalaxy;host=PrologueDirector;layout=PrologueDemo");
    }

    void SequenceBootService::emit_title_semantic_anchors() {
        const auto state = _stage_host.file_select_state();
        if (!state.has_value()) {
            return;
        }

        if (!_title_product_visible_emitted && state->title_started && has_active_layout(_runtime, "TitleLogo")) {
            _title_product_visible_emitted = true;
            _runtime.emit_semantic_trace_event("title", "title_product_visible", "layout=TitleLogo");
        }

        if (!_title_ab_gate_active_emitted && state->title_active && has_active_layout(_runtime, "PressStart")) {
            _title_ab_gate_active_emitted = true;
            _runtime.emit_semantic_trace_event("title", "ab_gate_active", "layout=PressStart;buttons=A+B");
        }

        if (!_title_input_accepted_emitted && has_system_sound(_runtime, "SE_SY_GAME_START")) {
            _title_input_accepted_emitted = true;
            _runtime.emit_semantic_trace_event("title", "title_input_accepted", "sound=SE_SY_GAME_START;buttons=A+B");
        }

        if (!_file_select_scene_requested_emitted && state->title_ended) {
            _file_select_scene_requested_emitted = true;
            _runtime.emit_semantic_trace_event("title", "file_select_scene_requested", "source=FileSelectorTitleEnd;stage=FileSelect");
            _runtime.emit_sequence_state_trace_event("file_select_scene_requested", "source=title;stage=FileSelect");
        }
    }

    void SequenceBootService::emit_file_select_semantic_anchors() {
        const auto state = _stage_host.file_select_state();
        if (!state.has_value()) {
            return;
        }

        if (!_file_select_title_nerve_entered_emitted && state->title_started) {
            _file_select_title_nerve_entered_emitted = true;
            _runtime.emit_semantic_trace_event("file_select", "title_nerve_entered", "source=FileSelector");
        }

        if (!_file_select_start_entered_emitted &&
            (state->file_select_start || (state->title_ended && !state->file_select_started))) {
            _file_select_start_entered_emitted = true;
            _runtime.emit_semantic_trace_event("file_select", "file_select_start_entered", "source=FileSelector");
        }

        if (!_file_select_selectable_emitted && state->file_select_started) {
            _file_select_selectable_emitted = true;
            _runtime.emit_semantic_trace_event("file_select", "file_select_selectable", "source=FileSelector");
        }

        if (!_file_select_demo_start_wait_emitted && state->demo_start_wait) {
            _file_select_demo_start_wait_emitted = true;
            _runtime.emit_semantic_trace_event("file_select", "demo_start_wait", "source=FileSelector");
        }

        if (!_file_select_demo_transition_requested_emitted &&
            _runtime.sequence_requests().is_change_stage_in_game_after_loading_game_data_requested()) {
            _file_select_demo_transition_requested_emitted = true;
            _runtime.emit_semantic_trace_event("file_select", "demo_transition_requested", "change_stage_in_game_after_loading_game_data");
        }
    }
#endif

}  // namespace smgpc::game
