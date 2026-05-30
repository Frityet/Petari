#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <revolution.h>

#include "Game/Util/JMapInfo.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

namespace smgpc::runtime {
    class DvdFileSystemService;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    struct StageZoneTransform {
        std::array<f32, 3U> translation{0.0F, 0.0F, 0.0F};
        std::array<f32, 3U> rotation{0.0F, 0.0F, 0.0F};
        std::array<f32, 3U> scale{1.0F, 1.0F, 1.0F};
    };

    struct StagePlacementTable {
        std::string stage_name;
        std::string zone_name;
        std::string category;
        std::string layer_name;
        std::string table_name;
        std::string archive_path;
        std::string table_path;
        JMapInfo jmap_info;
        s32 zone_id = 0;
        s32 layer_id = -1;
        u32 layer_mask = 0U;
        StageZoneTransform zone_transform{};
    };

    struct StagePlacementObject {
        std::string object_name;
        std::string type_name;
        std::string stage_name;
        std::string zone_name;
        std::string category;
        std::string layer_name;
        std::string table_name;
        std::string archive_path;
        std::string table_path;
        std::string object_archive_path;
        std::string model_archive_name;
        s32 l_id = -1;
        s32 zone_id = 0;
        s32 layer_id = -1;
        s32 child_object_count = 0;
        s32 camera_set_id = -1;
        s32 camera_id = -1;
        s32 common_path_id = -1;
        s32 path_id = -1;
        s32 parent_id = -1;
        s32 follow_id = -1;
        s32 group_id = -1;
        s32 clipping_group_id = -1;
        s32 demo_group_id = -1;
        s32 view_group_id = -1;
        s32 message_id = -1;
        s32 cast_id = -1;
        s32 shape_model_no = -1;
        s32 map_parts_id = -1;
        s32 switch_appear_id = -1;
        s32 switch_dead_id = -1;
        s32 switch_a_id = -1;
        s32 switch_b_id = -1;
        s32 switch_sleep_id = -1;
        std::array<s32, 8U> object_args{};
        std::array<f32, 3U> translation{};
        std::array<f32, 3U> rotation{};
        std::array<f32, 3U> scale{1.0F, 1.0F, 1.0F};
        JMapInfo jmap_info;
        s32 jmap_entry_index = -1;
        bool has_translation = false;
        bool has_rotation = false;
        bool has_scale = false;
        bool factory_supported = false;
        bool model_fallback_supported = false;
        bool alias_model_fallback_supported = false;
        bool intentionally_ignored = false;
        std::string support_reason;
        smgpc::scene::nameobj::NameObjPlacementSupportKind support_kind = smgpc::scene::nameobj::NameObjPlacementSupportKind::Unsupported;
    };

    [[nodiscard]] std::vector<StagePlacementTable> resolve_stage_placement_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                  s32 scenario_no);
    [[nodiscard]] std::vector<StagePlacementObject> resolve_stage_placement_objects(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                    s32 scenario_no);
    [[nodiscard]] std::vector<StagePlacementObject> resolve_stage_root_placements(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                  s32 scenario_no);
    [[nodiscard]] std::optional<StagePlacementObject> resolve_stage_root_placement(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                   s32 scenario_no);

}  // namespace smgpc::scene
