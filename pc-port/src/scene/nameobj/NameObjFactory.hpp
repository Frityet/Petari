#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class NameObj;

namespace smgpc::runtime {

    class DvdFileSystemService;

}  // namespace smgpc::runtime

namespace smgpc::scene::nameobj {

    enum class NameObjPlacementSupportKind {
        Unsupported,
        OriginalFactory,
        GenericModel,
        GenericAliasModel,
        IntentionallyIgnored,
    };

    struct NameObjPlacementSupport {
        NameObjPlacementSupportKind kind = NameObjPlacementSupportKind::Unsupported;
        std::string reason;
        std::string model_archive_name;
    };

    enum class NameObjArchiveKind {
        Object,
        Layout,
        Missing,
    };

    struct NameObjArchiveRequest {
        std::string archive_name;
        std::string disc_path;
        std::string resolved_path;
        NameObjArchiveKind kind = NameObjArchiveKind::Missing;
        bool loaded = false;
    };

    struct NameObjFactoryDescription {
        std::string object_name;
        bool creator_supported = false;
        std::vector<NameObjArchiveRequest> archives;
    };

    [[nodiscard]] bool can_create_name_obj(std::string_view object_name);
    [[nodiscard]] bool can_create_name_obj(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name);
    [[nodiscard]] NameObjPlacementSupport describe_name_obj_placement_support(smgpc::runtime::DvdFileSystemService &dvd,
                                                                              std::string_view object_name,
                                                                              std::string_view table_path);
    [[nodiscard]] NameObjFactoryDescription describe_name_obj_factory(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name);
    [[nodiscard]] std::vector<NameObjArchiveRequest> collect_name_obj_archive_requests(smgpc::runtime::DvdFileSystemService &dvd,
                                                                                       std::string_view object_name);
    std::vector<NameObjArchiveRequest> preload_name_obj_archives(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name);
    [[nodiscard]] std::unique_ptr<NameObj> create_name_obj(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
                                                           std::string_view actor_name);

}  // namespace smgpc::scene::nameobj
