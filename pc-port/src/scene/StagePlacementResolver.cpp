#include "scene/StagePlacementResolver.hpp"

#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>

namespace smgpc::scene {
    namespace {

        constexpr auto cCommonLayerMask = u32 {1U};

        constexpr std::array<std::string_view, 17U> cLayerDirNames {
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
            for (auto index = std::size_t {}; index < cLayerDirNames.size(); ++index) {
                if (lower == cLayerDirNames[index]) {
                    return (layer_mask & (1U << index)) != 0U;
                }
            }

            return false;
        }

        [[nodiscard]] s32 layer_id(std::string_view layer_name) {
            const auto lower = lower_copy(layer_name);
            for (auto index = std::size_t {}; index < cLayerDirNames.size(); ++index) {
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
            constexpr std::array<std::string_view, 5U> categories {
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

            auto scenario_layers = u32 {};
            if (scenario_iter.getValue(std::string(layer_column_name).c_str(), &scenario_layers)) {
                layer_mask |= scenario_layers * 2U;
            }

            return layer_mask;
        }

        void collect_stage_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, std::string_view scenario_stage_name,
                                  s32 scenario_no, s32 zone_id, std::set<std::string> &visited, std::vector<StagePlacementTable> &tables);

        void collect_placed_zones(smgpc::runtime::DvdFileSystemService &dvd, const StagePlacementTable &table, std::string_view scenario_stage_name,
                                  s32 scenario_no, std::set<std::string> &visited, std::vector<StagePlacementTable> &tables) {
            if (table.category != "placement" || table.table_name != "stageobjinfo") {
                return;
            }

            for (auto entry_index = s32 {}; entry_index < table.jmap_info.getNumEntries(); ++entry_index) {
                const auto iter = JMapInfoIter(&table.jmap_info, entry_index);
                const char *zone_name = nullptr;
                if (!iter.getValue("name", &zone_name) || zone_name == nullptr || zone_name[0] == '\0') {
                    continue;
                }

                collect_stage_tables(dvd, zone_name, scenario_stage_name, scenario_no, entry_index + 1, visited, tables);
            }
        }

        void collect_stage_tables(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, std::string_view scenario_stage_name,
                                  s32 scenario_no, s32 zone_id, std::set<std::string> &visited, std::vector<StagePlacementTable> &tables) {
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
                auto layer_name = std::string {};
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
                tables.push_back(StagePlacementTable {
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
                });
            }

            for (auto table_index = first_new_table; table_index < tables.size(); ++table_index) {
                collect_placed_zones(dvd, tables[table_index], scenario_stage_name, scenario_no, visited, tables);
            }
        }

        void read_object_args(StagePlacementObject &object, const JMapInfoIter &iter) {
            for (auto arg_index = std::size_t {}; arg_index < object.object_args.size(); ++arg_index) {
                auto value = s32 {-1};
                const auto field = "Obj_arg" + std::to_string(arg_index);
                (void)iter.getValue(field.c_str(), &value);
                object.object_args[arg_index] = value;
            }
        }

        void read_vec3(StagePlacementObject &object, const JMapInfoIter &iter, std::string_view prefix,
                       std::array<f32, 3U> StagePlacementObject::*field, bool StagePlacementObject::*has_field) {
            const auto base = std::string(prefix);
            auto x = f32 {};
            auto y = f32 {};
            auto z = f32 {};
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

        [[nodiscard]] std::optional<StagePlacementObject> read_placement_object(smgpc::runtime::DvdFileSystemService &dvd, const StagePlacementTable &table,
                                                                                s32 entry_index) {
            if (table.category != "placement" && table.category != "mapparts") {
                return std::nullopt;
            }

            const auto &info = table.jmap_info;
            const auto iter = JMapInfoIter(&info, entry_index);
            const char *object_name = nullptr;
            if (!iter.getValue("name", &object_name) || object_name == nullptr || object_name[0] == '\0') {
                return std::nullopt;
            }

            auto l_id = s32 {-1};
            (void)iter.getValue("l_id", &l_id);
            const auto object_archive = dvd.find_object_archive(object_name);
            auto object = StagePlacementObject {
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
                .jmap_info = info,
                .jmap_entry_index = entry_index,
                .factory_supported = smgpc::scene::nameobj::can_create_name_obj(object_name),
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
        collect_stage_tables(dvd, stage_name, stage_name, scenario_no, 0, visited, tables);
        return tables;
    }

    std::vector<StagePlacementObject> resolve_stage_placement_objects(smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto objects = std::vector<StagePlacementObject>{};
        for (const auto &table : resolve_stage_placement_tables(dvd, stage_name, scenario_no)) {
            for (auto entry_index = s32 {}; entry_index < table.jmap_info.getNumEntries(); ++entry_index) {
                if (auto object = read_placement_object(dvd, table, entry_index)) {
                    objects.push_back(std::move(*object));
                }
            }
        }

        return objects;
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

}  // namespace smgpc::scene
