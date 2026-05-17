#pragma once

#include "Game/compat/StageHostService.hpp"

namespace smgpc::game {

    class RuntimeContext;

    class SequenceBootService final {
    public:
        explicit SequenceBootService(RuntimeContext &runtime);
        ~SequenceBootService();

        SequenceBootService(const SequenceBootService &) = delete;
        SequenceBootService &operator=(const SequenceBootService &) = delete;

        void request_boot_to_file_select();
        void update_after_runtime_frame();

        [[nodiscard]] bool is_boot_requested() const;
        [[nodiscard]] bool is_file_select_host_active() const;
        [[nodiscard]] bool has_sent_autorush_begin() const;

    private:
        void ensure_file_select_stage_host();
#ifndef NDEBUG
        void emit_title_semantic_anchors();
        void emit_file_select_semantic_anchors();
#endif

        RuntimeContext &_runtime;
        StageHostService _stage_host;
        bool _boot_requested = false;
        bool _autorush_begin_sent = false;
#ifndef NDEBUG
        bool _title_product_created_emitted = false;
        bool _title_product_visible_emitted = false;
        bool _title_ab_gate_active_emitted = false;
        bool _title_input_accepted_emitted = false;
        bool _file_select_scene_requested_emitted = false;
        bool _file_select_title_nerve_entered_emitted = false;
        bool _file_select_start_entered_emitted = false;
        bool _file_select_selectable_emitted = false;
        bool _file_select_demo_start_wait_emitted = false;
        bool _file_select_demo_transition_requested_emitted = false;
#endif
    };

}  // namespace smgpc::game
