#include "scene/SequenceBootService.hpp"

#include "Game/Map/FileSelector.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string_view>

namespace smgpc::compat {
    SequenceBootService::SequenceBootService(RuntimeContext &runtime, StorySequenceService &story_sequence, StageHostService &stage_host)
        : _runtime(runtime), _story_sequence(story_sequence), _stage_host(stage_host) {
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
        ensure_file_select_host();
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
#endif
    }

    bool SequenceBootService::is_boot_requested() const {
        return _boot_requested;
    }

    bool SequenceBootService::is_file_select_host_active() const {
        return _file_selector != nullptr;
    }

    bool SequenceBootService::has_sent_autorush_begin() const {
        return _autorush_begin_sent;
    }

    void SequenceBootService::ensure_file_select_host() {
        if (_file_selector != nullptr) {
            return;
        }

#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "temporary_file_select_stage_host_started",
                                           "direct FileSelector host until GameScene placement is available");
        _runtime.emit_sequence_state_trace_event("temporary_stage_host_started", "host=FileSelector");
#endif
        _file_selector = std::make_unique<FileSelector>("ファイルセレクタ");
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("file_select", "file_selector_constructed", "sequence boot host");
#endif
        _file_selector->initWithoutIter();
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("file_select", "file_selector_initialized", "sequence boot host initWithoutIter");
        _title_product_created_emitted = true;
        _runtime.emit_semantic_trace_event("title", "title_product_created", "source=FileSelector;layouts=TitleLogo,PressStart");
#endif
    }

#ifndef NDEBUG
    void SequenceBootService::emit_title_semantic_anchors() {
        if (_file_selector == nullptr) {
            return;
        }

        if (!_title_product_visible_emitted && _file_selector->isTitleStarted() && has_active_layout(_runtime, "TitleLogo")) {
            _title_product_visible_emitted = true;
            _runtime.emit_semantic_trace_event("title", "title_product_visible", "layout=TitleLogo");
        }

        if (!_title_ab_gate_active_emitted && _file_selector->isTitleActive() && has_active_layout(_runtime, "PressStart")) {
            _title_ab_gate_active_emitted = true;
            _runtime.emit_semantic_trace_event("title", "ab_gate_active", "layout=PressStart;buttons=A+B");
        }

        if (!_title_input_accepted_emitted && has_system_sound(_runtime, "SE_SY_GAME_START")) {
            _title_input_accepted_emitted = true;
            _runtime.emit_semantic_trace_event("title", "title_input_accepted", "sound=SE_SY_GAME_START;buttons=A+B");
        }

        if (!_file_select_scene_requested_emitted && _file_selector->isTitleEnded()) {
            _file_select_scene_requested_emitted = true;
            _runtime.emit_semantic_trace_event("title", "file_select_scene_requested", "source=FileSelectorTitleEnd;stage=FileSelect");
            _runtime.emit_sequence_state_trace_event("file_select_scene_requested", "source=title;stage=FileSelect");
        }
    }

}  // namespace smgpc::compat
