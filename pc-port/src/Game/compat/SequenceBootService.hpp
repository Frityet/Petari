#pragma once

#include "Game/compat/StageHostService.hpp"
#include "Game/compat/StorySequenceService.hpp"

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
        [[nodiscard]] bool is_picturebook_host_active() const;
        [[nodiscard]] bool has_sent_autorush_begin() const;
        [[nodiscard]] bool has_picturebook_reached() const;

    private:
        void ensure_file_select_stage_host();
        void update_stage_transition_requests();
#ifndef NDEBUG
        void update_picturebook_reachability();
        void emit_title_semantic_anchors();
        void emit_file_select_semantic_anchors();
#endif

        RuntimeContext &_runtime;
        StorySequenceService _story_sequence;
        StageHostService _stage_host;
        bool _boot_requested = false;
        bool _autorush_begin_sent = false;
        bool _picturebook_reached = false;
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
