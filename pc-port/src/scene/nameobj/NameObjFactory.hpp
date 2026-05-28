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
    [[nodiscard]] NameObjFactoryDescription describe_name_obj_factory(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name);
    [[nodiscard]] std::vector<NameObjArchiveRequest> collect_name_obj_archive_requests(smgpc::runtime::DvdFileSystemService &dvd,
                                                                                        std::string_view object_name);
    std::vector<NameObjArchiveRequest> preload_name_obj_archives(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name);
    [[nodiscard]] std::unique_ptr<NameObj> create_name_obj(std::string_view object_name, std::string_view actor_name);

}  // namespace smgpc::scene::nameobj
