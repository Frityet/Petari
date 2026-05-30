#include "scene/nameobj/NameObjFactory.hpp"

#include "Game/LiveActor/ModelObj.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/SceneFunction.hpp"
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

        [[nodiscard]] bool direct_object_archive_exists(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name) {
            return !object_name.empty() && dvd.find_object_archive(object_name).has_value();
        }

        [[nodiscard]] std::string_view original_archive_model_name(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name) {
            for (const auto &record : cOriginalArchiveRecords) {
                if (record.object_name == object_name && dvd.find_object_archive(record.archive_name).has_value()) {
                    return record.archive_name;
                }
            }
            return {};
        }

        [[nodiscard]] bool contains_path_component(std::string_view path, std::string_view component) {
            return path.find(component) != std::string_view::npos;
        }

        [[nodiscard]] bool is_intentionally_ignored_placement_table(std::string_view object_name, std::string_view table_path) {
            if (contains_path_component(table_path, "/areaobjinfo") && can_create_name_obj(object_name)) {
                return false;
            }

            return contains_path_component(table_path, "/stageobjinfo") || contains_path_component(table_path, "/areaobjinfo") ||
                   contains_path_component(table_path, "/cameracubeinfo") || contains_path_component(table_path, "/planetobjinfo") ||
                   contains_path_component(table_path, "/demoobjinfo");
        }

    }  // namespace

    bool can_create_name_obj(std::string_view object_name) {
        const auto name = std::string(object_name);
        return NameObjFactory::canCreate(name.c_str());
    }

    bool can_create_name_obj(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name) {
        return can_create_name_obj(object_name) || direct_object_archive_exists(dvd, object_name) ||
               !original_archive_model_name(dvd, object_name).empty();
    }

    NameObjPlacementSupport describe_name_obj_placement_support(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
                                                                std::string_view table_path) {
        if (is_intentionally_ignored_placement_table(object_name, table_path)) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::IntentionallyIgnored,
                .reason = "non_renderable_placement_helper_table",
                .model_archive_name = "",
            };
        }

        if (can_create_name_obj(object_name)) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::OriginalFactory,
                .reason = "original_factory",
                .model_archive_name = std::string(object_name),
            };
        }

        if (direct_object_archive_exists(dvd, object_name)) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::GenericModel,
                .reason = "direct_object_archive_model",
                .model_archive_name = std::string(object_name),
            };
        }

        if (const auto archive_name = original_archive_model_name(dvd, object_name); !archive_name.empty()) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::GenericAliasModel,
                .reason = "original_archive_model",
                .model_archive_name = std::string(archive_name),
            };
        }

        return NameObjPlacementSupport{
            .kind = NameObjPlacementSupportKind::Unsupported,
            .reason = "no_original_factory_or_direct_object_archive",
            .model_archive_name = "",
        };
    }

    std::vector<NameObjArchiveRequest> collect_name_obj_archive_requests(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name) {
        auto requests = std::vector<NameObjArchiveRequest>{};
        for (const auto &record : cOriginalArchiveRecords) {
            if (record.object_name == object_name) {
                add_archive_request(requests, describe_archive(dvd, record.archive_name));
            }
        }

        if (requests.empty()) {
            if (auto archive_path = dvd.find_object_archive(object_name)) {
                auto request = NameObjArchiveRequest{
                    .archive_name = std::string(object_name),
                    .disc_path = disc_relative(dvd.root(), *archive_path).generic_string(),
                    .resolved_path = archive_path->generic_string(),
                    .kind = NameObjArchiveKind::Object,
                    .loaded = dvd.archive_load_count_for_path(*archive_path) > 0U,
                };
                add_archive_request(requests, std::move(request));
            }
        }

        if (requests.empty()) {
            const auto alias_name = original_archive_model_name(dvd, object_name);
            if (!alias_name.empty()) {
                if (auto archive_path = dvd.find_object_archive(alias_name)) {
                    auto request = NameObjArchiveRequest{
                        .archive_name = std::string(alias_name),
                        .disc_path = disc_relative(dvd.root(), *archive_path).generic_string(),
                        .resolved_path = archive_path->generic_string(),
                        .kind = NameObjArchiveKind::Object,
                        .loaded = dvd.archive_load_count_for_path(*archive_path) > 0U,
                    };
                    add_archive_request(requests, std::move(request));
                }
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

    std::unique_ptr<NameObj> create_name_obj(smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name, std::string_view actor_name) {
        const auto object = std::string(object_name);
        const auto creator = NameObjFactory::getCreator(object.c_str());
        if (creator != nullptr) {
            const auto name = std::string(actor_name);
            return std::unique_ptr<NameObj>(creator(name.c_str()));
        }

        if (direct_object_archive_exists(dvd, object_name)) {
            const auto name = std::string(actor_name);
            return std::make_unique<ModelObj>(name.c_str(), object.c_str(), nullptr, MR::DrawBufferType_MapObj, MR::MovementType_MapObj,
                                              MR::CalcAnimType_MapObj, true);
        }

        if (const auto archive_name = original_archive_model_name(dvd, object_name); !archive_name.empty()) {
            const auto name = std::string(actor_name);
            const auto archive = std::string(archive_name);
            return std::make_unique<ModelObj>(name.c_str(), archive.c_str(), nullptr, MR::DrawBufferType_MapObj, MR::MovementType_MapObj,
                                              MR::CalcAnimType_MapObj, true);
        }

        throw std::runtime_error("Unsupported NameObj factory request: " + std::string(object_name));
    }

}  // namespace smgpc::scene::nameobj
