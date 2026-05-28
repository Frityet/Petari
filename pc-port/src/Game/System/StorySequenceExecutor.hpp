#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <revolution.h>

class StorySequenceExecutor {
public:
    struct StageRequest {
        std::string mSceneName;
        std::string mStageName;
        std::string mObjectName;
        std::string mActorName;
        std::string mDemoName;
        std::string mEventName;
        s32 mScenarioNo = 1;
        bool mAppearAfterInit = false;
    };

    [[nodiscard]] static StageRequest makeInitialStageRequest();

    void requestChangeStageInGameAfterLoadingGameData();
    [[nodiscard]] std::optional<StageRequest> takePendingStageRequest();
    [[nodiscard]] bool isStoryDemoActive(std::string_view demoName) const;
    [[nodiscard]] std::string_view getActiveDemoName() const;
    [[nodiscard]] std::string_view getActiveEventName() const;

private:
    void preparePrologueAfterLoading();

    std::optional<StageRequest> mPendingStageRequest;
    std::string mActiveDemoName;
    std::string mActiveEventName;
};

namespace smgpc::game {
    StorySequenceExecutor& story_sequence_executor();
}  // namespace smgpc::game
