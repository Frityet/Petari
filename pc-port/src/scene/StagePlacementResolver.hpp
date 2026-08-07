#pragma once

#include <array>
#include <optional>
#include <span>
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
        // Row-major rigid 3x4 placement matrix. This mirrors the original
        // StageDataHolder makeMtxTR transform: zone scale is not part of the
        // placement matrix.
        std::array<f32, 12U> matrix{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
        };

        [[nodiscard]] static StageZoneTransform from_translation_rotation(const std::array<f32, 3U> &translation,
                                                                           const std::array<f32, 3U> &rotation_degrees);
        [[nodiscard]] StageZoneTransform concatenated(const StageZoneTransform &local) const;
        [[nodiscard]] std::array<f32, 3U> transform_point(const std::array<f32, 3U> &point) const;
        [[nodiscard]] std::array<f32, 3U> transform_vector(const std::array<f32, 3U> &vector) const;
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
        u32 archive_entry_order = 0U;
        StageZoneTransform zone_transform{};
    };

    struct StageStartInfo {
        std::string stage_name;
        std::string zone_name;
        std::string layer_name;
        std::string archive_path;
        std::string table_path;
        s32 start_id = 0;
        s32 zone_id = 0;
        s32 camera_id = -1;
        s32 jmap_entry_index = -1;
        std::array<f32, 3U> local_position{};
        std::array<f32, 3U> local_rotation{};
        std::array<f32, 3U> world_position{};
        std::array<f32, 3U> world_side{1.0F, 0.0F, 0.0F};
        std::array<f32, 3U> world_up{0.0F, 1.0F, 0.0F};
        std::array<f32, 3U> world_front{0.0F, 0.0F, 1.0F};
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
    [[nodiscard]] std::optional<StageStartInfo> select_stage_start_info(std::span<const StagePlacementTable> tables, s32 start_id = 0,
                                                                        s32 start_zone_id = 0);
    [[nodiscard]] std::optional<StageStartInfo> resolve_stage_start_info(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                         s32 scenario_no, s32 start_id = 0, s32 start_zone_id = 0);

}  // namespace smgpc::scene
