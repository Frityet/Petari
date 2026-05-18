#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <revolution.h>

namespace smgpc::game {

    class DvdFileSystemService;

    struct StagePlacementObject {
        std::string object_name;
        std::string type_name;
        s32 l_id = -1;
        std::array<s32, 8U> object_args{};
        std::array<f32, 3U> translation{};
        std::array<f32, 3U> rotation{};
        std::array<f32, 3U> scale{1.0F, 1.0F, 1.0F};
        bool has_translation = false;
        bool has_rotation = false;
        bool has_scale = false;
    };

    [[nodiscard]] std::vector<StagePlacementObject> resolve_stage_root_placements(DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                  s32 scenario_no);
    [[nodiscard]] std::optional<StagePlacementObject> resolve_stage_root_placement(DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                   s32 scenario_no);

}  // namespace smgpc::game
