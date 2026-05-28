#pragma once

#include "Game/compat/StageHostService.hpp"
#include "Game/compat/StorySequenceService.hpp"

#include <string>

namespace smgpc::game {

    class RuntimeContext;

    class SequenceBootService final {
    public:
        explicit SequenceBootService(RuntimeContext &runtime);
        ~SequenceBootService();

        SequenceBootService(const SequenceBootService &) = delete;
        SequenceBootService &operator=(const SequenceBootService &) = delete;

        void request_boot_to_initial_stage();
        void update_after_runtime_frame();

        [[nodiscard]] bool is_boot_requested() const;
        [[nodiscard]] bool is_initial_stage_host_active() const;
        [[nodiscard]] bool has_sent_autorush_begin() const;

    private:
        void update_stage_transition_requests();

        RuntimeContext &_runtime;
        StorySequenceService _story_sequence;
        StageHostService _stage_host;
        std::string _boot_stage_name;
        bool _boot_requested = false;
        bool _autorush_begin_sent = false;
    };

}  // namespace smgpc::game
