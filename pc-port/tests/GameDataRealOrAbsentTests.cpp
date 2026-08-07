#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/UserFile.hpp"
#include "compat/GameDataHolderCompat.hpp"

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

    std::cout << "Game-data real-or-absent tests passed: 17/17\n";
    return 0;
}
