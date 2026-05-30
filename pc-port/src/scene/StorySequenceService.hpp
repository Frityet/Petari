#pragma once

#include "scene/StageHostService.hpp"

#include <optional>

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    class StorySequenceService final {
    public:
        explicit StorySequenceService(smgpc::runtime::RuntimeContext &runtime);
        ~StorySequenceService();

        StorySequenceService(const StorySequenceService &) = delete;
        StorySequenceService &operator=(const StorySequenceService &) = delete;

        void update_after_loading_request();

        [[nodiscard]] std::optional<StageHostRequest> take_pending_stage_request();

    private:
        smgpc::runtime::RuntimeContext &_runtime;
        std::optional<StageHostRequest> _pending_stage_request;
    };

}  // namespace smgpc::scene
