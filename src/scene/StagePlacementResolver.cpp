#include "scene/StagePlacementResolver.hpp"

#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "Game/Util/JMapUtil.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <limits>
#include <set>
#include <utility>

namespace smgpc::scene {
    namespace {

        constexpr auto cCommonLayerMask = u32{1U};
        constexpr auto cDegToRad = f32{3.14159265358979323846F / 180.0F};

        constexpr std::array<std::string_view, 17U> cLayerDirNames{
            "common",
            "layera",
            "layerb",
            "layerc",
            "layerd",
            "layere",
            "layerf",
            "layerg",
            "layerh",
            "layeri",
            "layerj",
            "layerk",
            "layerl",
            "layerm",
            "layern",
            "layero",
            "layerp",
        };

        [[nodiscard]] std::optional<std::filesystem::path> find_stage_archive(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name) {
            if (stage_name.empty()) {
                return std::nullopt;
            }

            const auto archive_name = std::string(stage_name) + ".arc";
            return dvd.find_first({
                std::filesystem::path("StageData") / archive_name,
            });
        }

        [[nodiscard]] std::optional<std::filesystem::path> find_scenario_archive(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name) {
            if (stage_name.empty()) {
                return std::nullopt;
            }

            const auto archive_name = std::string(stage_name) + "Scenario.arc";
            return dvd.find_first({
                std::filesystem::path("StageData") / std::string(stage_name) / archive_name,
            });
        }

        [[nodiscard]] std::string lower_copy(std::string_view value) {
            auto result = std::string(value);
            std::ranges::transform(result, result.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
            return result;
        }

        [[nodiscard]] std::optional<JMapInfo> load_zone_list(smgpc::runtime::DvdFileSystemService &dvd,
                                                             std::string_view scenario_stage_name) {
            const auto scenario_archive_path = find_scenario_archive(dvd, scenario_stage_name);
            if (!scenario_archive_path.has_value()) {
                return std::nullopt;
            }

            auto &scenario_archive = dvd.archive_for_path(*scenario_archive_path);
            if (!scenario_archive.contains_resource("/ZoneList.bcsv")) {
                return std::nullopt;
            }

            return JMapInfo::from_bcsv(scenario_archive.resource_data("/ZoneList.bcsv"));
        }

        [[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) {
            return value.size() >= prefix.size() && value.substr(0U, prefix.size()) == prefix;
        }

        [[nodiscard]] bool layer_is_active(std::string_view layer_name, u32 layer_mask) {
            const auto lower = lower_copy(layer_name);
            for (auto index = std::size_t{}; index < cLayerDirNames.size(); ++index) {
                if (lower == cLayerDirNames[index]) {
                    return (layer_mask & (1U << index)) != 0U;
                }
            }

            return false;
        }

        [[nodiscard]] s32 layer_id(std::string_view layer_name) {
            const auto lower = lower_copy(layer_name);
            for (auto index = std::size_t{}; index < cLayerDirNames.size(); ++index) {
                if (lower == cLayerDirNames[index]) {
                    return static_cast<s32>(index);
                }
            }

            return -1;
        }

        [[nodiscard]] std::string basename(std::string_view path) {
            const auto slash = path.find_last_of('/');
            return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1U));
        }

        [[nodiscard]] bool split_layered_jmp_path(std::string_view path, std::string_view root, std::string &layer_name) {
            const auto prefix = std::string("jmp/") + std::string(root) + "/";
            if (!starts_with(path, prefix)) {
                return false;
            }

            const auto rest = path.substr(prefix.size());
            const auto slash = rest.find('/');
            if (slash == std::string_view::npos || slash == 0U || slash + 1U >= rest.size()) {
                return false;
            }

            layer_name = lower_copy(rest.substr(0U, slash));
            return true;
        }

        [[nodiscard]] std::optional<std::string> layered_jmp_category(std::string_view path, std::string &layer_name) {
            constexpr std::array<std::string_view, 5U> categories{
                "placement",
                "mapparts",
                "start",
                "generalpos",
                "childobj",
            };

            for (const auto category : categories) {
                if (split_layered_jmp_path(path, category, layer_name)) {
                    return std::string(category);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::string> flat_jmp_category(std::string_view path) {
            if (starts_with(path, "jmp/list/")) {
                return "list";
            }
            if (starts_with(path, "jmp/path/")) {
                return "path";
            }
            if (starts_with(path, "camera/")) {
                return "camera";
            }
            return std::nullopt;
        }

        [[nodiscard]] std::array<f32, 3U> read_vec3_or(const JMapInfoIter &iter, std::string_view prefix,
                                                       std::array<f32, 3U> fallback) {
            const auto base = std::string(prefix);
            auto value = fallback;
            (void)iter.getValue((base + "_x").c_str(), &value[0]);
            (void)iter.getValue((base + "_y").c_str(), &value[1]);
            (void)iter.getValue((base + "_z").c_str(), &value[2]);
            return value;
        }

        [[nodiscard]] std::array<f32, 3U> transform_point(const StageZoneTransform &transform, const std::array<f32, 3U> &point) {
            return transform.transform_point(point);
        }

        [[nodiscard]] std::array<f32, 3U> rotation_degrees(const StageZoneTransform &transform) {
            constexpr auto cRadToDeg = f32{180.0F / 3.14159265358979323846F};
            const auto &m = transform.matrix;
            auto result = std::array<f32, 3U>{};
            if (m[8] >= 0.999F) {
                result[0] = std::atan2(-m[1], m[5]);
                result[1] = -0.5F * 3.14159265358979323846F;
            } else if (m[8] <= -0.999F) {
                result[0] = std::atan2(m[1], m[5]);
                result[1] = 0.5F * 3.14159265358979323846F;
            } else {
                result[0] = std::atan2(m[9], m[10]);
                result[1] = std::asin(-m[8]);
                result[2] = std::atan2(m[4], m[0]);
            }
            for (auto &angle : result) {
                angle *= cRadToDeg;
            }
            return result;
        }

        void apply_point_transform(JMapInfo &info, int entry_index, const StageZoneTransform &transform, std::string_view prefix) {
            const auto base = std::string(prefix);
            const auto iter = JMapInfoIter(&info, entry_index);
            auto local_point = std::array<f32, 3U>{};
            if (!iter.getValue((base + "_x").c_str(), &local_point[0]) || !iter.getValue((base + "_y").c_str(), &local_point[1]) ||
                !iter.getValue((base + "_z").c_str(), &local_point[2])) {
                return;
            }

            const auto world_point = transform_point(transform, local_point);
            info.setValue(entry_index, (base + "_x").c_str(), world_point[0]);
            info.setValue(entry_index, (base + "_y").c_str(), world_point[1]);
            info.setValue(entry_index, (base + "_z").c_str(), world_point[2]);
        }

        [[nodiscard]] StageZoneTransform compose_zone_transform(const StageZoneTransform &parent, const JMapInfoIter &zone_iter) {
            const auto local_translation = read_vec3_or(zone_iter, "pos", {0.0F, 0.0F, 0.0F});
            const auto local_rotation = read_vec3_or(zone_iter, "dir", {0.0F, 0.0F, 0.0F});
            return parent.concatenated(StageZoneTransform::from_translation_rotation(local_translation, local_rotation));
        }

        void apply_zone_transform(JMapInfo &info, const StageZoneTransform &transform) {
            for (auto entry_index = 0; entry_index < info.getNumEntries(); ++entry_index) {
                apply_point_transform(info, entry_index, transform, "pos");
                apply_point_transform(info, entry_index, transform, "pnt0");
                apply_point_transform(info, entry_index, transform, "pnt1");
                apply_point_transform(info, entry_index, transform, "pnt2");

                const auto iter = JMapInfoIter(&info, entry_index);
                auto local_rotation = std::array<f32, 3U>{};
                if (iter.getValue("dir_x", &local_rotation[0]) && iter.getValue("dir_y", &local_rotation[1]) &&
                    iter.getValue("dir_z", &local_rotation[2])) {
                    const auto world_rotation = transform.concatenated(
                        StageZoneTransform::from_translation_rotation({0.0F, 0.0F, 0.0F}, local_rotation));
                    const auto world_euler = rotation_degrees(world_rotation);
                    info.setValue(entry_index, "dir_x", world_euler[0]);
                    info.setValue(entry_index, "dir_y", world_euler[1]);
                    info.setValue(entry_index, "dir_z", world_euler[2]);
                }
            }
        }

        [[nodiscard]] const StagePlacementTable *find_path_table(const std::vector<StagePlacementTable> &tables,
                                                                 const StagePlacementTable &placement_table,
                                                                 std::string_view table_name) {
            const auto normalized_name = lower_copy(table_name);
            const auto found = std::ranges::find_if(tables, [&](const auto &candidate) {
                return candidate.holder_instance_id ==
                           placement_table.holder_instance_id &&
                       candidate.category == "path" &&
                       lower_copy(candidate.table_name) == normalized_name;
            });
            return found != tables.end() ? &*found : nullptr;
        }

        void attach_rail_info(std::vector<StagePlacementTable> &tables) {
            for (auto &table : tables) {
                if (table.jmap_info.searchItemInfo("CommonPath_ID") < 0) {
                    continue;
                }

                const auto *path_info_table = find_path_table(tables, table, "commonpathinfo");
                if (path_info_table == nullptr) {
                    continue;
                }

                for (auto entry_index = 0; entry_index < table.jmap_info.getNumEntries(); ++entry_index) {
                    auto common_path_id = s32{-1};
                    if (!table.jmap_info.getValue(entry_index, "CommonPath_ID", &common_path_id) || common_path_id < 0) {
                        continue;
                    }

                    const auto path_iter = path_info_table->jmap_info.findElement<s32>("l_id", common_path_id, 0);
                    if (!path_iter.isValid()) {
                        continue;
                    }

                    const auto point_table_name = "commonpathpointinfo." + std::to_string(path_iter.mIndex);
                    auto *point_info_table = find_path_table(tables, table, point_table_name);
                    if (point_info_table == nullptr) {
                        point_info_table = find_path_table(tables, table, "commonpathpointinfo");
                    }
                    if (point_info_table == nullptr || point_info_table->jmap_info.getNumEntries() == 0) {
                        continue;
                    }

                    auto point_info = point_info_table->jmap_info;
                    apply_zone_transform(point_info, point_info_table->zone_transform);
                    table.jmap_info.setRailInfo(entry_index, path_info_table->jmap_info, std::move(point_info), path_iter.mIndex);
                }
            }
        }

        [[nodiscard]] u32 resolve_stage_layer_mask(smgpc::runtime::DvdFileSystemService &dvd, std::string_view scenario_stage_name,
                                                   std::string_view layer_column_name, s32 scenario_no) {
            auto layer_mask = cCommonLayerMask;
            const auto scenario_archive_path = find_scenario_archive(dvd, scenario_stage_name);
            if (!scenario_archive_path.has_value()) {
                return layer_mask;
            }

            auto &scenario_archive = dvd.archive_for_path(*scenario_archive_path);
            if (!scenario_archive.contains_resource("/ScenarioData.bcsv")) {
                return layer_mask;
            }

            const auto scenario_info = JMapInfo::from_bcsv(scenario_archive.resource_data("/ScenarioData.bcsv"));
            const auto scenario_iter = scenario_info.findElement<s32>("ScenarioNo", scenario_no, 0);
            if (!scenario_iter.isValid()) {
                return layer_mask;
            }

            auto scenario_layers = u32{};
            if (scenario_iter.getValue(std::string(layer_column_name).c_str(), &scenario_layers)) {
                layer_mask |= scenario_layers * 2U;
            }

            return layer_mask;
        }

        [[nodiscard]] bool is_stage_obj_table(
            const StagePlacementTable &table) {
            return table.category == "placement" &&
                   lower_copy(table.table_name) == "stageobjinfo";
        }

        void load_stage_holder_tables(
            smgpc::runtime::DvdFileSystemService &dvd,
            std::string_view scenario_stage_name, s32 scenario_no,
            const StageHolderOccurrence &holder,
            std::vector<StagePlacementTable> &tables) {
            const auto stage_archive_path =
                find_stage_archive(dvd, holder.stage_name);
            if (!stage_archive_path.has_value()) {
                return;
            }

            auto &archive = dvd.archive_for_path(*stage_archive_path);
            const auto layer_mask = resolve_stage_layer_mask(
                dvd, scenario_stage_name, holder.stage_name, scenario_no);
            for (const auto &entry : archive.entries()) {
                auto layer_name = std::string{};
                auto category =
                    layered_jmp_category(entry.path, layer_name);
                if (category.has_value()) {
                    if (!layer_is_active(layer_name, layer_mask)) {
                        continue;
                    }
                } else {
                    category = flat_jmp_category(entry.path);
                    if (!category.has_value()) {
                        continue;
                    }
                    layer_name = "";
                }

                auto info = JMapInfo::from_bcsv(archive.file_data(entry));
                const auto table_name = basename(entry.path);
                const auto table_layer_id = layer_id(layer_name);
                const auto load_batch =
                    holder.discovery_batch ==
                            StagePlacementLoadBatch::CommonBootstrap &&
                        table_layer_id == 0
                        ? StagePlacementLoadBatch::CommonBootstrap
                        : StagePlacementLoadBatch::ScenarioSelected;
                info.setName(table_name.c_str());
                info.setPlacedZoneId(holder.zone_id);
                tables.push_back(StagePlacementTable{
                    .stage_name = holder.stage_name,
                    .zone_name = holder.stage_name,
                    .category = std::move(*category),
                    .layer_name = std::move(layer_name),
                    .table_name = table_name,
                    .archive_path = stage_archive_path->generic_string(),
                    .table_path = entry.path,
                    .jmap_info = std::move(info),
                    .zone_id = holder.zone_id,
                    .layer_id = table_layer_id,
                    .layer_mask = layer_mask,
                    .archive_entry_order = entry.file_entry_index,
                    .holder_instance_id = holder.instance_id,
                    .parent_holder_instance_id =
                        holder.parent_instance_id,
                    .holder_depth = holder.depth,
                    .holder_sibling_order = holder.sibling_order,
                    .holder_discovery_batch =
                        holder.discovery_batch,
                    .participates_in_root_placement = holder.depth <= 1U,
                    .load_batch = load_batch,
                    .placement_attachment_order =
                        std::numeric_limits<std::size_t>::max(),
                    .zone_transform = holder.zone_transform,
                });
            }
        }

        [[nodiscard]] std::vector<std::size_t>
        ordered_stage_obj_table_indices(
            std::span<const StagePlacementTable> tables,
            std::size_t holder_instance_id,
            StagePlacementLoadBatch load_batch) {
            auto result = std::vector<std::size_t>{};
            for (auto index = std::size_t{}; index < tables.size(); ++index) {
                const auto &table = tables[index];
                if (table.holder_instance_id == holder_instance_id &&
                    table.load_batch == load_batch &&
                    is_stage_obj_table(table)) {
                    result.push_back(index);
                }
            }
            std::ranges::stable_sort(
                result, [&](std::size_t left, std::size_t right) {
                    const auto &left_table = tables[left];
                    const auto &right_table = tables[right];
                    if (left_table.layer_id != right_table.layer_id) {
                        return left_table.layer_id < right_table.layer_id;
                    }
                    return left_table.archive_entry_order <
                           right_table.archive_entry_order;
                });
            return result;
        }

        [[nodiscard]] std::vector<StageHolderChildDescriptor>
        describe_stage_holder_children(
            const JMapInfo &zone_list,
            const StageHolderOccurrence &holder,
            StagePlacementLoadBatch load_batch,
            std::span<const StagePlacementTable> tables) {
            auto children = std::vector<StageHolderChildDescriptor>{};
            const auto table_indices = ordered_stage_obj_table_indices(
                tables, holder.instance_id, load_batch);
            for (const auto table_index : table_indices) {
                const auto entry_count =
                    tables[table_index].jmap_info.getNumEntries();
                for (auto entry_index = s32{}; entry_index < entry_count;
                     ++entry_index) {
                    const auto iter = JMapInfoIter(
                        &tables[table_index].jmap_info, entry_index);
                    const char *zone_name = "";
                    (void)MR::getObjectName(&zone_name, iter);
                    if (zone_name == nullptr || zone_name[0] == '\0') {
                        continue;
                    }
                    const auto zone_id =
                        find_stage_zone_id(zone_list, zone_name);
                    if (!zone_id.has_value()) {
                        continue;
                    }
                    children.push_back(StageHolderChildDescriptor{
                        .stage_name = zone_name,
                        .zone_id = *zone_id,
                        .zone_transform = compose_zone_transform(
                            holder.zone_transform, iter),
                    });
                }
            }
            return children;
        }

        void assign_placement_provenance_impl(
            std::vector<StagePlacementTable> &tables,
            const JMapInfo &zone_list) {
            (void)zone_list;

            for (const auto batch : {
                     StagePlacementLoadBatch::CommonBootstrap,
                     StagePlacementLoadBatch::ScenarioSelected}) {
                auto placement_tables =
                    std::vector<StagePlacementTable *>{};
                for (auto &table : tables) {
                    if (table.load_batch == batch &&
                        table.participates_in_root_placement &&
                        (table.category == "placement" ||
                         table.category == "mapparts")) {
                        placement_tables.push_back(&table);
                    }
                }
                std::ranges::stable_sort(
                    placement_tables, [](const auto *left,
                                         const auto *right) {
                        if (left->holder_depth != right->holder_depth) {
                            return left->holder_depth <
                                   right->holder_depth;
                        }
                        if (left->holder_sibling_order !=
                            right->holder_sibling_order) {
                            return left->holder_sibling_order <
                                   right->holder_sibling_order;
                        }
                        if (left->holder_instance_id !=
                            right->holder_instance_id) {
                            return left->holder_instance_id <
                                   right->holder_instance_id;
                        }
                        if (left->layer_id != right->layer_id) {
                            return left->layer_id < right->layer_id;
                        }
                        if (left->category != right->category) {
                            return left->category == "placement";
                        }
                        return left->archive_entry_order <
                               right->archive_entry_order;
                    });
                for (auto order = std::size_t{};
                     order < placement_tables.size(); ++order) {
                    placement_tables[order]->placement_attachment_order =
                        order;
                }
            }
        }

        void read_object_args(StagePlacementObject &object, const JMapInfoIter &iter) {
            for (auto arg_index = std::size_t{}; arg_index < object.object_args.size(); ++arg_index) {
                auto value = s32{-1};
                const auto field = "Obj_arg" + std::to_string(arg_index);
                (void)iter.getValue(field.c_str(), &value);
                object.object_args[arg_index] = value;
            }
        }

        void read_vec3(StagePlacementObject &object, const JMapInfoIter &iter, std::string_view prefix,
                       std::array<f32, 3U> StagePlacementObject::*field, bool StagePlacementObject::*has_field) {
            const auto base = std::string(prefix);
            auto x = f32{};
            auto y = f32{};
            auto z = f32{};
            if (!iter.getValue((base + "_x").c_str(), &x) || !iter.getValue((base + "_y").c_str(), &y) ||
                !iter.getValue((base + "_z").c_str(), &z)) {
                object.*has_field = false;
                return;
            }

            object.*field = std::array<f32, 3U>{x, y, z};
            object.*has_field = true;
        }

        void read_standard_placement_fields(StagePlacementObject &object, const JMapInfoIter &iter) {
            const char *type_name = "";
            if (iter.getValue("type", &type_name)) {
                object.type_name = type_name;
            }
            read_vec3(object, iter, "pos", &StagePlacementObject::translation, &StagePlacementObject::has_translation);
            read_vec3(object, iter, "dir", &StagePlacementObject::rotation, &StagePlacementObject::has_rotation);
            read_vec3(object, iter, "scale", &StagePlacementObject::scale, &StagePlacementObject::has_scale);
        }

        void read_s32_field(const JMapInfoIter &iter, const char *field_name, s32 &out) {
            (void)iter.getValue(field_name, &out);
        }

        void read_optional_ids(StagePlacementObject &object, const JMapInfoIter &iter) {
            read_s32_field(iter, "CameraSetId", object.camera_set_id);
            read_s32_field(iter, "Camera_id", object.camera_id);
            read_s32_field(iter, "CommonPath_ID", object.common_path_id);
            read_s32_field(iter, "Path_ID", object.path_id);
            read_s32_field(iter, "ParentID", object.parent_id);
            read_s32_field(iter, "FollowId", object.follow_id);
            read_s32_field(iter, "GroupId", object.group_id);
            read_s32_field(iter, "ClippingGroupId", object.clipping_group_id);
            read_s32_field(iter, "DemoGroupId", object.demo_group_id);
            read_s32_field(iter, "ViewGroupId", object.view_group_id);
            read_s32_field(iter, "MessageId", object.message_id);
            read_s32_field(iter, "CastId", object.cast_id);
            read_s32_field(iter, "ShapeModelNo", object.shape_model_no);
            read_s32_field(iter, "MapParts_ID", object.map_parts_id);
            read_s32_field(iter, "SW_APPEAR", object.switch_appear_id);
            read_s32_field(iter, "SW_DEAD", object.switch_dead_id);
            read_s32_field(iter, "SW_A", object.switch_a_id);
            read_s32_field(iter, "SW_B", object.switch_b_id);
            read_s32_field(iter, "SW_SLEEP", object.switch_sleep_id);
        }

        [[nodiscard]] std::optional<JMapInfo> find_child_info(
            std::span<const StagePlacementTable> tables,
            const StagePlacementTable &parent_table) {
            const auto child_table = std::ranges::find_if(tables, [&](const auto &table) {
                return table.holder_instance_id ==
                           parent_table.holder_instance_id &&
                       table.category == "childobj" &&
                       table.layer_name == parent_table.layer_name &&
                       table.table_name == "childobjinfo";
            });
            if (child_table == tables.end()) {
                return std::nullopt;
            }

            auto info = child_table->jmap_info;
            apply_zone_transform(info, parent_table.zone_transform);
            return info;
        }

        [[nodiscard]] s32 count_child_objects(const JMapInfo &info, const JMapInfoIter &parent_iter) {
            const auto *child_info = info.getChildObjInfo();
            if (child_info == nullptr) {
                return 0;
            }

            auto parent_id = s32{-1};
            if (!parent_iter.getValue("l_id", &parent_id) || parent_id < 0) {
                return 0;
            }

            auto count = s32{};
            for (auto entry_index = 0; entry_index < child_info->getNumEntries(); ++entry_index) {
                auto child_parent_id = s32{-1};
                if (child_info->getValue(entry_index, "ParentID", &child_parent_id) && child_parent_id == parent_id) {
                    ++count;
                }
            }
            return count;
        }

        [[nodiscard]] std::optional<StagePlacementObject> read_placement_object(
            smgpc::runtime::DvdFileSystemService &dvd,
            const StagePlacementTable &table,
            std::span<const StagePlacementTable> tables, s32 entry_index) {
            if (table.category != "placement" && table.category != "mapparts") {
                return std::nullopt;
            }

            auto info = table.jmap_info;
            apply_zone_transform(info, table.zone_transform);
            if (auto child_info = find_child_info(tables, table)) {
                info.setChildObjInfo(std::move(*child_info));
            }

            const auto iter = JMapInfoIter(&info, entry_index);
            const char *raw_name = "";
            (void)iter.getValue("name", &raw_name);
            const char *creator_identifier = "";
            // PlacementInfoOrdered initializes the destination to the shared
            // empty string and ignores getObjectName's return. A row missing
            // both fields therefore remains an empty-identifier SameIdSet;
            // it is not discarded from retail sorting evidence.
            (void)MR::getObjectName(&creator_identifier, iter);
            if (creator_identifier == nullptr) {
                creator_identifier = "";
            }

            auto l_id = s32{-1};
            (void)iter.getValue("l_id", &l_id);
            auto object = StagePlacementObject{
                .object_name = raw_name != nullptr ? raw_name : "",
                .type_name = "",
                .creator_identifier = creator_identifier,
                .stage_name = table.stage_name,
                .zone_name = table.zone_name,
                .category = table.category,
                .layer_name = table.layer_name,
                .table_name = table.table_name,
                .archive_path = table.archive_path,
                .table_path = table.table_path,
                .object_archive_path = "",
                .l_id = l_id,
                .zone_id = table.zone_id,
                .layer_id = table.layer_id,
                .holder_instance_id = table.holder_instance_id,
                .parent_holder_instance_id =
                    table.parent_holder_instance_id,
                .holder_depth = table.holder_depth,
                .load_batch = table.load_batch,
                .placement_attachment_order =
                    table.placement_attachment_order,
                .child_object_count = count_child_objects(info, iter),
                .jmap_info = info,
                .jmap_entry_index = entry_index,
            };
            read_object_args(object, iter);
            read_standard_placement_fields(object, iter);
            read_optional_ids(object, iter);
            const auto object_archive =
                dvd.find_object_archive(object.creator_identifier);
            object.object_archive_path = object_archive.has_value()
                                             ? object_archive->generic_string()
                                             : "";
            const auto support =
                smgpc::scene::nameobj::describe_name_obj_placement_support(
                    dvd, object.creator_identifier, table.table_path);
            object.factory_supported =
                support.kind == smgpc::scene::nameobj::
                                    NameObjPlacementSupportKind::OriginalFactory;
            object.intentionally_ignored =
                support.kind == smgpc::scene::nameobj::
                                    NameObjPlacementSupportKind::
                                        IntentionallyIgnored;
            object.support_reason = support.reason;
            object.support_kind = support.kind;
            return object;
        }

    }  // namespace

    std::vector<StageHolderOccurrence>
    discover_stage_holder_occurrences(
        std::string_view root_stage_name, s32 root_zone_id,
        const StageZoneTransform &root_transform,
        const StageHolderChildrenResolver &children_resolver,
        const StageHolderCreatedObserver &created_observer) {
        if (root_stage_name.empty() || !children_resolver) {
            return {};
        }

        auto holders = std::vector<StageHolderOccurrence>{};
        auto ancestries = std::vector<std::set<std::string>>{};
        const auto append_holder = [&holders, &ancestries,
                                    &created_observer](
                                       std::string stage_name, s32 zone_id,
                                       const StageZoneTransform &transform,
                                       std::optional<std::size_t> parent_id,
                                       StagePlacementLoadBatch batch,
                                       std::set<std::string> ancestry) {
            const auto instance_id = holders.size();
            const auto depth = parent_id.has_value()
                                   ? holders[*parent_id].depth + 1U
                                   : 0U;
            const auto sibling_order = parent_id.has_value()
                                           ? holders[*parent_id]
                                                 .children.size()
                                           : 0U;
            holders.push_back(StageHolderOccurrence{
                .instance_id = instance_id,
                .parent_instance_id = parent_id,
                .depth = depth,
                .sibling_order = sibling_order,
                .discovery_batch = batch,
                .stage_name = std::move(stage_name),
                .zone_id = zone_id,
                .zone_transform = transform,
            });
            ancestries.push_back(std::move(ancestry));
            if (parent_id.has_value()) {
                holders[*parent_id].children.push_back(instance_id);
            }
            if (created_observer) {
                created_observer(holders[instance_id]);
            }
            return instance_id;
        };

        auto root_ancestry =
            std::set<std::string>{lower_copy(root_stage_name)};
        const auto root_id = append_holder(
            std::string(root_stage_name), root_zone_id, root_transform,
            std::nullopt, StagePlacementLoadBatch::CommonBootstrap,
            std::move(root_ancestry));

        const auto append_child =
            [&](std::size_t parent_id, StagePlacementLoadBatch batch,
                const StageHolderChildDescriptor &descriptor)
                -> std::optional<std::size_t> {
            if (descriptor.stage_name.empty()) {
                return std::nullopt;
            }
            const auto stage_key = lower_copy(descriptor.stage_name);
            if (ancestries[parent_id].contains(stage_key)) {
                // Bound malformed data by the current holder path. No global
                // visited set may suppress a later sibling occurrence.
                return std::nullopt;
            }
            auto ancestry = ancestries[parent_id];
            ancestry.insert(stage_key);
            return append_holder(
                descriptor.stage_name, descriptor.zone_id,
                descriptor.zone_transform, parent_id, batch,
                std::move(ancestry));
        };

        const auto append_children =
            [&](std::size_t parent_id, StagePlacementLoadBatch batch) {
                auto child_ids = std::vector<std::size_t>{};
                for (const auto &descriptor :
                     children_resolver(holders[parent_id], batch)) {
                    if (const auto child_id =
                            append_child(parent_id, batch, descriptor)) {
                        child_ids.push_back(*child_id);
                    }
                }
                return child_ids;
            };

        auto discover_common = std::function<void(std::size_t)>{};
        discover_common = [&](std::size_t holder_id) {
            const auto descriptors = children_resolver(
                holders[holder_id],
                StagePlacementLoadBatch::CommonBootstrap);
            for (const auto &descriptor : descriptors) {
                const auto child_id = append_child(
                    holder_id,
                    StagePlacementLoadBatch::CommonBootstrap,
                    descriptor);
                if (!child_id.has_value()) {
                    continue;
                }
                // StageDataHolder::initWithoutIter initializes a new common
                // child immediately, yielding depth-first holder creation.
                discover_common(*child_id);
            }
        };
        discover_common(root_id);

        auto discover_scenario = std::function<void(std::size_t)>{};
        discover_scenario = [&](std::size_t holder_id) {
            // initAfterScenarioSelected appends this holder's entire scenario
            // child set, then recurses through common and scenario children.
            (void)append_children(
                holder_id, StagePlacementLoadBatch::ScenarioSelected);
            const auto children = holders[holder_id].children;
            for (const auto child_id : children) {
                discover_scenario(child_id);
            }
        };
        discover_scenario(root_id);

        auto next_traversal_order = std::size_t{};
        auto assign_traversal = std::function<void(std::size_t)>{};
        assign_traversal = [&](std::size_t holder_id) {
            holders[holder_id].traversal_order = next_traversal_order++;
            for (const auto child_id : holders[holder_id].children) {
                assign_traversal(child_id);
            }
        };
        assign_traversal(root_id);
        return holders;
    }

    std::optional<s32> find_stage_zone_id(const JMapInfo &zone_list,
                                          std::string_view zone_name) {
        if (zone_name.empty()) {
            return std::nullopt;
        }

        const auto expected_name = lower_copy(zone_name);
        for (auto zone_id = s32{}; zone_id < zone_list.getNumEntries(); ++zone_id) {
            const char *candidate_name = nullptr;
            if (zone_list.getValue(zone_id, "ZoneName", &candidate_name) &&
                candidate_name != nullptr && lower_copy(candidate_name) == expected_name) {
                return zone_id;
            }
        }

        return std::nullopt;
    }

    void assign_stage_placement_provenance(
        std::vector<StagePlacementTable> &tables,
        const JMapInfo &zone_list) {
        assign_placement_provenance_impl(tables, zone_list);
    }

    StageZoneTransform StageZoneTransform::from_translation_rotation(const std::array<f32, 3U> &translation,
                                                                     const std::array<f32, 3U> &rotation_degrees) {
        const auto rx = rotation_degrees[0] * cDegToRad;
        const auto ry = rotation_degrees[1] * cDegToRad;
        const auto rz = rotation_degrees[2] * cDegToRad;
        const auto sx = std::sin(rx);
        const auto cx = std::cos(rx);
        const auto sy = std::sin(ry);
        const auto cy = std::cos(ry);
        const auto sz = std::sin(rz);
        const auto cz = std::cos(rz);

        auto result = StageZoneTransform{};
        result.matrix = {
            cz * cy,
            (cz * sy * sx) - (sz * cx),
            (cz * sy * cx) + (sz * sx),
            translation[0],
            sz * cy,
            (sz * sy * sx) + (cz * cx),
            (sz * sy * cx) - (cz * sx),
            translation[1],
            -sy,
            cy * sx,
            cy * cx,
            translation[2],
        };
        return result;
    }

    StageZoneTransform StageZoneTransform::concatenated(const StageZoneTransform &local) const {
        auto result = StageZoneTransform{};
        for (auto row = std::size_t{}; row < 3U; ++row) {
            for (auto column = std::size_t{}; column < 3U; ++column) {
                result.matrix[row * 4U + column] = matrix[row * 4U + 0U] * local.matrix[column + 0U] +
                                                   matrix[row * 4U + 1U] * local.matrix[column + 4U] +
                                                   matrix[row * 4U + 2U] * local.matrix[column + 8U];
            }
            result.matrix[row * 4U + 3U] = matrix[row * 4U + 0U] * local.matrix[3U] +
                                           matrix[row * 4U + 1U] * local.matrix[7U] +
                                           matrix[row * 4U + 2U] * local.matrix[11U] + matrix[row * 4U + 3U];
        }
        return result;
    }

    std::array<f32, 3U> StageZoneTransform::transform_point(const std::array<f32, 3U> &point) const {
        const auto rotated = transform_vector(point);
        return {
            rotated[0] + matrix[3U],
            rotated[1] + matrix[7U],
            rotated[2] + matrix[11U],
        };
    }

    std::array<f32, 3U> StageZoneTransform::transform_vector(const std::array<f32, 3U> &vector) const {
        return {
            matrix[0U] * vector[0] + matrix[1U] * vector[1] + matrix[2U] * vector[2],
            matrix[4U] * vector[0] + matrix[5U] * vector[1] + matrix[6U] * vector[2],
            matrix[8U] * vector[0] + matrix[9U] * vector[1] + matrix[10U] * vector[2],
        };
    }

    std::optional<StageStartInfo> select_stage_start_info(std::span<const StagePlacementTable> tables, s32 start_id,
                                                          s32 start_zone_id) {
        auto start_tables = std::vector<const StagePlacementTable *>{};
        for (const auto &table : tables) {
            if (table.category == "start" && table.zone_id == start_zone_id && table.layer_id >= 0) {
                start_tables.push_back(&table);
            }
        }
        std::ranges::stable_sort(start_tables, [](const auto *lhs, const auto *rhs) {
            if (lhs->holder_traversal_order !=
                rhs->holder_traversal_order) {
                return lhs->holder_traversal_order <
                       rhs->holder_traversal_order;
            }
            if (lhs->layer_id != rhs->layer_id) {
                return lhs->layer_id < rhs->layer_id;
            }
            return lhs->archive_entry_order < rhs->archive_entry_order;
        });

        for (const auto *table : start_tables) {
            for (auto entry_index = s32{}; entry_index < table->jmap_info.getNumEntries(); ++entry_index) {
                const auto iter = JMapInfoIter(&table->jmap_info, entry_index);
                auto mario_no = s32{-1};
                if (!iter.getValue("MarioNo", &mario_no) || mario_no != start_id) {
                    continue;
                }

                const char *object_name = nullptr;
                (void)MR::getObjectName(&object_name, iter);

                const auto local_position = read_vec3_or(iter, "pos", {0.0F, 0.0F, 0.0F});
                const auto local_rotation = read_vec3_or(iter, "dir", {0.0F, 0.0F, 0.0F});
                const auto world_rotation = table->zone_transform.concatenated(
                    StageZoneTransform::from_translation_rotation({0.0F, 0.0F, 0.0F}, local_rotation));
                auto camera_id = s32{-1};
                (void)iter.getValue("Camera_id", &camera_id);
                auto start_jmap_info = table->jmap_info;
                apply_zone_transform(start_jmap_info, table->zone_transform);
                start_jmap_info.setName(table->table_name.c_str());
                start_jmap_info.setPlacedZoneId(table->zone_id);

                return StageStartInfo{
                    .object_name = object_name != nullptr ? object_name : "",
                    .stage_name = table->stage_name,
                    .zone_name = table->zone_name,
                    .layer_name = table->layer_name,
                    .archive_path = table->archive_path,
                    .table_path = table->table_path,
                    .start_id = start_id,
                    .zone_id = start_zone_id,
                    .camera_id = camera_id,
                    .jmap_entry_index = entry_index,
                    .local_position = local_position,
                    .local_rotation = local_rotation,
                    .world_position = table->zone_transform.transform_point(local_position),
                    .world_side = {world_rotation.matrix[0U], world_rotation.matrix[4U], world_rotation.matrix[8U]},
                    .world_up = {world_rotation.matrix[1U], world_rotation.matrix[5U], world_rotation.matrix[9U]},
                    .world_front = {world_rotation.matrix[2U], world_rotation.matrix[6U], world_rotation.matrix[10U]},
                    .zone_transform = table->zone_transform,
                    .jmap_info = std::move(start_jmap_info),
                };
            }
        }
        return std::nullopt;
    }

    std::vector<StageGeneralPos> select_stage_general_positions(
        std::span<const StagePlacementTable> tables) {
        auto general_pos_tables = std::vector<const StagePlacementTable *>{};
        for (const auto &table : tables) {
            if (table.category == "generalpos" && table.layer_id >= 0) {
                general_pos_tables.push_back(&table);
            }
        }
        std::ranges::stable_sort(general_pos_tables, [](const auto *lhs, const auto *rhs) {
            if (lhs->holder_traversal_order !=
                rhs->holder_traversal_order) {
                return lhs->holder_traversal_order <
                       rhs->holder_traversal_order;
            }
            if (lhs->layer_id != rhs->layer_id) {
                return lhs->layer_id < rhs->layer_id;
            }
            return lhs->archive_entry_order < rhs->archive_entry_order;
        });

        auto positions = std::vector<StageGeneralPos>{};
        for (const auto *table : general_pos_tables) {
            for (auto entry_index = s32{};
                 entry_index < table->jmap_info.getNumEntries(); ++entry_index) {
                const auto iter = JMapInfoIter(&table->jmap_info, entry_index);
                const char *raw_name = nullptr;
                if (!iter.getValue("PosName", &raw_name) || raw_name == nullptr || raw_name[0] == '\0') {
                    continue;
                }
                const auto local_position = read_vec3_or(iter, "pos", {0.0F, 0.0F, 0.0F});
                const auto local_rotation = read_vec3_or(iter, "dir", {0.0F, 0.0F, 0.0F});
                const auto world_transform = table->zone_transform.concatenated(
                    StageZoneTransform::from_translation_rotation(
                        {0.0F, 0.0F, 0.0F}, local_rotation));
                positions.push_back(StageGeneralPos{
                    .name = smgpc::resource::decode_cp932(raw_name),
                    .stage_name = table->stage_name,
                    .zone_name = table->zone_name,
                    .layer_name = table->layer_name,
                    .table_path = table->table_path,
                    .zone_id = table->zone_id,
                    .jmap_entry_index = entry_index,
                    .local_position = local_position,
                    .local_rotation = local_rotation,
                    .world_position = table->zone_transform.transform_point(local_position),
                    .world_rotation = rotation_degrees(world_transform),
                });
            }
        }
        return positions;
    }

    std::vector<StagePlacementTable> resolve_stage_placement_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no,
                                                                  std::vector<StageHolderOccurrence>* retained_holders) {
        if (retained_holders != nullptr) {
            retained_holders->clear();
        }
        auto tables = std::vector<StagePlacementTable>{};
        auto zone_list = load_zone_list(dvd, stage_name);
        if (!zone_list.has_value()) {
            return tables;
        }

        const auto root_zone_id = find_stage_zone_id(*zone_list, stage_name);
        // Retail constructs the top-level StageDataHolder with zone ID 0. A
        // scenario whose row 0 is not the requested stage has no valid root.
        if (root_zone_id != std::optional<s32>{0}) {
            return tables;
        }

        auto holders = discover_stage_holder_occurrences(
            stage_name, *root_zone_id, StageZoneTransform{},
            [&](const StageHolderOccurrence &holder,
                StagePlacementLoadBatch batch) {
                return describe_stage_holder_children(
                    *zone_list, holder, batch, tables);
            },
            [&](const StageHolderOccurrence &holder) {
                load_stage_holder_tables(
                    dvd, stage_name, scenario_no, holder, tables);
            });
        for (auto &table : tables) {
            table.holder_traversal_order =
                holders.at(table.holder_instance_id).traversal_order;
        }
        assign_stage_placement_provenance(tables, *zone_list);
        attach_rail_info(tables);
        if (retained_holders != nullptr) {
            *retained_holders = std::move(holders);
        }
        return tables;
    }

    std::vector<StagePlacementObject> resolve_stage_placement_objects(
        smgpc::runtime::DvdFileSystemService &dvd,
        std::span<const StagePlacementTable> tables) {
        auto objects = std::vector<StagePlacementObject>{};
        for (const auto &table : tables) {
            if (!table.participates_in_root_placement) {
                continue;
            }
            for (auto entry_index = s32{}; entry_index < table.jmap_info.getNumEntries(); ++entry_index) {
                if (auto object = read_placement_object(dvd, table, tables, entry_index)) {
                    objects.push_back(std::move(*object));
                }
            }
        }

        return objects;
    }

    std::vector<StagePlacementObject> resolve_stage_placement_objects(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
        s32 scenario_no) {
        const auto tables = resolve_stage_placement_tables(dvd, stage_name, scenario_no);
        return resolve_stage_placement_objects(dvd, tables);
    }

    std::vector<StagePlacementObject> resolve_stage_root_placements(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto objects = std::vector<StagePlacementObject>{};
        for (auto &object : resolve_stage_placement_objects(dvd, stage_name, scenario_no)) {
            if (object.factory_supported) {
                objects.push_back(std::move(object));
            }
        }

        return objects;
    }

    std::optional<StagePlacementObject> resolve_stage_root_placement(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto objects = resolve_stage_root_placements(dvd, stage_name, scenario_no);
        if (objects.empty()) {
            return std::nullopt;
        }

        return std::move(objects.front());
    }

    std::optional<StageStartInfo> resolve_stage_start_info(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name,
                                                           s32 scenario_no, s32 start_id, s32 start_zone_id) {
        const auto tables = resolve_stage_placement_tables(dvd, stage_name, scenario_no);
        return select_stage_start_info(tables, start_id, start_zone_id);
    }

}  // namespace smgpc::scene
