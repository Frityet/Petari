#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class NameObj;
class JMapInfoIter;

namespace smgpc::runtime {

    class DvdFileSystemService;

}  // namespace smgpc::runtime

namespace smgpc::scene::nameobj {

    enum class NameObjCreatorSupportKind {
        Supported,
        RuntimeClosureUnavailable,
        NotLinked,
    };

    struct NameObjCreatorSupport {
        NameObjCreatorSupportKind kind = NameObjCreatorSupportKind::NotLinked;
        std::string reason;
    };

    enum class NameObjPlacementSupportKind {
        Unsupported,
        OriginalFactory,
        IntentionallyIgnored,
    };

    struct NameObjPlacementSupport {
        NameObjPlacementSupportKind kind = NameObjPlacementSupportKind::Unsupported;
        std::string reason;
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
        NameObjCreatorSupport creator_support;
        std::vector<NameObjArchiveRequest> archives;
    };

    [[nodiscard]] bool can_create_name_obj(std::string_view object_name);
    [[nodiscard]] NameObjCreatorSupport describe_name_obj_creator_support(std::string_view object_name);
    [[nodiscard]] NameObjPlacementSupport describe_name_obj_placement_support(smgpc::runtime::DvdFileSystemService &dvd,
                                                                              std::string_view object_name,
                                                                              std::string_view table_path);
    [[nodiscard]] NameObjFactoryDescription describe_name_obj_factory(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name);
    [[nodiscard]] std::vector<NameObjArchiveRequest> collect_name_obj_archive_requests(smgpc::runtime::DvdFileSystemService &dvd,
                                                                                       std::string_view object_name,
                                                                                       const JMapInfoIter *placement_iter = nullptr);
    std::vector<NameObjArchiveRequest> preload_name_obj_archives(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
                                                                 const JMapInfoIter *placement_iter = nullptr);
    // The object name selects the retail creator. The actor name independently mirrors
    // the nullable ObjNameTable display-name result passed to that creator.
    [[nodiscard]] std::unique_ptr<NameObj> create_name_obj(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
                                                           const char *actor_name);

}  // namespace smgpc::scene::nameobj
