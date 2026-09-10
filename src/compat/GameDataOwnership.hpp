#pragma once

#include "Game/System/GameDataPlayerStatus.hpp"
#include "Game/System/GameDataHolder.hpp"

namespace smgpc::compat::game_data {

// Initialize the actual original process singleton before entering a shorter
// lived profile heap. Its immutable table must outlive every selected file.
void initialize_event_table();

// Retire original typed children before their owning JKR domain is released.
// This function does not own, copy, or substitute any gameplay state.
void destroy_holder(GameDataHolder& holder);

inline u8 holder_story_progress(const GameDataHolder& holder) {
    return holder.mPlayerStatus->mStoryProgress;
}

} // namespace smgpc::compat::game_data
