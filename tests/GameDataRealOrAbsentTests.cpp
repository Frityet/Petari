#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/UserFile.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/GameDataFunctionCompat.hpp"
#include "compat/GameDataSession.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

template <typename Exception = std::exception>
void require_throws(const std::function<void()>& operation, std::string_view message) {
    auto threw = false;
    try {
        operation();
    } catch (const Exception&) {
        threw = true;
    }
    require(threw, message);
}
}  // namespace

int main() {
    auto current = UserFile{};
    require(current.mGameDataHolder != nullptr && current.mConfigDataHolder != nullptr,
            "a directly constructed retail UserFile must own its actual holder objects");
    current.resetAllData();
    current.setUserName(L"Rosalina");
    smgpc::compat::game_data::set_holder_name(*current.mGameDataHolder, "mario1");
    require(std::wstring_view(current.mUserName) == L"Rosalina" &&
                std::string_view(current.getGameDataName()) == "mario1",
            "direct UserFile state must retain the retail name and data-file identity");

    auto* holder = current.mGameDataHolder;
    require(holder->isPassedStoryEvent("ゲーム開始直後"),
            "retail story progress zero must include the first StoryEvent BCSV row");
    require(!holder->isPassedStoryEvent("スピン権利"),
            "retail story progress zero must precede the spin entitlement row");
    holder->followStoryEventByName("スピン権利");
    require(holder->isPassedStoryEvent("スピン権利"),
            "following a retail story event must advance numeric story progress");
    require_throws<std::invalid_argument>(
        [&] { holder->followStoryEventByName("invented-story-event"); },
        "an invented story event must not become a boolean alias");

    require(!holder->isOnGameEventFlag("ハチマリオ初変身"),
            "a fresh storable retail flag must be off");
    holder->tryOnGameEventFlag("ハチマリオ初変身");
    require(holder->isOnGameEventFlag("ハチマリオ初変身"),
            "a Type_0 retail flag must use stored flag state");
    require_throws<std::invalid_argument>(
        [&] { static_cast<void>(holder->isOnGameEventFlag("invented-flag")); },
        "an invented game-event query must be rejected by the retail table");
    require_throws<std::invalid_argument>(
        [&] { holder->tryOnGameEventFlag("invented-flag"); },
        "an invented game-event write must be rejected by the retail table");

    holder->setPictureBookChapterAlreadyRead(3);
    require(holder->getPictureBookChapterAlreadyRead() == 3,
            "picture-book progress must use the retail 絵本既読章 value");
    require_throws<std::invalid_argument>(
        [&] { smgpc::compat::game_data::set_holder_event_state(*holder, {{"invented-flag", true}}, {}); },
        "host state binding must reject invented game-event names");

    require_throws<std::logic_error>([&] { holder->makeFileBinary(nullptr, 0U); },
                                     "fabricated game-data serialization must remain absent");
    require_throws<std::logic_error>([&] { static_cast<void>(holder->loadFromFileBinary("mario1", nullptr, 0U)); },
                                     "fabricated game-data deserialization must remain absent");

    require_throws<std::logic_error>([] { static_cast<void>(GameDataFunction::getUserName()); },
                                     "global user state must be unavailable without the retail save sequence");
    require_throws<std::logic_error>([] { static_cast<void>(GameDataFunction::getPictureBookChapterCanRead()); },
                                     "global picture-book state must be unavailable without retail backing");
    require_throws<std::logic_error>([] { GameDataFunction::onGameEventFlag("ハチマリオ初変身"); },
                                     "global event writes must not target a synthetic user file");
    require_throws<std::logic_error>([] { static_cast<void>(GameDataFunction::getSysConfigFileTimeAnnounced()); },
                                     "global system state must be unavailable without retail backing");

    auto checkpoint = GameDataHolder{nullptr};
    smgpc::compat::game_data::set_holder_story_progress(checkpoint, 10U);
    {
        const auto binding = smgpc::compat::ScopedGameDataHolderOverride{checkpoint};
        require(GameDataFunction::getCurrentGameDataHolder() == &checkpoint &&
                    GameDataFunction::getSceneStartGameDataHolder() == &checkpoint &&
                    GameDataFunction::isPassedStoryEvent("チコガイドデモ終了") &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利"),
                "an authored checkpoint must expose its real holder without fabricating save-data ownership");
        GameDataFunction::followStoryEventByName("スピン権利");
        require(GameDataFunction::isPassedStoryEvent("スピン権利") &&
                    smgpc::compat::game_data::holder_story_progress(checkpoint) == 15U,
                "the original story-event API must advance the bound checkpoint holder");
    }
    require_throws<std::logic_error>([] { static_cast<void>(GameDataFunction::getCurrentGameDataHolder()); },
                                     "checkpoint holder binding must restore global save-data absence");

    const auto session_state_baseline = smgpc::compat::game_data::holder_state_count();
    for (auto selected_file = u16{1U}; selected_file <= 6U; ++selected_file) {
        {
            auto session = smgpc::compat::GameDataSession{selected_file};
            const auto expected_name = std::string("mario") + std::to_string(selected_file);
            require(session.selected_file() == selected_file &&
                        std::string_view(session.holder().mName) == expected_name,
                    "a selected-file session must retain the exact marioN identity for all six files");
            require(GameDataFunction::getCurrentGameDataHolder() == &session.holder() &&
                        GameDataFunction::getSceneStartGameDataHolder() == &session.holder(),
                    "a selected-file session must bind one owned holder as current and scene-start data");
            require(smgpc::compat::game_data::holder_story_progress(session.holder()) == 5U &&
                        GameDataFunction::isPassedStoryEvent("ゲーム開始直後") &&
                        GameDataFunction::isPassedStoryEvent("クッパ襲来後") &&
                        GameDataFunction::isPassedStoryEvent("ピーチ城浮上後") &&
                        !GameDataFunction::isPassedStoryEvent("チコガイドデモ終了") &&
                        !GameDataFunction::isPassedStoryEvent("スピン権利"),
                    "a selected-file session must begin at the exact post-castle-rise story boundary");
        }
        require(smgpc::compat::game_data::holder_state_count() == session_state_baseline,
                "destroying each selected-file session must reclaim its holder state");
    }

    require_throws<std::out_of_range>(
        [] { static_cast<void>(smgpc::compat::GameDataSession{0U}); },
        "selected-file zero must be rejected");
    require_throws<std::out_of_range>(
        [] { static_cast<void>(smgpc::compat::GameDataSession{7U}); },
        "selected files above the retail six slots must be rejected");
    require(smgpc::compat::game_data::holder_state_count() == session_state_baseline,
            "rejected selected-file sessions must not leak holder state");

    {
        auto outer = smgpc::compat::GameDataSession{2U};
        auto* const outer_holder = &outer.holder();
        require(smgpc::compat::game_data::holder_story_progress(*outer_holder) == 5U,
                "the outer selected-file holder must begin at story progress 5");

        GameDataFunction::followStoryEventByName("チコガイドデモ終了");
        require(GameDataFunction::getCurrentGameDataHolder() == outer_holder &&
                    smgpc::compat::game_data::holder_story_progress(*outer_holder) == 10U &&
                    GameDataFunction::isPassedStoryEvent("チコガイドデモ終了") &&
                    !GameDataFunction::isPassedStoryEvent("スピン権利"),
                "the bound selected-file holder must advance in place from progress 5 to 10");

        {
            auto inner = smgpc::compat::GameDataSession{6U};
            require(GameDataFunction::getCurrentGameDataHolder() == &inner.holder() &&
                        GameDataFunction::getSceneStartGameDataHolder() == &inner.holder() &&
                        smgpc::compat::game_data::holder_story_progress(inner.holder()) == 5U,
                    "a nested selected-file session must replace both bindings with its own seeded holder");
        }

        require(GameDataFunction::getCurrentGameDataHolder() == outer_holder &&
                    GameDataFunction::getSceneStartGameDataHolder() == outer_holder &&
                    smgpc::compat::game_data::holder_story_progress(*outer_holder) == 10U,
                "destroying a nested selected-file session must restore the unchanged outer holder");
        GameDataFunction::followStoryEventByName("スピン権利");
        require(GameDataFunction::getCurrentGameDataHolder() == outer_holder &&
                    smgpc::compat::game_data::holder_story_progress(*outer_holder) == 15U &&
                    GameDataFunction::isPassedStoryEvent("スピン権利"),
                "the same selected-file holder must advance in place from progress 10 to 15");
    }
    require(smgpc::compat::game_data::holder_state_count() == session_state_baseline,
            "nested selected-file session teardown must reclaim both owned holder states");
    require_throws<std::logic_error>([] { static_cast<void>(GameDataFunction::getCurrentGameDataHolder()); },
                                     "selected-file session teardown must restore global save-data absence");

    std::cout << "Game-data real-or-absent tests passed: selected-file session contract included\n";
    return 0;
}
