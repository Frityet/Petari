#include "scene/nameobj/NameObjFactory.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace smgpc::scene::nameobj {
    namespace {

        struct OriginalArchiveRecord {
            std::string_view object_name;
            std::string_view archive_name;
        };

        constexpr OriginalArchiveRecord cOriginalArchiveRecords[] = {
#include "NameObjArchiveTable.inc"
        };

        [[nodiscard]] std::filesystem::path disc_relative(const std::filesystem::path &root, const std::filesystem::path &path) {
            std::error_code error{};
            auto relative = std::filesystem::relative(path, root, error);
            if (!error && !relative.empty()) {
                return relative;
            }
            return path.filename();
        }

        [[nodiscard]] NameObjArchiveRequest describe_archive(smgpc::runtime::DvdFileSystemService &dvd, std::string_view archive_name) {
            auto request = NameObjArchiveRequest{
                .archive_name = std::string(archive_name),
                .disc_path = "",
                .resolved_path = "",
                .kind = NameObjArchiveKind::Missing,
                .loaded = false,
            };

            auto archive_path = dvd.find_object_archive(archive_name);
            if (archive_path.has_value()) {
                request.kind = NameObjArchiveKind::Object;
            } else {
                archive_path = dvd.find_layout_archive(archive_name);
                if (archive_path.has_value()) {
                    request.kind = NameObjArchiveKind::Layout;
                }
            }

            if (!archive_path.has_value()) {
                return request;
            }

            request.resolved_path = archive_path->generic_string();
            request.disc_path = disc_relative(dvd.root(), *archive_path).generic_string();
            request.loaded = dvd.archive_load_count_for_path(*archive_path) > 0U;
            return request;
        }

        void add_archive_request(std::vector<NameObjArchiveRequest> &requests, NameObjArchiveRequest request) {
            const auto duplicate = std::ranges::find_if(requests, [&](const auto &existing) {
                return existing.archive_name == request.archive_name && existing.kind == request.kind && existing.resolved_path == request.resolved_path;
            });
            if (duplicate == requests.end()) {
                requests.push_back(std::move(request));
            }
        }

    }  // namespace

    bool can_create_name_obj(std::string_view object_name) {
        const auto name = std::string(object_name);
        return NameObjFactory::canCreate(name.c_str());
    }

    std::vector<NameObjArchiveRequest> collect_name_obj_archive_requests(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name) {
        auto requests = std::vector<NameObjArchiveRequest>{};
        for (const auto &record : cOriginalArchiveRecords) {
            if (record.object_name == object_name) {
                add_archive_request(requests, describe_archive(dvd, record.archive_name));
            }
        }
        return requests;
    }

    NameObjFactoryDescription describe_name_obj_factory(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name) {
        return NameObjFactoryDescription{
            .object_name = std::string(object_name),
            .creator_supported = can_create_name_obj(object_name),
            .archives = collect_name_obj_archive_requests(dvd, object_name),
        };
    }

    std::vector<NameObjArchiveRequest> preload_name_obj_archives(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name) {
        auto requests = collect_name_obj_archive_requests(dvd, object_name);
        for (auto &request : requests) {
            if (request.kind == NameObjArchiveKind::Missing || request.resolved_path.empty()) {
                continue;
            }
            auto &archive = dvd.archive_for_path(request.resolved_path);
            (void)archive;
            request.loaded = true;
        }
        return requests;
    }

    std::unique_ptr<NameObj> create_name_obj(std::string_view object_name, std::string_view actor_name) {
        const auto object = std::string(object_name);
        const auto creator = NameObjFactory::getCreator(object.c_str());
        if (creator == nullptr) {
            throw std::runtime_error("Unsupported NameObj factory request: " + std::string(object_name));
        }

        const auto name = std::string(actor_name);
        return std::unique_ptr<NameObj>(creator(name.c_str()));
    }

}  // namespace smgpc::scene::nameobj
