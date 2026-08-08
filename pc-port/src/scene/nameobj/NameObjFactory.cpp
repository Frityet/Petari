#include "scene/nameobj/NameObjFactory.hpp"

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Map/GroupSwitchWatcher.hpp"
#include "Game/Map/SwitchSynchronizer.hpp"
#include "Game/MapObj/CollisionBlocker.hpp"
#include "Game/MapObj/InvisiblePolygonObj.hpp"
#include "Game/MapObj/InvisiblePolygonObjGCapture.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/AreaObjRuntime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    template <typename T>
    NameObj *create_supported_name_obj(const char *pName) {
        return new T(pName);
    }

    // This is a compiled subset of the retail cCreateTable, not an alternate
    // placement policy. An entry is present only when its normal init path has
    // no known mandatory dependency on an unavailable host subsystem.
    constexpr auto cSupportedCreateTable = std::array{
        NameObjFactory::Name2CreateFunc{
            "PrologueDirector",
            create_supported_name_obj<PrologueDirector>,
            "DemoLetter",
        },
        NameObjFactory::Name2CreateFunc{
            "GroupSwitchWatcher",
            create_supported_name_obj<GroupSwitchWatcher>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "SwitchSynchronizerReverse",
            create_supported_name_obj<SwitchSynchronizer>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "CollisionBlocker",
            create_supported_name_obj<CollisionBlocker>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GhostShipCavePipeCollision",
            create_supported_name_obj<InvisiblePolygonObj>,
            "GhostShipCavePipeCollision",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWall10x10",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWall10x10",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWall10x20",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWall10x20",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallJump10x10",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWallJump10x10",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallJump10x20",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWallJump10x20",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallGCapture10x10",
            create_supported_name_obj<InvisiblePolygonObjGCapture>,
            "InvisibleWallGCapture10x10",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallGCapture10x20",
            create_supported_name_obj<InvisiblePolygonObjGCapture>,
            "InvisibleWallGCapture10x20",
        },
        NameObjFactory::Name2CreateFunc{
            "PolygonCodeRecoveryPlate",
            create_supported_name_obj<InvisiblePolygonObj>,
            "PolygonCodeRecoveryPlate",
        },
        NameObjFactory::Name2CreateFunc{
            "PolygonCodeRecoveryBowl",
            create_supported_name_obj<InvisiblePolygonObj>,
            "PolygonCodeRecoveryBowl",
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalCubeGravity",
            MR::createGlobalCubeGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalConeGravity",
            MR::createGlobalConeGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalDiskGravity",
            MR::createGlobalDiskGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalDiskTorusGravity",
            MR::createGlobalDiskTorusGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPlaneGravity",
            MR::createGlobalPlaneGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPlaneGravityInBox",
            MR::createGlobalPlaneInBoxGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPlaneGravityInCylinder",
            MR::createGlobalPlaneInCylinderGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPointGravity",
            MR::createGlobalPointGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalSegmentGravity",
            MR::createGlobalSegmentGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalWireGravity",
            MR::createGlobalWireGravityObj,
            nullptr,
        },
    };

    struct UnavailableCreatorRecord {
        std::string_view object_name;
        std::string_view reason;
    };

    constexpr auto cUnavailableCreatorTable = std::array{
        UnavailableCreatorRecord{"FileSelector", "retail_file_select_actor_runtime_unavailable"},
        UnavailableCreatorRecord{
            "SphereSelectorHandle",
            "system_se_me_and_stage_bgm_playback_runtime_unavailable",
        },
        UnavailableCreatorRecord{
            "RestartCube",
            "real_mario_update_and_restart_dispatch_runtime_unavailable",
        },
        UnavailableCreatorRecord{"Steam", "clipping_group_runtime_unavailable"},
        UnavailableCreatorRecord{"Coin", "shadow_runtime_unavailable"},
        UnavailableCreatorRecord{"PurpleCoin", "shadow_runtime_unavailable"},
        UnavailableCreatorRecord{"RailCoin", "shadow_area_and_mercator_runtime_unavailable"},
        UnavailableCreatorRecord{"PurpleRailCoin", "shadow_area_and_mercator_runtime_unavailable"},
        UnavailableCreatorRecord{"PurpleCoinStarter", "event_power_star_and_scene_layout_runtime_unavailable"},
        UnavailableCreatorRecord{"DemoRabbit", "npc_joint_controller_and_behavior_runtime_unavailable"},
        UnavailableCreatorRecord{"StarPieceFlow", "star_piece_director_runtime_unavailable"},
        UnavailableCreatorRecord{"StarPieceGroup", "star_piece_director_runtime_unavailable"},
    };

    struct OriginalArchiveRecord {
        std::string_view object_name;
        std::string_view archive_name;
    };

    constexpr OriginalArchiveRecord cOriginalArchiveRecords[] = {
#include "NameObjArchiveTable.inc"
    };

    constexpr auto cPlayerArchiveLoaderObjTable = std::array<std::string_view, 8>{
        "Hopper",
        "BenefitItemInvincible",
        "MorphItemNeoBee",
        "MorphItemNeoFire",
        "MorphItemNeoFoo",
        "MorphItemNeoHopper",
        "MorphItemNeoIce",
        "MorphItemNeoTeresa",
    };

    [[nodiscard]] bool equal_string_case(std::string_view lhs, std::string_view rhs) {
        return lhs.size() == rhs.size() && std::ranges::equal(lhs, rhs, [](char left, char right) {
                   return std::tolower(static_cast<unsigned char>(left)) ==
                          std::tolower(static_cast<unsigned char>(right));
               });
    }

    [[nodiscard]] const NameObjFactory::Name2CreateFunc *find_supported_entry(std::string_view object_name) {
        const auto found = std::ranges::find_if(cSupportedCreateTable, [&](const auto &entry) {
            return equal_string_case(entry.mName, object_name);
        });
        return found != cSupportedCreateTable.end() ? &*found : nullptr;
    }

    [[nodiscard]] const std::vector<NameObjFactory::Name2CreateFunc> &area_obj_create_table() {
        static const auto table = [] {
            auto result = std::vector<NameObjFactory::Name2CreateFunc>{};
            const auto descriptors = smgpc::scene::complete_area_obj_placement_descriptors();
            result.reserve(descriptors.size());
            for (const auto &descriptor : descriptors) {
                result.push_back(NameObjFactory::Name2CreateFunc{
                    descriptor.object_name.data(),
                    descriptor.object_creator,
                    nullptr,
                });
            }
            return result;
        }();
        return table;
    }

    [[nodiscard]] const NameObjFactory::Name2CreateFunc *find_area_obj_entry(std::string_view object_name) {
        const auto &table = area_obj_create_table();
        const auto found = std::ranges::find_if(table, [&](const auto &entry) {
            return equal_string_case(entry.mName, object_name);
        });
        return found != table.end() ? &*found : nullptr;
    }

    [[nodiscard]] const UnavailableCreatorRecord *find_unavailable_entry(std::string_view object_name) {
        const auto found = std::ranges::find_if(cUnavailableCreatorTable, [&](const auto &entry) {
            return equal_string_case(entry.object_name, object_name);
        });
        return found != cUnavailableCreatorTable.end() ? &*found : nullptr;
    }

    [[nodiscard]] smgpc::scene::nameobj::NameObjCreatorSupport describe_creator_support(std::string_view object_name) {
        using smgpc::scene::nameobj::NameObjCreatorSupport;
        using smgpc::scene::nameobj::NameObjCreatorSupportKind;

        if (find_supported_entry(object_name) != nullptr) {
            return NameObjCreatorSupport{
                .kind = NameObjCreatorSupportKind::Supported,
                .reason = "compiled_retail_creator",
            };
        }
        if (find_area_obj_entry(object_name) != nullptr) {
            return NameObjCreatorSupport{
                .kind = NameObjCreatorSupportKind::Supported,
                .reason = "compiled_retail_area_creator_and_manager",
            };
        }
        if (const auto *unavailable = find_unavailable_entry(object_name); unavailable != nullptr) {
            return NameObjCreatorSupport{
                .kind = NameObjCreatorSupportKind::RuntimeClosureUnavailable,
                .reason = std::string(unavailable->reason),
            };
        }
        return NameObjCreatorSupport{
            .kind = NameObjCreatorSupportKind::NotLinked,
            .reason = "retail_creator_not_linked",
        };
    }

    [[nodiscard]] std::filesystem::path disc_relative(const std::filesystem::path &root,
                                                      const std::filesystem::path &path) {
        std::error_code error{};
        auto relative = std::filesystem::relative(path, root, error);
        if (!error && !relative.empty()) {
            return relative;
        }
        return path.filename();
    }

    [[nodiscard]] smgpc::scene::nameobj::NameObjArchiveRequest describe_archive(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view archive_name) {
        using smgpc::scene::nameobj::NameObjArchiveKind;
        using smgpc::scene::nameobj::NameObjArchiveRequest;

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

    void add_archive_request(std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> &requests,
                             smgpc::scene::nameobj::NameObjArchiveRequest request) {
        const auto duplicate = std::ranges::find_if(requests, [&](const auto &existing) {
            return existing.archive_name == request.archive_name && existing.kind == request.kind &&
                   existing.resolved_path == request.resolved_path;
        });
        if (duplicate == requests.end()) {
            requests.push_back(std::move(request));
        }
    }

    [[nodiscard]] bool contains_path_component(std::string_view path, std::string_view component) {
        return path.find(component) != std::string_view::npos;
    }

    [[nodiscard]] bool is_proven_non_actor_helper_table(std::string_view table_path) {
        // StageObjInfo rows describe zone composition and DemoObjInfo rows feed
        // the separate demo-sheet loader. Actor-bearing AreaObjInfo,
        // CameraCubeInfo, and PlanetObjInfo must pass through the factory
        // boundary and remain blocked until their real owners exist.
        return contains_path_component(table_path, "/stageobjinfo") ||
               contains_path_component(table_path, "/demoobjinfo");
    }

}  // namespace

namespace NameObjFactory {

    CreatorFuncPtr getCreator(const char *pName) {
        const auto name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        if (const auto *entry = find_supported_entry(name); entry != nullptr) {
            return entry->mCreateFunc;
        }
        const auto *area_entry = find_area_obj_entry(name);
        return area_entry != nullptr ? area_entry->mCreateFunc : nullptr;
    }

    void requestMountObjectArchives(const char *pName, const JMapInfoIter &rIter) {
        auto archive_list = NameObjArchiveListCollector{};
        getMountObjectArchiveList(&archive_list, pName, rIter);
        for (auto index = s32{}; index < archive_list.mCount; ++index) {
            MR::mountAsyncArchiveByObjectOrLayoutName(archive_list.getArchive(index), nullptr);
        }
    }

    bool isReadResourceFromDVD(const char *pName, const JMapInfoIter &rIter) {
        auto archive_list = NameObjArchiveListCollector{};
        getMountObjectArchiveList(&archive_list, pName, rIter);
        for (auto index = s32{}; index < archive_list.mCount; ++index) {
            if (!MR::isLoadedObjectOrLayoutArchive(archive_list.getArchive(index))) {
                return true;
            }
        }
        return false;
    }

    bool isPlayerArchiveLoaderObj(const char *pArchive) {
        if (pArchive == nullptr) {
            return false;
        }
        return std::ranges::any_of(cPlayerArchiveLoaderObjTable, [&](std::string_view entry) {
            return equal_string_case(entry, pArchive);
        });
    }

    const Name2CreateFunc *getName2CreateFunc(const char *pName, const Name2CreateFunc *pTable) {
        if (pTable != nullptr && pTable != cSupportedCreateTable.data()) {
            throw std::invalid_argument(
                "External NameObj creator tables are unavailable without an explicit table extent.");
        }
        const auto name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        if (const auto *entry = find_supported_entry(name); entry != nullptr || pTable != nullptr) {
            return entry;
        }
        return find_area_obj_entry(name);
    }

    void getMountObjectArchiveList(NameObjArchiveListCollector *pArchiveList, const char *pName,
                                   const JMapInfoIter &) {
        if (pArchiveList == nullptr) {
            throw std::invalid_argument("NameObj archive collection requires a real collector.");
        }

        const auto object_name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        const auto *creator_entry = find_supported_entry(object_name);
        if (creator_entry == nullptr && find_area_obj_entry(object_name) == nullptr) {
            return;
        }

        if (creator_entry != nullptr && creator_entry->mArchiveName != nullptr) {
            pArchiveList->addArchive(creator_entry->mArchiveName);
        }
        for (const auto &archive : cOriginalArchiveRecords) {
            if (archive.object_name != object_name) {
                continue;
            }
            if (creator_entry != nullptr && creator_entry->mArchiveName != nullptr &&
                archive.archive_name == creator_entry->mArchiveName) {
                continue;
            }
            pArchiveList->addArchive(archive.archive_name.data());
        }
    }

}  // namespace NameObjFactory

namespace smgpc::scene::nameobj {

    bool can_create_name_obj(std::string_view object_name) {
        return find_supported_entry(object_name) != nullptr || find_area_obj_entry(object_name) != nullptr;
    }

    NameObjCreatorSupport describe_name_obj_creator_support(std::string_view object_name) {
        return describe_creator_support(object_name);
    }

    NameObjPlacementSupport describe_name_obj_placement_support(smgpc::runtime::DvdFileSystemService &,
                                                                std::string_view object_name,
                                                                std::string_view table_path) {
        if (is_proven_non_actor_helper_table(table_path)) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::IntentionallyIgnored,
                .reason = "non_actor_helper_table",
            };
        }

        const auto creator_support = describe_creator_support(object_name);
        if (creator_support.kind == NameObjCreatorSupportKind::Supported) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::OriginalFactory,
                .reason = "original_factory",
            };
        }

        return NameObjPlacementSupport{
            .kind = NameObjPlacementSupportKind::Unsupported,
            .reason = creator_support.reason,
        };
    }

    std::vector<NameObjArchiveRequest> collect_name_obj_archive_requests(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
        const JMapInfoIter *placement_iter) {
        auto requests = std::vector<NameObjArchiveRequest>{};
        if (!can_create_name_obj(object_name)) {
            return requests;
        }

        auto collector = NameObjArchiveListCollector{};
        const auto invalid_iter = JMapInfoIter{};
        const auto object = std::string(object_name);
        NameObjFactory::getMountObjectArchiveList(
            &collector, object.c_str(), placement_iter != nullptr ? *placement_iter : invalid_iter);
        for (auto index = s32{}; index < collector.mCount; ++index) {
            add_archive_request(requests, describe_archive(dvd, collector.getArchive(index)));
        }
        return requests;
    }

    NameObjFactoryDescription describe_name_obj_factory(smgpc::runtime::DvdFileSystemService &dvd,
                                                        std::string_view object_name) {
        const auto creator_support = describe_creator_support(object_name);
        return NameObjFactoryDescription{
            .object_name = std::string(object_name),
            .creator_supported = creator_support.kind == NameObjCreatorSupportKind::Supported,
            .creator_support = creator_support,
            .archives = collect_name_obj_archive_requests(dvd, object_name, nullptr),
        };
    }

    std::vector<NameObjArchiveRequest> preload_name_obj_archives(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
        const JMapInfoIter *placement_iter) {
        auto requests = collect_name_obj_archive_requests(dvd, object_name, placement_iter);
        for (auto &request : requests) {
            if (request.kind == NameObjArchiveKind::Missing || request.resolved_path.empty()) {
                throw std::runtime_error("Required retail archive is unavailable for " +
                                         std::string(object_name) + ": " + request.archive_name);
            }
            auto &archive = dvd.archive_for_path(request.resolved_path);
            (void)archive;
            request.loaded = true;
        }
        return requests;
    }

    std::unique_ptr<NameObj> create_name_obj(smgpc::runtime::DvdFileSystemService &,
                                             std::string_view object_name, const char *actor_name) {
        const auto object = std::string(object_name);
        const auto creator = NameObjFactory::getCreator(object.c_str());
        if (creator == nullptr) {
            const auto support = describe_creator_support(object_name);
            throw std::runtime_error("Unsupported NameObj factory request: " + object +
                                     " (" + support.reason + ")");
        }

        auto result = std::unique_ptr<NameObj>(creator(actor_name));
        if (result == nullptr) {
            throw std::runtime_error("Retail NameObj creator returned null: " + object);
        }
        return result;
    }

}  // namespace smgpc::scene::nameobj
