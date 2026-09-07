#pragma once

#include <cstdint>
#include <string_view>

#include <revolution/types.h>

struct GameEventFlag;

namespace smgpc::compat::game_data {

struct StoryEventEntry {
    std::string_view name;
    u8 progress;
};

struct EventValueEntry {
    std::string_view name;
    u16 default_value;
};

[[nodiscard]] const GameEventFlag& require_retail_flag(std::string_view name);
[[nodiscard]] const StoryEventEntry& require_retail_story_event(std::string_view name);
[[nodiscard]] const EventValueEntry& require_retail_event_value(std::string_view name);

}  // namespace smgpc::compat::game_data
