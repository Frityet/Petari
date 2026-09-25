#include "Game/Map/HitInfo.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "scene/SceneInitializationState.hpp"
#include <aurora/allocation.hpp>
#include <aurora/main.h>
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Map/FileSelector.hpp"
#include "Game/MapObj/InvisiblePolygonObj.hpp"
#include "Game/MapObj/InvisiblePolygonObjGCapture.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/NPC/DemoRabbit.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "resource/GameResourceRuntime.hpp"
#include <aurora/aurora.h>
#include "runtime/RuntimeServices.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Util/MapUtil.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(std::is_base_of_v<LiveActor, FileSelector>,
              "the exact retail FileSelector declaration must coexist with the host boundary");

namespace aurora { extern AuroraConfig g_config; }

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            aurora::throw_host_exception<std::runtime_error>(std::string(message));
        }
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset,
                    std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    JMapInfo make_demo_rabbit_archive_placement_info() {
        constexpr auto cEntryCount = 3U;
        constexpr auto cFieldOffset = 0x10U;
        constexpr auto cDataOffset = 0x1cU;
        constexpr auto cEntrySize = 4U;
        auto bytes = std::vector<std::uint8_t>(
            cDataOffset + cEntryCount * cEntrySize, 0U);
        write_be32(bytes, 0x00U, cEntryCount);
        write_be32(bytes, 0x04U, 1U);
        write_be32(bytes, 0x08U, cDataOffset);
        write_be32(bytes, 0x0cU, cEntrySize);
        write_be32(bytes, cFieldOffset,
                   smgpc::resource::jmap_hash("CastId"));
        write_be32(bytes, cFieldOffset + 0x04U, 0xffffffffU);
        for (auto cast_id = 0U; cast_id < cEntryCount; ++cast_id) {
            write_be32(bytes, cDataOffset + cast_id * cEntrySize, cast_id);
        }
        return JMapInfo::from_bcsv(bytes);
    }

    template <typename Function>
    void require_throws(Function &&function, std::string_view message) {
        try {
            function();
        } catch (const std::exception &) {
            return;
        }
        throw std::runtime_error(std::string(message));
    }

    class ScopedEnvironmentVariable final {
    public:
        ScopedEnvironmentVariable(const char *name, std::string value) : _name(name) {
            if (const auto *previous = std::getenv(name); previous != nullptr) {
                _previous = previous;
            }
            if (setenv(_name.c_str(), value.c_str(), 1) != 0) {
                throw std::runtime_error("could not set test environment variable " + _name);
            }
        }

        ~ScopedEnvironmentVariable() {
            if (_previous.has_value()) {
                (void)setenv(_name.c_str(), _previous->c_str(), 1);
            } else {
                (void)unsetenv(_name.c_str());
            }
        }

    private:
        std::string _name;
        std::optional<std::string> _previous;
    };

    [[nodiscard]] std::optional<std::filesystem::path> find_real_disc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            return std::filesystem::path(configured);
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        return std::nullopt;
    }

    void test_factory_is_the_only_constructible_support_kind() {
        aurora_dvd_close();
        DVDInit();
        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        constexpr auto cUnsupportedFixture = std::string_view{"PcPortArchiveOnlyFixture"};

        require(smgpc::scene::nameobj::can_create_name_obj("CollisionBlocker"),
                "an object in the real NameObjFactory table should be constructible");
        require(!smgpc::scene::nameobj::can_create_name_obj(cUnsupportedFixture),
                "the synthetic unsupported fixture must remain absent from the real factory table");

        const auto factory = smgpc::scene::nameobj::describe_name_obj_placement_support(
            dvd, "CollisionBlocker", "jmp/placement/common/objinfo");
        require(factory.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::OriginalFactory &&
                    factory.reason == "original_factory",
                "ordinary placement support should come only from the real factory");

        const auto unsupported = smgpc::scene::nameobj::describe_name_obj_placement_support(
            dvd, cUnsupportedFixture, "jmp/placement/common/objinfo");
        require(unsupported.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::Unsupported &&
                    unsupported.reason == "retail_creator_not_linked",
                "an ordinary placement without a real creator should be unsupported");

        const auto helper = smgpc::scene::nameobj::describe_name_obj_placement_support(
            dvd, cUnsupportedFixture, "jmp/placement/common/stageobjinfo");
        require(helper.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::IntentionallyIgnored,
                "non-renderable helper tables should remain intentionally ignored");

        for (const auto table : {"jmp/placement/common/areaobjinfo", "jmp/placement/common/cameracubeinfo",
                                 "jmp/placement/common/planetobjinfo"}) {
            const auto actor_bearing = smgpc::scene::nameobj::describe_name_obj_placement_support(
                dvd, cUnsupportedFixture, table);
            require(actor_bearing.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::Unsupported,
                    "actor-bearing placement tables must not be blanket-hidden as helper metadata");
        }

        require(smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, "CollisionBlocker").empty(),
                "a creator with no retail archive mapping must not infer a same-name archive");
        const auto factory_archives = smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, "PrologueDirector");
        const auto has_archive = [&](std::string_view name) {
            return std::ranges::any_of(factory_archives, [&](const auto &request) {
                return request.archive_name == name;
            });
        };
        require(has_archive("DemoLetter") && has_archive("PeachLetterMini") &&
                    has_archive("PrologueDemo") && has_archive("DemoPeachCastleGate"),
                "supported creators should retain only their retail-derived archive mappings");
        require(smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, cUnsupportedFixture).empty(),
                "unsupported objects should not produce preload requests");

        constexpr auto cInvisiblePolygonFamily = std::array{
            std::pair<std::string_view, bool>{"GhostShipCavePipeCollision", false},
            std::pair<std::string_view, bool>{"InvisibleWall10x10", false},
            std::pair<std::string_view, bool>{"InvisibleWall10x20", false},
            std::pair<std::string_view, bool>{"InvisibleWallJump10x10", false},
            std::pair<std::string_view, bool>{"InvisibleWallJump10x20", false},
            std::pair<std::string_view, bool>{"InvisibleWallGCapture10x10", true},
            std::pair<std::string_view, bool>{"InvisibleWallGCapture10x20", true},
            std::pair<std::string_view, bool>{"PolygonCodeRecoveryPlate", false},
            std::pair<std::string_view, bool>{"PolygonCodeRecoveryBowl", false},
        };
        for (const auto &[name, gravity_capture] : cInvisiblePolygonFamily) {
            const auto support = smgpc::scene::nameobj::describe_name_obj_creator_support(name);
            const auto archives = smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, name);
            require(support.kind == smgpc::scene::nameobj::NameObjCreatorSupportKind::Supported &&
                        NameObjFactory::getCreator(std::string(name).c_str()) != nullptr &&
                        archives.size() == 1U && archives.front().archive_name == name,
                    "the whole retail InvisiblePolygonObj family must expose its exact creator/archive pair");

            auto object = smgpc::scene::nameobj::create_name_obj(dvd, name, name.data());
            require(dynamic_cast<InvisiblePolygonObj *>(object.get()) != nullptr &&
                        (dynamic_cast<InvisiblePolygonObjGCapture *>(object.get()) != nullptr) == gravity_capture,
                    "each InvisiblePolygonObj family entry must construct its exact retail class");
        }

        for (const auto &descriptor : smgpc::scene::complete_area_obj_placement_descriptors()) {
            const auto object_name = std::string(descriptor.object_name);
            const auto *table_entry = NameObjFactory::getName2CreateFunc(object_name.c_str(), nullptr);
            require(smgpc::scene::nameobj::can_create_name_obj(descriptor.object_name) &&
                        NameObjFactory::getCreator(object_name.c_str()) == descriptor.object_creator &&
                        table_entry != nullptr && table_entry->mCreateFunc == descriptor.object_creator,
                    "AreaObj factory support must derive from the same complete creator-manager descriptor");

            auto object = smgpc::scene::nameobj::create_name_obj(
                dvd, descriptor.object_name, "localized actor label");
            require(dynamic_cast<AreaObj *>(object.get()) != nullptr &&
                        std::string_view(object->getName()) == "localized actor label",
                    "the factory must preserve the resolved actor name until exact placement init applies JMap naming");
        }

        constexpr auto cUnavailableCreators = std::array{
            std::string_view{"FileSelector"},
            std::string_view{"RailCoin"},
            std::string_view{"PurpleRailCoin"},
            std::string_view{"PurpleCoinStarter"},
        };
        for (const auto name : cUnavailableCreators) {
            const auto support = smgpc::scene::nameobj::describe_name_obj_creator_support(name);
            require(support.kind == smgpc::scene::nameobj::NameObjCreatorSupportKind::RuntimeClosureUnavailable &&
                        !support.reason.empty() && NameObjFactory::getCreator(std::string(name).c_str()) == nullptr &&
                        smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, name).empty(),
                    "retail creator still lacks its mandatory init owner: " + std::string(name));
        }
        for (const auto name : {"RestartCube", "Steam", "Coin", "PurpleCoin", "StarPieceFlow", "StarPieceGroup"}) {
            require(smgpc::scene::nameobj::describe_name_obj_creator_support(name).kind ==
                        smgpc::scene::nameobj::NameObjCreatorSupportKind::Supported && NameObjFactory::getCreator(name),
                    "current original creator must remain registered: " + std::string(name));
        }

        require_throws(
            [&] {
                (void)smgpc::scene::nameobj::preload_name_obj_archives(dvd, "PrologueDirector");
            },
            "preloading a missing explicit retail archive should reject instead of silently continuing");

        require_throws(
            [&] {
                (void)smgpc::scene::nameobj::create_name_obj(dvd, cUnsupportedFixture,
                                                             cUnsupportedFixture.data());
            },
            "an unsupported object should not be synthesized as a generic ModelObj");
    }

    void test_demo_rabbit_creator_and_placement_archive_callback_are_atomic() {
        aurora_dvd_close();
        DVDInit();
        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};

        const auto support =
            smgpc::scene::nameobj::describe_name_obj_creator_support("DemoRabbit");
        const auto *entry = NameObjFactory::getName2CreateFunc("DemoRabbit", nullptr);
        require(support.kind ==
                        smgpc::scene::nameobj::NameObjCreatorSupportKind::Supported &&
                    smgpc::scene::nameobj::can_create_name_obj("DemoRabbit") &&
                    NameObjFactory::getCreator("DemoRabbit") != nullptr &&
                    entry != nullptr && entry->mArchiveName == nullptr,
                "DemoRabbit creator support must become available atomically without a fabricated fixed archive");

        auto object = smgpc::scene::nameobj::create_name_obj(
            dvd, "DemoRabbit", "localized DemoRabbit");
        require(dynamic_cast<DemoRabbit *>(object.get()) != nullptr &&
                    std::string_view(object->getName()) == "localized DemoRabbit",
                "the supported DemoRabbit creator must construct its exact retail actor class and preserve its actor name");

        auto fixed_then_aliases = NameObjArchiveListCollector{};
        NameObjFactory::getMountObjectArchiveList(
            &fixed_then_aliases, "PrologueDirector", JMapInfoIter{});
        constexpr auto cExpectedFixedThenAliases = std::array<std::string_view, 5>{
            "DemoLetter",
            "DemoLetter",
            "PeachLetterMini",
            "PrologueDemo",
            "DemoPeachCastleGate",
        };
        require(fixed_then_aliases.getArchiveNum() ==
                    static_cast<s32>(cExpectedFixedThenAliases.size()),
                "retail archive collection must retain the fixed archive followed by every exact original alias");
        for (auto index = 0U; index < cExpectedFixedThenAliases.size(); ++index) {
            require(std::string_view(fixed_then_aliases.getArchive(static_cast<s32>(index))) ==
                        cExpectedFixedThenAliases[index],
                    "fixed and original-alias archives must retain retail table order without duplicate suppression");
        }

        auto placement = make_demo_rabbit_archive_placement_info();
        constexpr auto cExpectedArchives = std::array<std::string_view, 3>{
            "TrickRabbitBaby",
            "TrickRabbit",
            "TrickRabbit",
        };
        for (auto row = 0; row < placement.getNumEntries(); ++row) {
            const auto iter = JMapInfoIter(&placement, row);
            auto collector = NameObjArchiveListCollector{};
            NameObjFactory::getMountObjectArchiveList(
                &collector, "DemoRabbit", iter);
            require(collector.getArchiveNum() == 1 &&
                        std::string_view(collector.getArchive(0)) ==
                            cExpectedArchives[static_cast<std::size_t>(row)],
                    "DemoRabbit CastId 0/1/2 must select baby/adult/adult through the real placement iterator");

            const auto requests =
                smgpc::scene::nameobj::collect_name_obj_archive_requests(
                    dvd, "DemoRabbit", &iter);
            require(requests.size() == 1U &&
                        requests.front().archive_name ==
                            cExpectedArchives[static_cast<std::size_t>(row)],
                    "the host preload description must retain the exact placement-selected DemoRabbit archive");
        }
    }

    void test_optional_real_disc_archive_presence_does_not_create_support() {
        const auto disc_path = find_real_disc();
        if (!disc_path.has_value()) {
            std::cout << "[skip] real-disc placement support test (set SMGPC_REAL_DISC or place RMGK01.iso in a workspace ancestor)\n";
            return;
        }

        aurora_dvd_close();
        const auto disc_path_string = disc_path->string();
        require(aurora_dvd_open(disc_path_string.c_str()), "the selected real-disc fixture should be a readable SMG image");
        std::cout << "[info] real-disc fixture: " << disc_path_string << '\n';
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        auto direct_archive_only_name = std::string{};
        auto direct_archive_only_path = std::filesystem::path{};
        for (const auto &entry : dvd.directory_entries("/ObjectData")) {
            const auto path = std::filesystem::path(entry.name);
            if (entry.is_directory || path.extension() != ".arc") {
                continue;
            }
            const auto object_name = path.stem().string();
            if (!smgpc::scene::nameobj::can_create_name_obj(object_name)) {
                direct_archive_only_name = object_name;
                direct_archive_only_path = dvd.find_object_archive(object_name).value_or(std::filesystem::path{});
                if (!direct_archive_only_path.empty()) {
                    break;
                }
            }
        }
        if (!direct_archive_only_name.empty()) {
            std::cout << "[info] direct archive-only candidate: " << direct_archive_only_name << '\n';
            const auto direct = smgpc::scene::nameobj::describe_name_obj_placement_support(
                dvd, direct_archive_only_name, "jmp/placement/common/objinfo");
            require(direct.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::Unsupported &&
                        smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, direct_archive_only_name).empty(),
                    "a direct model archive must not make an object constructible or preloadable");
        } else {
            std::cout << "[skip] every ObjectData archive has acquired a real factory creator\n";
        }

        constexpr auto cAliasCandidates = std::array{
            std::pair<std::string_view, std::string_view>{"Bomb", "BombHei"},
            std::pair<std::string_view, std::string_view>{"Rabbit", "MoonRabbit"},
            std::pair<std::string_view, std::string_view>{"TalkSyati", "Syati"},
            std::pair<std::string_view, std::string_view>{"CollectTico", "StrayTico"},
            std::pair<std::string_view, std::string_view>{"PenguinRacer", "Penguin"},
            std::pair<std::string_view, std::string_view>{"ButlerMap", "Butler"},
            std::pair<std::string_view, std::string_view>{"DemoKoopaJrShip", "KoopaJrShip"},
            std::pair<std::string_view, std::string_view>{"JetTurtle", "Koura"},
        };
        const auto alias_only = std::ranges::find_if(cAliasCandidates, [&](const auto &candidate) {
            return !smgpc::scene::nameobj::can_create_name_obj(candidate.first) &&
                   !dvd.find_object_archive(candidate.first).has_value() &&
                   dvd.find_object_archive(candidate.second).has_value();
        });
        if (alias_only != cAliasCandidates.end()) {
            std::cout << "[info] alias-only candidate: " << alias_only->first << " -> " << alias_only->second << '\n';
            const auto alias = smgpc::scene::nameobj::describe_name_obj_placement_support(
                dvd, alias_only->first, "jmp/placement/common/objinfo");
            require(alias.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::Unsupported &&
                        smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, alias_only->first).empty(),
                    "an original archive alias must not make an object constructible or preloadable");
        } else {
            std::cout << "[skip] no remaining non-factory alias-only candidate on this disc\n";
        }


    }

    [[nodiscard]] bool line_query_hits_registered_wall(
        Triangle* hit = nullptr, const CollisionPartsFilterBase* filter = nullptr) {
        constexpr auto coordinates = std::array{-750.0F, -500.0F, -250.0F, 0.0F,
                                                250.0F, 500.0F, 750.0F, 1000.0F};
        for (auto axis = 0U; axis < 3U; ++axis) {
            for (const auto first : coordinates) {
                for (const auto second : coordinates) {
                    for (const auto direction : {-1.0F, 1.0F}) {
                        auto start = TVec3f{};
                        auto offset = TVec3f{};
                        auto *start_values = &start.x;
                        auto *offset_values = &offset.x;
                        start_values[axis] = -direction * 2000.0F;
                        offset_values[axis] = direction * 4000.0F;
                        start_values[(axis + 1U) % 3U] = first;
                        start_values[(axis + 2U) % 3U] = second;
                        TVec3f position;
                        Triangle triangle;
                        if (MR::getFirstPolyOnLineToMap(&position, &triangle, start, offset, filter, nullptr)) {
                            if (hit != nullptr) {
                                *hit = triangle;
                            }
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

#ifndef NDEBUG
    // This deliberately injects one unchanged FileSelect row into a bounded
    // original-process test. It does not claim the wall is authored in Gateway.
    class PlacementTableRangeBinding final {
    public:
        PlacementTableRangeBinding(StageDataHolder& holder, const JMapInfo& table)
            : _holder(holder), _begin(holder._E4), _end(holder._E8) {
            const auto begin = reinterpret_cast<std::uintptr_t>(table.getData());
            const auto size = table.getDataSize();
            require(begin && size && begin + size > begin,
                    "Injected placement requires its retained exact JMap byte range");
            _holder._E4 = begin;
            _holder._E8 = begin + size;
        }
        ~PlacementTableRangeBinding() {
            _holder._E4 = _begin;
            _holder._E8 = _end;
        }
    private:
        StageDataHolder& _holder;
        std::uintptr_t _begin;
        std::uintptr_t _end;
    };

    class OnlyWall final : public CollisionPartsFilterBase {
    public:
        explicit OnlyWall(const CollisionParts* parts) : _parts(parts) {}
        bool isInvalidParts(const CollisionParts* parts) const override { return parts != _parts; }
    private:
        const CollisionParts* _parts;
    };

    struct WallProbe {
        bool exercised = false;
        const InvisiblePolygonObj* retired_actor = nullptr;
        Triangle retired_triangle;

        void after_frame(GameSystem& system, std::uint64_t frame) {
            // Inject only at the terminal completed frame, after all ordinary
            // game callbacks. All temporary owners retire before returning.
            if (exercised || frame != 119) return;
            auto* controller = system.mSceneController;
            require(controller && controller->mSceneInitializeState == SceneInitializeState_End &&
                        controller->getCurrentSceneForExecute() == controller->mScene &&
                        dynamic_cast<GameScene*>(controller->mScene) && MR::isEqualStageName("HeavensDoorGalaxy"),
                    "Terminal wall probe requires the fully initialized ordinary Gateway process");
            const aurora::allocation::HostAllocationScope host;
            auto* stage = MR::getStageDataHolder();
            auto* collision = MR::getCollisionDirector();
            auto* resources = SingletonHolder<ResourceHolderManager>::get();
            const auto domain = MR::getSceneObjHolder()->nativeAllocationDomain();
            require(stage && stage->mZoneID == 0 && collision && resources && domain,
                    "Actual process owns the root stage, collision, resource manager and scene heap");

            JKRMemArchive* file_select = nullptr;
            smgpc::test::on_resource_worker([&] {
                file_select = MR::mountArchive("/StageData/FileSelect.arc", nullptr);
            });
            require(file_select != nullptr, "The original FileLoader owns the FileSelect archive");
            auto* placement_data = file_select->getResource('????', "/jmp/placement/common/objinfo");
            require(placement_data != nullptr, "FileSelect retains its original common object table");
            JMapInfo wall_table;
            wall_table.attach(placement_data);
            const auto iter = wall_table.findElement<const char*>("name", "InvisibleWall10x10", 0);
            require(iter.isValid() && NameObjFactory::getCreator("InvisibleWall10x10") != nullptr,
                    "The injected original row has its actual wall creator");
            ResourceHolder* wall_resources = nullptr;
            smgpc::test::on_resource_worker([&] {
                wall_resources = resources->createAndAdd("InvisibleWall10x10.arc", nullptr);
            });
            const auto& archive = wall_resources->nativeResourceSource();
            require(archive.resource_data("InvisibleWall10x10.kcl").size() == 1222U &&
                        archive.resource_data("InvisibleWall10x10.pa").size() == 96U &&
                        archive.resource_data("CollisionVersion").size() == 7U,
                    "Actual resource owner exposes the real RMGK01 wall KCL, attributes and version");
            const auto* kcl_entry = archive.find_resource("InvisibleWall10x10.kcl");
            require(kcl_entry != nullptr, "The exact wall KCL entry is retained in the resource archive");
            const auto expected_source = std::string("/ObjectData/InvisibleWall10x10.arc:/") + kcl_entry->path;
            const auto begin_before = stage->_E4;
            const auto end_before = stage->_E8;
            const auto zone_before = MR::getCurrentPlacementZoneId();
            const auto actors_before = smgpc::compat::actor_runtime_state_count();
            {
                const PlacementTableRangeBinding range(*stage, wall_table);
                require(stage->findPlacedStageDataHolder(iter) == stage && MR::getPlacedZoneId(iter) == 0,
                        "The exact injected row resolves through the original root holder's temporary range");
                struct RestorePlacementZone {
                    s32 previous = MR::getCurrentPlacementZoneId();
                    ~RestorePlacementZone() { MR::setCurrentPlacementZoneId(previous); }
                } zone;
                MR::setCurrentPlacementZoneId(0);
                const smgpc::scene::SceneInitializationScope placement(SceneInitializeState_Placement);
                std::unique_ptr<NameObj> object_owner;
                InvisiblePolygonObj* actor = nullptr;
                {
                    const smgpc::compat::JkrAllocationScope game(domain);
                    const auto creator = NameObjFactory::getCreator("InvisibleWall10x10");
                    require(creator != nullptr, "The retail factory provides the exact wall creator");
                    object_owner.reset(creator("InvisibleWall10x10"));
                    auto* object = object_owner.get();
                    actor = dynamic_cast<InvisiblePolygonObj*>(object);
                    require(actor != nullptr, "The retail wall creator constructs InvisiblePolygonObj");
                    actor->init(iter);
                }
                const auto source = actor->mCollisionParts ? actor->mCollisionParts->nativeResourceSource() : std::string_view{};
                const auto radius = actor->mCollisionParts ? MR::getCollisionBoundingSphereRange(actor) : -1.0F;
                std::fprintf(stderr, "[wall-probe] initialized actor=%p parts=%p registered=%d radius=%g source=%.*s\n",
                             static_cast<void*>(actor), static_cast<void*>(actor->mCollisionParts),
                             !actor->nativeCollisionParts().empty(), static_cast<double>(radius),
                             static_cast<int>(source.size()), source.data());
                require(actor->mCollisionParts && !actor->nativeCollisionParts().empty() &&
                            source == expected_source &&
                            MR::getCollisionBoundingSphereRange(actor) > 0.0F,
                        "Unchanged actor init creates actual CollisionParts and original clipping bounds");
                auto* parts = actor->mCollisionParts;
                auto* original_zone = parts->mZone;
                const auto zone_count = original_zone->mNumParts;
                const OnlyWall only_wall(parts);
                require(line_query_hits_registered_wall(nullptr, &only_wall),
                        "Original CollisionParts registration immediately publishes wall line queries");
                actor->initAfterPlacement();
                require(actor->nativeCollisionParts().size() == 1 &&
                            line_query_hits_registered_wall(&retired_triangle, &only_wall),
                        "Original post-placement callback retains exactly one additional KCL source");
                require(retired_triangle.mParts == parts && retired_triangle.getHostName() == actor->mName &&
                            retired_triangle.getHostPlacementZoneID() == 0 &&
                            actor->mCollisionParts->nativeResourceSource() == expected_source,
                        "Original triangle retains its injected host zone and exact resource provenance");

                actor->makeActorDead();
                require(!line_query_hits_registered_wall(nullptr, &only_wall),
                        "Dead original actors remove their parts from map queries");
                actor->makeActorAppeared();
                require(line_query_hits_registered_wall(nullptr, &only_wall),
                        "Original appearance restores retained collision membership");
                MR::invalidateCollisionParts(actor);
                MR::invalidateCollisionParts(actor);
                require(!line_query_hits_registered_wall(nullptr, &only_wall),
                        "Repeated invalidation remains absent from original queries");
                actor->makeActorDead();
                actor->makeActorAppeared();
                require(actor->mCollisionParts == parts && line_query_hits_registered_wall(nullptr, &only_wall),
                        "Appearance revalidates the same original CollisionParts identity");
                MR::invalidateCollisionParts(actor);
                require(!line_query_hits_registered_wall(nullptr, &only_wall),
                        "Explicit invalidation removes collision after reappearance");
                retired_actor = actor;
                object_owner.reset();
                require(!line_query_hits_registered_wall(nullptr, &only_wall) &&
                            !retired_triangle.isValid() && retired_triangle.getHostName() == nullptr &&
                            original_zone->mNumParts + 1 == zone_count &&
                            !smgpc::compat::has_actor_runtime_state(retired_actor),
                        "Actor retirement removes actual keeper membership, queries and retained Triangle identities");
            }
            require(stage->_E4 == begin_before && stage->_E8 == end_before &&
                        MR::getCurrentPlacementZoneId() == zone_before &&
                        controller->mSceneInitializeState == SceneInitializeState_End &&
                        smgpc::compat::actor_runtime_state_count() == actors_before,
                    "Injected row bounds, placement zone, initialization state and actor ownership restore exactly");
            exercised = true;
            std::fprintf(stderr, "[wall-probe] PASS terminal injected FileSelect row: original creator/init, line queries, appearance/death/invalidation/retirement and restored Gateway owners; frame=%llu\n",
                         static_cast<unsigned long long>(frame));
        }
    };
#endif

    void test_real_file_select_invisible_wall_collision_lifecycle() {
#ifdef NDEBUG
        require(false, "Original-process wall lifecycle diagnostic requires a debug build");
#else
        const auto disc_path = find_real_disc();
        if (!disc_path) {
            std::cout << "[skip] real wall lifecycle test (set SMGPC_REAL_DISC)\n";
            return;
        }
        auto pattern = (std::filesystem::temp_directory_path() / "petari-wall-process-XXXXXX").string();
        const auto* directory = mkdtemp(pattern.data());
        require(directory, "Wall process fixture requires a fresh private native console directory");
        const ScopedEnvironmentVariable save("SMGPC_SAVE_DIR", directory);
        const ScopedEnvironmentVariable nand("SMGPC_NAND_DIR", "");
        const ScopedEnvironmentVariable buttons("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "");
        const ScopedEnvironmentVariable pointer("SMGPC_DEBUG_WPAD_POINTER_SCRIPT", "");
        const ScopedEnvironmentVariable stick("SMGPC_DEBUG_WPAD_STICK_SCRIPT", "");
        const ScopedEnvironmentVariable input_file("SMGPC_DEBUG_WPAD_INPUT_FILE", "");
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original wall lifecycle fixture",
            .arguments = {"original-wall-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
            .disc_image = *disc_path,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct DiscLifetime { ~DiscLifetime() { smgpc::app::close_disc_image(); } } disc_lifetime;
        WallProbe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<WallProbe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised,
                "Actual original process completes the terminal wall fixture and bounded frame loop");
        require(!smgpc::compat::has_actor_runtime_state(probe.retired_actor) &&
                    !probe.retired_triangle.isValid() && probe.retired_triangle.getHostName() == nullptr,
                "Normal original-process teardown preserves retired wall identities and releases the scene collision owner");
#endif
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main(int, char**) {
    constexpr auto tests = std::array{
        TestCase{"factory-only placement support", test_factory_is_the_only_constructible_support_kind},
        TestCase{"DemoRabbit creator/archive callback atomicity", test_demo_rabbit_creator_and_placement_archive_callback_are_atomic},
        TestCase{"optional real-disc archive-only rejection", test_optional_real_disc_archive_presence_does_not_create_support},
        TestCase{"injected retail wall lifecycle under original Gateway process", test_real_file_select_invisible_wall_collision_lifecycle},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " NameObj placement test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " NameObj placement test(s) passed\n";
    return 0;
}
