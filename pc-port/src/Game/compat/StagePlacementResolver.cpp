#include "Game/compat/StagePlacementResolver.hpp"

#include "Game/compat/BcsvTable.hpp"
#include "Game/compat/NameObjFactoryCompat.hpp"
#include "Game/compat/RarcArchive.hpp"
#include "Game/compat/RuntimeServices.hpp"

#include <filesystem>

namespace smgpc::game {
    namespace {

        constexpr auto cCommonObjInfoPath = std::string_view{"jmp/placement/common/objinfo"};

        [[nodiscard]] std::optional<std::filesystem::path> find_stage_archive(DvdFileSystemService &dvd, std::string_view stage_name) {
            if (stage_name.empty()) {
                return std::nullopt;
            }

            const auto archive_name = std::string(stage_name) + ".arc";
            return dvd.find_first({
                std::filesystem::path("StageData") / archive_name,
            });
        }

        void read_object_args(StagePlacementObject &object, const BcsvTable &table, std::size_t entry_index) {
            for (auto arg_index = std::size_t{}; arg_index < object.object_args.size(); ++arg_index) {
                const auto value = table.get_s32(entry_index, "Obj_arg" + std::to_string(arg_index));
                object.object_args[arg_index] = value.value_or(-1);
            }
        }

        void read_vec3(StagePlacementObject &object, const BcsvTable &table, std::size_t entry_index, std::string_view prefix,
                       std::array<f32, 3U> StagePlacementObject::*field, bool StagePlacementObject::*has_field) {
            const auto base = std::string(prefix);
            const auto x = table.get_float(entry_index, base + "_x");
            const auto y = table.get_float(entry_index, base + "_y");
            const auto z = table.get_float(entry_index, base + "_z");
            if (!x.has_value() || !y.has_value() || !z.has_value()) {
                object.*has_field = false;
                return;
            }

            object.*field = std::array<f32, 3U>{*x, *y, *z};
            object.*has_field = true;
        }

        void read_standard_placement_fields(StagePlacementObject &object, const BcsvTable &table, std::size_t entry_index) {
            object.type_name = table.get_string(entry_index, "type").value_or("");
            read_vec3(object, table, entry_index, "pos", &StagePlacementObject::translation, &StagePlacementObject::has_translation);
            read_vec3(object, table, entry_index, "dir", &StagePlacementObject::rotation, &StagePlacementObject::has_rotation);
            read_vec3(object, table, entry_index, "scale", &StagePlacementObject::scale, &StagePlacementObject::has_scale);
        }

        [[nodiscard]] std::optional<StagePlacementObject> read_supported_placement_object(const BcsvTable &table, std::size_t entry_index) {
            const auto object_name = table.get_string(entry_index, "name");
            if (!object_name.has_value() || object_name->empty() || !can_create_name_obj(*object_name)) {
                return std::nullopt;
            }

            auto object = StagePlacementObject{
                .object_name = *object_name,
                .type_name = "",
                .l_id = table.get_s32(entry_index, "l_id").value_or(-1),
            };
            read_object_args(object, table, entry_index);
            read_standard_placement_fields(object, table, entry_index);
            return object;
        }

    }  // namespace

    std::vector<StagePlacementObject> resolve_stage_root_placements(DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        (void)scenario_no;

        const auto stage_archive_path = find_stage_archive(dvd, stage_name);
        if (!stage_archive_path.has_value()) {
            return {};
        }

        auto &archive = dvd.archive_for_path(*stage_archive_path);
        if (!archive.contains(cCommonObjInfoPath)) {
            return {};
        }

        auto objects = std::vector<StagePlacementObject>{};
        const auto table = BcsvTable::from_bytes(archive.file_data(cCommonObjInfoPath));
        for (auto entry_index = std::size_t{}; entry_index < table.entry_count(); ++entry_index) {
            if (auto object = read_supported_placement_object(table, entry_index)) {
                objects.push_back(std::move(*object));
            }
        }

        return objects;
    }

    std::optional<StagePlacementObject> resolve_stage_root_placement(DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no) {
        auto objects = resolve_stage_root_placements(dvd, stage_name, scenario_no);
        if (objects.empty()) {
            return std::nullopt;
        }

        return std::move(objects.front());
    }

}  // namespace smgpc::game
