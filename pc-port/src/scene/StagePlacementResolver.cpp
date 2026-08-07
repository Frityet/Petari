#include "scene/StagePlacementResolver.hpp"

#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <map>
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

        [[nodiscard]] std::optional<s32> find_zone_id(const JMapInfo *zone_list, std::string_view zone_name) {
            if (zone_list == nullptr || zone_name.empty()) {
                return std::nullopt;
            }

            const auto expected_name = lower_copy(zone_name);
            for (auto zone_id = s32{}; zone_id < zone_list->getNumEntries(); ++zone_id) {
                const char *candidate_name = nullptr;
                if (zone_list->getValue(zone_id, "ZoneName", &candidate_name) && candidate_name != nullptr &&
                    lower_copy(candidate_name) == expected_name) {
                    return zone_id;
                }
            }

            return std::nullopt;
        }

        class StageZoneIdResolver {
        public:
            explicit StageZoneIdResolver(std::optional<JMapInfo> zone_list)
                : _zone_list(std::move(zone_list)), _next_fallback_id(_zone_list.has_value() ? _zone_list->getNumEntries() : 1) {
            }

            [[nodiscard]] s32 resolve(std::string_view zone_name) {
                if (const auto canonical_id = find_zone_id(_zone_list.has_value() ? &*_zone_list : nullptr, zone_name)) {
                    return *canonical_id;
                }

                const auto normalized_name = lower_copy(zone_name);
                const auto found = _fallback_ids.find(normalized_name);
                if (found != _fallback_ids.end()) {
                    return found->second;
                }

                const auto fallback_id = _next_fallback_id++;
                _fallback_ids.emplace(normalized_name, fallback_id);
                return fallback_id;
            }

        private:
            std::optional<JMapInfo> _zone_list;
            std::map<std::string, s32, std::less<>> _fallback_ids;
            s32 _next_fallback_id = 1;
        };

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
                return candidate.stage_name == placement_table.stage_name && candidate.category == "path" &&
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

        void collect_stage_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, std::string_view scenario_stage_name,
                                  s32 scenario_no, s32 zone_id, const StageZoneTransform &zone_transform, StageZoneIdResolver &zone_ids,
                                  std::set<std::string> &visited, std::vector<StagePlacementTable> &tables);

        void collect_placed_zones(smgpc::runtime::DvdFileSystemService &dvd, const StagePlacementTable &table, std::string_view scenario_stage_name,
                                  s32 scenario_no, StageZoneIdResolver &zone_ids, std::set<std::string> &visited,
                                  std::vector<StagePlacementTable> &tables) {
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
                    .id = zone_ids.resolve(zone_name),
                    .transform = compose_zone_transform(table.zone_transform, iter),
                });
            }

            for (const auto &zone : zone_records) {
                collect_stage_tables(dvd, zone.name, scenario_stage_name, scenario_no, zone.id, zone.transform, zone_ids, visited, tables);
            }
        }

        void collect_stage_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, std::string_view scenario_stage_name,
                                  s32 scenario_no, s32 zone_id, const StageZoneTransform &zone_transform, StageZoneIdResolver &zone_ids,
                                  std::set<std::string> &visited, std::vector<StagePlacementTable> &tables) {
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
                info.setPlacedZoneId(zone_id);
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
                    .archive_entry_order = entry.file_entry_index,
                    .zone_transform = zone_transform,
                });
            }

            for (auto table_index = first_new_table; table_index < tables.size(); ++table_index) {
                collect_placed_zones(dvd, tables[table_index], scenario_stage_name, scenario_no, zone_ids, visited, tables);
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
                .l_id = l_id,
                .zone_id = table.zone_id,
                .layer_id = table.layer_id,
                .child_object_count = count_child_objects(info, iter),
                .jmap_info = info,
                .jmap_entry_index = entry_index,
                .factory_supported = support.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::OriginalFactory,
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

                const auto local_position = read_vec3_or(iter, "pos", {0.0F, 0.0F, 0.0F});
                const auto local_rotation = read_vec3_or(iter, "dir", {0.0F, 0.0F, 0.0F});
                const auto world_rotation = table->zone_transform.concatenated(
                    StageZoneTransform::from_translation_rotation({0.0F, 0.0F, 0.0F}, local_rotation));
                auto camera_id = s32{-1};
                (void)iter.getValue("Camera_id", &camera_id);

                return StageStartInfo{
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
                };
            }
        }
        return std::nullopt;
    }

    std::vector<StageGeneralPos> select_stage_general_positions(
        std::span<const StagePlacementTable> tables) {
        auto general_pos_tables = std::vector<const StagePlacementTable *>{};
        auto zone_order = std::vector<s32>{};
        for (const auto &table : tables) {
            if (std::ranges::find(zone_order, table.zone_id) == zone_order.end()) {
                zone_order.push_back(table.zone_id);
            }
            if (table.category == "generalpos" && table.layer_id >= 0) {
                general_pos_tables.push_back(&table);
            }
        }
        std::ranges::stable_sort(general_pos_tables, [&](const auto *lhs, const auto *rhs) {
            const auto lhs_zone = std::ranges::find(zone_order, lhs->zone_id);
            const auto rhs_zone = std::ranges::find(zone_order, rhs->zone_id);
            if (lhs_zone != rhs_zone) {
                return lhs_zone < rhs_zone;
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

    std::vector<StagePlacementTable> resolve_stage_placement_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto tables = std::vector<StagePlacementTable>{};
        auto visited = std::set<std::string>{};
        auto zone_ids = StageZoneIdResolver(load_zone_list(dvd, stage_name));
        collect_stage_tables(dvd, stage_name, stage_name, scenario_no, 0, StageZoneTransform{}, zone_ids, visited, tables);
        attach_rail_info(tables);
        return tables;
    }

    std::vector<StagePlacementObject> resolve_stage_placement_objects(
        smgpc::runtime::DvdFileSystemService &dvd,
        std::span<const StagePlacementTable> tables) {
        auto objects = std::vector<StagePlacementObject>{};
        for (const auto &table : tables) {
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
