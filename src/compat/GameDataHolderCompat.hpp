#pragma once

#include <cstddef>
#include <map>
#include <string>

#include <revolution/types.h>

class GameDataHolder;

namespace smgpc::compat::game_data {

[[nodiscard]] std::size_t holder_state_count() noexcept;
void destroy_holder_state(const GameDataHolder& holder);
void copy_holder_state(GameDataHolder& destination, const GameDataHolder& source);
void set_holder_name(GameDataHolder& holder, const char* name);
void set_holder_save_counts(GameDataHolder& holder, s32 power_star_num, s32 star_piece_num, s32 player_miss_num);
void set_holder_ending_flags(GameDataHolder& holder, bool view_normal_ending, bool view_complete_ending,
                             bool final_challenge_star);
void set_holder_event_state(GameDataHolder& holder, const std::map<std::string, bool>& flags,
                            const std::map<std::string, u16>& values);
[[nodiscard]] const std::map<std::string, bool>& holder_event_flags(const GameDataHolder& holder);
[[nodiscard]] const std::map<std::string, u16>& holder_event_values(const GameDataHolder& holder);
[[nodiscard]] u8 holder_story_progress(const GameDataHolder& holder);
void set_holder_story_progress(GameDataHolder& holder, u8 progress);

}  // namespace smgpc::compat::game_data
