#pragma once

#include "Game/compat/StageHostService.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace smgpc::game {

    class RuntimeContext;

    class StorySequenceService final {
    public:
        explicit StorySequenceService(RuntimeContext &runtime);
        ~StorySequenceService();

        StorySequenceService(const StorySequenceService &) = delete;
        StorySequenceService &operator=(const StorySequenceService &) = delete;

        void update_after_loading_request();

        [[nodiscard]] std::optional<StageHostRequest> take_pending_stage_request();
        [[nodiscard]] bool is_story_demo_active(std::string_view demo_name) const;
        [[nodiscard]] std::string_view active_demo_name() const;
        [[nodiscard]] std::string_view active_event_name() const;

    private:
        void prepare_prologue_after_loading();

        RuntimeContext &_runtime;
        std::optional<StageHostRequest> _pending_stage_request;
        std::string _active_demo_name;
        std::string _active_event_name;
        bool _after_loading_request_consumed = false;
    };

}  // namespace smgpc::game
