#include "scene/StagePlacementResolver.hpp"

#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
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

        [[nodiscard]] std::array<f32, 3U> rotate_euler_xyz(const std::array<f32, 3U> &value, const std::array<f32, 3U> &rotation) {
            const auto rx = rotation[0] * cDegToRad;
            const auto ry = rotation[1] * cDegToRad;
            const auto rz = rotation[2] * cDegToRad;
            const auto sx = std::sin(rx);
            const auto cx = std::cos(rx);
            const auto sy = std::sin(ry);
            const auto cy = std::cos(ry);
            const auto sz = std::sin(rz);
            const auto cz = std::cos(rz);

            const auto r00 = cz * cy;
            const auto r01 = (cz * sy * sx) - (sz * cx);
            const auto r02 = (cz * sy * cx) + (sz * sx);
            const auto r10 = sz * cy;
            const auto r11 = (sz * sy * sx) + (cz * cx);
            const auto r12 = (sz * sy * cx) - (cz * sx);
            const auto r20 = -sy;
            const auto r21 = cy * sx;
            const auto r22 = cy * cx;

            return std::array<f32, 3U>{
                (r00 * value[0]) + (r01 * value[1]) + (r02 * value[2]),
                (r10 * value[0]) + (r11 * value[1]) + (r12 * value[2]),
                (r20 * value[0]) + (r21 * value[1]) + (r22 * value[2]),
            };
        }

        [[nodiscard]] std::array<f32, 3U> transform_point(const StageZoneTransform &transform, const std::array<f32, 3U> &point) {
            const auto scaled = std::array<f32, 3U>{
                point[0] * transform.scale[0],
                point[1] * transform.scale[1],
                point[2] * transform.scale[2],
            };
            const auto rotated = rotate_euler_xyz(scaled, transform.rotation);
            return std::array<f32, 3U>{
                rotated[0] + transform.translation[0],
                rotated[1] + transform.translation[1],
                rotated[2] + transform.translation[2],
            };
        }

        [[nodiscard]] StageZoneTransform compose_zone_transform(const StageZoneTransform &parent, const JMapInfoIter &zone_iter) {
            const auto local_translation = read_vec3_or(zone_iter, "pos", {0.0F, 0.0F, 0.0F});
            const auto local_rotation = read_vec3_or(zone_iter, "dir", {0.0F, 0.0F, 0.0F});
            const auto local_scale = read_vec3_or(zone_iter, "scale", {1.0F, 1.0F, 1.0F});
            return StageZoneTransform{
                .translation = transform_point(parent, local_translation),
                .rotation =
                    {
                        parent.rotation[0] + local_rotation[0],
                        parent.rotation[1] + local_rotation[1],
                        parent.rotation[2] + local_rotation[2],
                    },
                .scale =
                    {
                        parent.scale[0] * local_scale[0],
                        parent.scale[1] * local_scale[1],
                        parent.scale[2] * local_scale[2],
                    },
            };
        }

        void apply_zone_transform(JMapInfo &info, const StageZoneTransform &transform) {
            for (auto entry_index = 0; entry_index < info.getNumEntries(); ++entry_index) {
                const auto iter = JMapInfoIter(&info, entry_index);
                auto local_translation = std::array<f32, 3U>{};
                if (iter.getValue("pos_x", &local_translation[0]) && iter.getValue("pos_y", &local_translation[1]) &&
                    iter.getValue("pos_z", &local_translation[2])) {
                    const auto world_translation = transform_point(transform, local_translation);
                    info.setValue(entry_index, "pos_x", world_translation[0]);
                    info.setValue(entry_index, "pos_y", world_translation[1]);
                    info.setValue(entry_index, "pos_z", world_translation[2]);
                }

                auto local_rotation = std::array<f32, 3U>{};
                if (iter.getValue("dir_x", &local_rotation[0]) && iter.getValue("dir_y", &local_rotation[1]) &&
                    iter.getValue("dir_z", &local_rotation[2])) {
                    info.setValue(entry_index, "dir_x", transform.rotation[0] + local_rotation[0]);
                    info.setValue(entry_index, "dir_y", transform.rotation[1] + local_rotation[1]);
                    info.setValue(entry_index, "dir_z", transform.rotation[2] + local_rotation[2]);
                }

                auto local_scale = std::array<f32, 3U>{};
                if (iter.getValue("scale_x", &local_scale[0]) && iter.getValue("scale_y", &local_scale[1]) &&
                    iter.getValue("scale_z", &local_scale[2])) {
                    info.setValue(entry_index, "scale_x", transform.scale[0] * local_scale[0]);
                    info.setValue(entry_index, "scale_y", transform.scale[1] * local_scale[1]);
                    info.setValue(entry_index, "scale_z", transform.scale[2] * local_scale[2]);
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

        void collect_stage_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, std::string_view scenario_stage_name,
                                  s32 scenario_no, s32 zone_id, const StageZoneTransform &zone_transform, std::set<std::string> &visited,
                                  std::vector<StagePlacementTable> &tables);

        void collect_placed_zones(smgpc::runtime::DvdFileSystemService &dvd, const StagePlacementTable &table, std::string_view scenario_stage_name,
                                  s32 scenario_no, std::set<std::string> &visited, std::vector<StagePlacementTable> &tables) {
            if (table.category != "placement" || table.table_name != "stageobjinfo") {
                return;
            }

            struct ZoneRecord {
                std::string name;
                s32 id = 0;
                StageZoneTransform transform{};
            };

            auto zone_records = std::vector<ZoneRecord>{};
            for (auto entry_index = s32{}; entry_index < table.jmap_info.getNumEntries(); ++entry_index) {
                const auto iter = JMapInfoIter(&table.jmap_info, entry_index);
                const char *zone_name = nullptr;
                if (!iter.getValue("name", &zone_name) || zone_name == nullptr || zone_name[0] == '\0') {
                    continue;
                }

                zone_records.push_back(ZoneRecord{
                    .name = zone_name,
                    .id = entry_index + 1,
                    .transform = compose_zone_transform(table.zone_transform, iter),
                });
            }

            for (const auto &zone : zone_records) {
                collect_stage_tables(dvd, zone.name, scenario_stage_name, scenario_no, zone.id, zone.transform, visited, tables);
            }
        }

        void collect_stage_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, std::string_view scenario_stage_name,
                                  s32 scenario_no, s32 zone_id, const StageZoneTransform &zone_transform, std::set<std::string> &visited,
                                  std::vector<StagePlacementTable> &tables) {
            const auto stage_key = lower_copy(stage_name);
            if (!visited.insert(stage_key).second) {
                return;
            }

            const auto stage_archive_path = find_stage_archive(dvd, stage_name);
            if (!stage_archive_path.has_value()) {
                return;
            }

            auto &archive = dvd.archive_for_path(*stage_archive_path);
            const auto layer_mask = resolve_stage_layer_mask(dvd, scenario_stage_name, stage_name, scenario_no);
            const auto first_new_table = tables.size();
            for (const auto &entry : archive.entries()) {
                auto layer_name = std::string{};
                auto category = layered_jmp_category(entry.path, layer_name);
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
                info.setName(table_name.c_str());
                tables.push_back(StagePlacementTable{
                    .stage_name = std::string(stage_name),
                    .zone_name = std::string(stage_name),
                    .category = std::move(*category),
                    .layer_name = std::move(layer_name),
                    .table_name = table_name,
                    .archive_path = stage_archive_path->generic_string(),
                    .table_path = entry.path,
                    .jmap_info = std::move(info),
                    .zone_id = zone_id,
                    .layer_id = table_layer_id,
                    .layer_mask = layer_mask,
                    .zone_transform = zone_transform,
                });
            }

            for (auto table_index = first_new_table; table_index < tables.size(); ++table_index) {
                collect_placed_zones(dvd, tables[table_index], scenario_stage_name, scenario_no, visited, tables);
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

        [[nodiscard]] std::optional<JMapInfo> find_child_info(const std::vector<StagePlacementTable> &tables, const StagePlacementTable &parent_table) {
            const auto child_table = std::ranges::find_if(tables, [&](const auto &table) {
                return table.stage_name == parent_table.stage_name && table.category == "childobj" && table.layer_name == parent_table.layer_name &&
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

        [[nodiscard]] std::optional<StagePlacementObject> read_placement_object(smgpc::runtime::DvdFileSystemService &dvd, const StagePlacementTable &table,
                                                                                const std::vector<StagePlacementTable> &tables, s32 entry_index) {
            if (table.category != "placement" && table.category != "mapparts") {
                return std::nullopt;
            }

            auto info = table.jmap_info;
            apply_zone_transform(info, table.zone_transform);
            if (auto child_info = find_child_info(tables, table)) {
                info.setChildObjInfo(std::move(*child_info));
            }

            const auto iter = JMapInfoIter(&info, entry_index);
            const char *object_name = nullptr;
            if (!iter.getValue("name", &object_name) || object_name == nullptr || object_name[0] == '\0') {
                return std::nullopt;
            }

            auto l_id = s32{-1};
            (void)iter.getValue("l_id", &l_id);
            const auto object_archive = dvd.find_object_archive(object_name);
            const auto support = smgpc::scene::nameobj::describe_name_obj_placement_support(dvd, object_name, table.table_path);
            auto object = StagePlacementObject{
                .object_name = object_name,
                .type_name = "",
                .stage_name = table.stage_name,
                .zone_name = table.zone_name,
                .category = table.category,
                .layer_name = table.layer_name,
                .table_name = table.table_name,
                .archive_path = table.archive_path,
                .table_path = table.table_path,
                .object_archive_path = object_archive.has_value() ? object_archive->generic_string() : "",
                .model_archive_name = support.model_archive_name,
                .l_id = l_id,
                .zone_id = table.zone_id,
                .layer_id = table.layer_id,
                .child_object_count = count_child_objects(info, iter),
                .jmap_info = info,
                .jmap_entry_index = entry_index,
                .factory_supported = support.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::OriginalFactory,
                .model_fallback_supported = support.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::GenericModel,
                .alias_model_fallback_supported = support.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::GenericAliasModel,
                .intentionally_ignored = support.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::IntentionallyIgnored,
                .support_reason = support.reason,
                .support_kind = support.kind,
            };
            read_object_args(object, iter);
            read_standard_placement_fields(object, iter);
            read_optional_ids(object, iter);
            return object;
        }

    }  // namespace

    std::vector<StagePlacementTable> resolve_stage_placement_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto tables = std::vector<StagePlacementTable>{};
        auto visited = std::set<std::string>{};
        collect_stage_tables(dvd, stage_name, stage_name, scenario_no, 0, StageZoneTransform{}, visited, tables);
        return tables;
    }

    std::vector<StagePlacementObject> resolve_stage_placement_objects(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto objects = std::vector<StagePlacementObject>{};
        const auto tables = resolve_stage_placement_tables(dvd, stage_name, scenario_no);
        for (const auto &table : tables) {
            for (auto entry_index = s32{}; entry_index < table.jmap_info.getNumEntries(); ++entry_index) {
                if (auto object = read_placement_object(dvd, table, tables, entry_index)) {
                    objects.push_back(std::move(*object));
                }
            }
        }

        return objects;
    }

    std::vector<StagePlacementObject> resolve_stage_root_placements(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto objects = std::vector<StagePlacementObject>{};
        for (auto &object : resolve_stage_placement_objects(dvd, stage_name, scenario_no)) {
            if (object.factory_supported || object.model_fallback_supported || object.alias_model_fallback_supported) {
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

}  // namespace smgpc::scene
