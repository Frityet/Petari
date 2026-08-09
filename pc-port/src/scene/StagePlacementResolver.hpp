#pragma once

#include <array>
#include <functional>
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

    // StageDataHolder retains common-layer tables loaded during its initial
    // bootstrap separately from every table discovered after scenario
    // selection. A common-layer table in a scenario-discovered child zone is
    // therefore ScenarioSelected, not CommonBootstrap.
    enum class StagePlacementLoadBatch {
        CommonBootstrap,
        ScenarioSelected,
    };

    struct StageZoneTransform {
        // Row-major rigid 3x4 placement matrix. This mirrors the original
        // StageDataHolder makeMtxTR transform: zone scale is not part of the
        // placement matrix.
        std::array<f32, 12U> matrix{
            1.0F,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            1.0F,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            1.0F,
            0.0F,
        };

        [[nodiscard]] static StageZoneTransform from_translation_rotation(const std::array<f32, 3U> &translation,
                                                                          const std::array<f32, 3U> &rotation_degrees);
        [[nodiscard]] StageZoneTransform concatenated(const StageZoneTransform &local) const;
        [[nodiscard]] std::array<f32, 3U> transform_point(const std::array<f32, 3U> &point) const;
        [[nodiscard]] std::array<f32, 3U> transform_vector(const std::array<f32, 3U> &vector) const;
    };

    // Pure holder-occurrence discovery seam shared by the live archive
    // resolver and focused synthetic graph tests. Child descriptors are
    // returned in StageObj table/row order. The traversal preserves repeated
    // sibling zones and rejects cycles only along the current ancestry path.
    struct StageHolderChildDescriptor {
        std::string stage_name;
        s32 zone_id = 0;
        StageZoneTransform zone_transform{};
    };

    struct StageHolderOccurrence {
        std::size_t instance_id = 0U;
        std::optional<std::size_t> parent_instance_id{};
        std::size_t depth = 0U;
        std::size_t sibling_order = 0U;
        std::size_t traversal_order = 0U;
        StagePlacementLoadBatch discovery_batch =
            StagePlacementLoadBatch::CommonBootstrap;
        std::string stage_name;
        s32 zone_id = 0;
        StageZoneTransform zone_transform{};
        std::vector<std::size_t> children{};
    };

    using StageHolderChildrenResolver = std::function<
        std::vector<StageHolderChildDescriptor>(
            const StageHolderOccurrence &, StagePlacementLoadBatch)>;
    using StageHolderCreatedObserver =
        std::function<void(const StageHolderOccurrence &)>;

    [[nodiscard]] std::vector<StageHolderOccurrence>
    discover_stage_holder_occurrences(
        std::string_view root_stage_name, s32 root_zone_id,
        const StageZoneTransform &root_transform,
        const StageHolderChildrenResolver &children_resolver,
        const StageHolderCreatedObserver &created_observer = {});

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
        // A StageObj row creates a distinct StageDataHolder even when another
        // row names the same zone. These fields retain occurrence identity and
        // ancestry instead of collapsing ownership by ZoneList id.
        std::size_t holder_instance_id = 0U;
        std::optional<std::size_t> parent_holder_instance_id{};
        std::size_t holder_depth = 0U;
        std::size_t holder_sibling_order = 0U;
        std::size_t holder_traversal_order = 0U;
        StagePlacementLoadBatch holder_discovery_batch =
            StagePlacementLoadBatch::CommonBootstrap;
        // Root PlacementInfoOrdered attaches only the root holder and its
        // immediate mStageDataArray children. Deeper holder tables remain for
        // recursive start/general/rail evidence, not actor construction.
        bool participates_in_root_placement = true;
        StagePlacementLoadBatch load_batch =
            StagePlacementLoadBatch::ScenarioSelected;
        std::size_t placement_attachment_order = 0U;
        StageZoneTransform zone_transform{};
    };

    struct StageStartInfo {
        std::string object_name;
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
        JMapInfo jmap_info;

        [[nodiscard]] JMapInfoIter iter() const & {
            return JMapInfoIter(&jmap_info, jmap_entry_index);
        }

        [[nodiscard]] JMapInfoIter iter() const && = delete;
    };

    struct StageGeneralPos {
        std::string name;
        std::string stage_name;
        std::string zone_name;
        std::string layer_name;
        std::string table_path;
        s32 zone_id = 0;
        s32 jmap_entry_index = -1;
        std::array<f32, 3U> local_position{};
        std::array<f32, 3U> local_rotation{};
        std::array<f32, 3U> world_position{};
        std::array<f32, 3U> world_rotation{};
    };

    struct StagePlacementObject {
        // Raw authored `name` evidence. Creation and SameIdSet identity use
        // creator_identifier, which is MR::getObjectName's exact `type`-field
        // preference (including an authored empty type value).
        std::string object_name;
        std::string type_name;
        std::string creator_identifier;
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
        std::size_t holder_instance_id = 0U;
        std::optional<std::size_t> parent_holder_instance_id{};
        std::size_t holder_depth = 0U;
        StagePlacementLoadBatch load_batch =
            StagePlacementLoadBatch::ScenarioSelected;
        std::size_t placement_attachment_order = 0U;
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

    [[nodiscard]] std::optional<s32> find_stage_zone_id(const JMapInfo &zone_list,
                                                        std::string_view zone_name);
    // Retains the two StageDataHolder load batches and exact table-array
    // attachment ordinals used by PlacementInfoOrdered. Normal callers receive
    // this through resolve_stage_placement_tables; focused fixtures use it to
    // prove table attachment policy after occurrence discovery.
    void assign_stage_placement_provenance(
        std::vector<StagePlacementTable> &tables,
        const JMapInfo &zone_list);
    [[nodiscard]] std::vector<StagePlacementTable> resolve_stage_placement_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                  s32 scenario_no);
    [[nodiscard]] std::vector<StagePlacementObject> resolve_stage_placement_objects(
        smgpc::runtime::DvdFileSystemService &dvd,
        std::span<const StagePlacementTable> tables);
    [[nodiscard]] std::vector<StagePlacementObject> resolve_stage_placement_objects(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                    s32 scenario_no);
    [[nodiscard]] std::vector<StagePlacementObject> resolve_stage_root_placements(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                  s32 scenario_no);
    [[nodiscard]] std::optional<StagePlacementObject> resolve_stage_root_placement(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                                   s32 scenario_no);
    [[nodiscard]] std::optional<StageStartInfo> select_stage_start_info(std::span<const StagePlacementTable> tables, s32 start_id = 0,
                                                                        s32 start_zone_id = 0);
    [[nodiscard]] std::vector<StageGeneralPos> select_stage_general_positions(
        std::span<const StagePlacementTable> tables);
    [[nodiscard]] std::optional<StageStartInfo> resolve_stage_start_info(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                                         s32 scenario_no, s32 start_id = 0, s32 start_zone_id = 0);

}  // namespace smgpc::scene
