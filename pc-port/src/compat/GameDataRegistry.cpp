#include "compat/GameDataRegistry.hpp"

#include <array>
#include <stdexcept>
#include <string>

#include "Game/System/GameEventFlag.hpp"
#include "Game/System/GameEventFlagTable.hpp"

namespace smgpc::compat::game_data {
namespace {
constexpr auto cStoryEvents = std::array{
    StoryEventEntry{"ゲーム開始直後", 0},
    StoryEventEntry{"クッパ襲来後", 2},
    StoryEventEntry{"ピーチ城浮上後", 5},
    StoryEventEntry{"チコガイドデモ終了", 10},
    StoryEventEntry{"スピン権利", 15},
    StoryEventEntry{"バトラー情報Ａ", 25},
    StoryEventEntry{"天球儀レクチャー", 30},
    StoryEventEntry{"ギャラクシー移動レクチャー", 35},
    StoryEventEntry{"スターピースレクチャー", 40},
    StoryEventEntry{"クッパＪｒロボプラント発見", 42},
    StoryEventEntry{"クッパスタープラント発見", 45},
    StoryEventEntry{"クッパＪｒシッププラント発見", 50},
    StoryEventEntry{"クッパダークマタープラント発見", 55},
    StoryEventEntry{"クッパＪｒクリーチャープラント発見", 60},
};

constexpr auto cEventValues = std::array{
    EventValueEntry{"ペンギンレース[オーシャンリング]/hi", 0},
    EventValueEntry{"ペンギンレース[オーシャンリング]/lo", 90 * 60},
    EventValueEntry{"テレサレース[ファントム]/hi", 0},
    EventValueEntry{"テレサレース[ファントム]/lo", 90 * 60},
    EventValueEntry{"テレサレース[デスプロムナード]/hi", 0},
    EventValueEntry{"テレサレース[デスプロムナード]/lo", 90 * 60},
    EventValueEntry{"サーフィン[トライアル]/hi", 0},
    EventValueEntry{"サーフィン[トライアル]/lo", 90 * 60},
    EventValueEntry{"サーフィン[チャレンジ]/hi", 0},
    EventValueEntry{"サーフィン[チャレンジ]/lo", 90 * 60},
    EventValueEntry{"LibraryOpenNewStarCount", 1},
    EventValueEntry{"絵本既読章", 0},
    EventValueEntry{"MsgLedPattern", 1},
    EventValueEntry{"LuigiEventState", 0xff00},
    EventValueEntry{"WarpPodSaveBits", 0},
    EventValueEntry{"TicoGalaxyAlreadyTalk", 0},
    EventValueEntry{"MessageAlreadyRead", 0},
    EventValueEntry{"MissPointForLetter", 0},
    EventValueEntry{"MissNum", 0},
    EventValueEntry{"Comet1Status", 0},
    EventValueEntry{"Comet2Status", 0},
    EventValueEntry{"Comet3Status", 0},
    EventValueEntry{"Comet4Status", 0},
    EventValueEntry{"Comet5Status", 0},
    EventValueEntry{"Comet6Status", 0},
};

template <typename Entry, std::size_t Size>
[[nodiscard]] const Entry& require_entry(const std::array<Entry, Size>& entries, std::string_view name,
                                         std::string_view table_name) {
    if (name.empty()) {
        throw std::invalid_argument(std::string(table_name) + " name must not be empty");
    }
    for (const auto& entry : entries) {
        if (entry.name == name) {
            return entry;
        }
    }
    throw std::invalid_argument(std::string(name) + " is absent from the retail " + std::string(table_name));
}
}  // namespace

const GameEventFlag& require_retail_flag(std::string_view name) {
    if (name.empty()) {
        throw std::invalid_argument("Game event flag name must not be empty");
    }
    for (auto index = s32{}; index < GameEventFlagTable::getTableSize(); ++index) {
        const auto* flag = GameEventFlagTable::getFlag(index);
        if (flag != nullptr && name == flag->mName) {
            return *flag;
        }
    }
    throw std::invalid_argument(std::string(name) + " is absent from the retail GameEventFlagTable");
}

const StoryEventEntry& require_retail_story_event(std::string_view name) {
    return require_entry(cStoryEvents, name, "StoryEvent BCSV");
}

const EventValueEntry& require_retail_event_value(std::string_view name) {
    return require_entry(cEventValues, name, "GameEventValue table");
}

}  // namespace smgpc::compat::game_data
