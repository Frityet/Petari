#include "scene/StagePlacementResolver.hpp"

#include "resource/RarcArchive.hpp"
#include "runtime/NameObjFactoryCompat.hpp"
#include "runtime/RuntimeServices.hpp"

#include <filesystem>

namespace smgpc::compat {
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

        [[nodiscard]] std::optional<StagePlacementObject> read_supported_placement_object(const JMapInfo &info, s32 entry_index) {
            const auto iter = JMapInfoIter(&info, entry_index);
            const char *object_name = nullptr;
            if (!iter.getValue("name", &object_name) || object_name == nullptr || object_name[0] == '\0' || !can_create_name_obj(object_name)) {
                return std::nullopt;
            }

            auto l_id = s32{-1};
            (void)iter.getValue("l_id", &l_id);
            auto object = StagePlacementObject{
                .object_name = object_name,
                .type_name = "",
                .l_id = l_id,
                .jmap_info = info,
                .jmap_entry_index = entry_index,
            };
            read_object_args(object, iter);
            read_standard_placement_fields(object, iter);
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
        const auto info = JMapInfo::from_bcsv(archive.file_data(cCommonObjInfoPath));
        for (auto entry_index = s32{}; entry_index < info.getNumEntries(); ++entry_index) {
            if (auto object = read_supported_placement_object(info, entry_index)) {
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

}  // namespace smgpc::compat
