#pragma once

#include <span>
#include <string_view>

#include "resource/RarcArchive.hpp"

namespace smgpc::layout {

    // Layout archives name their primary resources after the requested layout.
    // A different BRLYT/BRLAN is not a substitute for a missing one.
    [[nodiscard]] const smgpc::resource::RarcEntry *find_layout_brlyt(
        std::span<const smgpc::resource::RarcEntry> entries, std::string_view layout_name);
    [[nodiscard]] const smgpc::resource::RarcEntry *find_layout_brlan(
        std::span<const smgpc::resource::RarcEntry> entries, std::string_view animation_name);

}  // namespace smgpc::layout
