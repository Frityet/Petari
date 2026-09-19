#pragma once

#include "scene/StagePlacementResolver.hpp"

#include <span>
#include <string_view>

namespace smgpc::scene {
    // Inspect retained authored rows before constructing any actors.
    void preflight_stage_placements_or_throw(
        std::string_view stage_name, s32 scenario_no,
        std::span<const StagePlacementObject> placements);
}  // namespace smgpc::scene
