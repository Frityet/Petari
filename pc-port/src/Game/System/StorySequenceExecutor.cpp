#include "Game/System/StorySequenceExecutor.hpp"

#include "Game/System/GameDataHolder.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"

#include <cstdlib>

namespace {
    constexpr auto cPrologueDemoName = std::string_view{"Prologue"};
    constexpr auto cPrologueEventName = std::string_view{"cDemoPrologue"};
    constexpr auto cPrologueStageName = std::string_view{"PeachCastleGardenGalaxy"};
    constexpr auto cPrologueDirectorName = std::string_view{"PrologueDirector"};
    constexpr auto cHeavensDoorDemoName = std::string_view{"HeavensDoorBunny"};
    constexpr auto cHeavensDoorEventName = std::string_view{"cDemoHeavensDoorBunny"};
    constexpr auto cHeavensDoorStageName = std::string_view{"HeavensDoorGalaxy"};
    constexpr auto cFileSelectStageName = std::string_view{"FileSelect"};
    constexpr auto cGameSceneName = std::string_view{"Game"};

    [[nodiscard]] bool is_current_data_mario() {
        auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        return file == nullptr || file->mGameDataHolder == nullptr || file->mGameDataHolder->isDataMario();
    }

#ifndef NDEBUG
    [[nodiscard]] bool debug_env_equals(const char* name, std::string_view expected) {
        const auto* value = std::getenv(name);
        return value != nullptr && std::string_view(value) == expected;
    }

    [[nodiscard]] bool debug_heavensdoor_after_picturebook_enabled() {
        return debug_env_equals("SMGPC_DEMO_ROUTE", "heavensdoor_bunny") || debug_env_equals("SMGPC_DEMO_ROUTE", "heavensdoor_after_picturebook");
    }
#endif

    [[nodiscard]] StorySequenceExecutor::StageRequest make_heavensdoor_bunny_request() {
        return StorySequenceExecutor::StageRequest{
            .mSceneName = std::string(cGameSceneName),
            .mStageName = std::string(cHeavensDoorStageName),
            .mObjectName = {},
            .mActorName = {},
            .mDemoName = std::string(cHeavensDoorDemoName),
            .mEventName = std::string(cHeavensDoorEventName),
            .mScenarioNo = 1,
            .mAppearAfterInit = true,
            .mFailUnsupportedPlacement = false,
        };
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
    if (!mPendingStageRequest.has_value() && is_current_data_mario()) {
        preparePrologueAfterLoading();
    }
}

void StorySequenceExecutor::requestHeavensDoorBunnyDemoAfterPictureBook() {
    prepareHeavensDoorBunnyDemoAfterPictureBook();
}

std::optional< StorySequenceExecutor::StageRequest > StorySequenceExecutor::takePendingStageRequest() {
    auto request = std::move(mPendingStageRequest);
    mPendingStageRequest.reset();
    return request;
}

bool StorySequenceExecutor::isStoryDemoActive(std::string_view demoName) const {
    return mActiveDemoName == demoName;
}

bool StorySequenceExecutor::shouldRouteToHeavensDoorBunnyDemoAfterPictureBook() const {
#ifndef NDEBUG
    return debug_heavensdoor_after_picturebook_enabled();
#else
    return false;
#endif
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
        .mFailUnsupportedPlacement = false,
    };
}

void StorySequenceExecutor::prepareHeavensDoorBunnyDemoAfterPictureBook() {
    mActiveDemoName = cHeavensDoorDemoName;
    mActiveEventName = cHeavensDoorEventName;
    mPendingStageRequest = make_heavensdoor_bunny_request();
}

namespace smgpc::game {
    StorySequenceExecutor& story_sequence_executor() {
        static auto executor = StorySequenceExecutor{};
        return executor;
    }
}  // namespace smgpc::game
