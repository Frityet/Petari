#pragma once

#include "Game/compat/StageHostService.hpp"

#include <optional>

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

    private:
        RuntimeContext &_runtime;
        std::optional<StageHostRequest> _pending_stage_request;
        bool _after_loading_request_consumed = false;
    };

}  // namespace smgpc::game
