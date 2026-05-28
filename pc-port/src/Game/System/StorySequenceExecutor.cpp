#include "Game/System/StorySequenceExecutor.hpp"

#include "Game/System/GameDataHolder.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"

namespace {
    constexpr auto cPrologueDemoName = std::string_view{"Prologue"};
    constexpr auto cPrologueEventName = std::string_view{"cDemoPrologue"};
    constexpr auto cPrologueStageName = std::string_view{"PeachCastleGardenGalaxy"};
    constexpr auto cPrologueDirectorName = std::string_view{"PrologueDirector"};
    constexpr auto cFileSelectStageName = std::string_view{"FileSelect"};
    constexpr auto cGameSceneName = std::string_view{"Game"};

    [[nodiscard]] bool is_current_data_mario() {
        auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        return file == nullptr || file->mGameDataHolder == nullptr || file->mGameDataHolder->isDataMario();
    }
}  // namespace

StorySequenceExecutor::StageRequest StorySequenceExecutor::makeInitialStageRequest() {
    return StageRequest{
        .mSceneName = std::string(cGameSceneName),
        .mStageName = std::string(cFileSelectStageName),
        .mObjectName = {},
        .mActorName = {},
        .mDemoName = {},
        .mEventName = {},
        .mScenarioNo = 1,
        .mAppearAfterInit = false,
    };
}

void StorySequenceExecutor::requestChangeStageInGameAfterLoadingGameData() {
    if (is_current_data_mario()) {
        preparePrologueAfterLoading();
    }
}

std::optional<StorySequenceExecutor::StageRequest> StorySequenceExecutor::takePendingStageRequest() {
    auto request = std::move(mPendingStageRequest);
    mPendingStageRequest.reset();
    return request;
}

bool StorySequenceExecutor::isStoryDemoActive(std::string_view demoName) const {
    return mActiveDemoName == demoName;
}

std::string_view StorySequenceExecutor::getActiveDemoName() const {
    return mActiveDemoName;
}

std::string_view StorySequenceExecutor::getActiveEventName() const {
    return mActiveEventName;
}

void StorySequenceExecutor::preparePrologueAfterLoading() {
    mActiveDemoName = cPrologueDemoName;
    mActiveEventName = cPrologueEventName;
    mPendingStageRequest = StageRequest{
        .mSceneName = std::string(cGameSceneName),
        .mStageName = std::string(cPrologueStageName),
        .mObjectName = std::string(cPrologueDirectorName),
        .mActorName = {},
        .mDemoName = std::string(cPrologueDemoName),
        .mEventName = std::string(cPrologueEventName),
        .mScenarioNo = 1,
        .mAppearAfterInit = true,
    };
}

namespace smgpc::game {
    StorySequenceExecutor& story_sequence_executor() {
        static auto executor = StorySequenceExecutor{};
        return executor;
    }
}  // namespace smgpc::game
