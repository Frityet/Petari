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
#include "Game/Util/SceneUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/HitInfoCompat.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "runtime/RuntimeServices.hpp"
#include "resource/BcsvTable.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/PlacementZoneNameScope.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StageHostScene.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
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

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
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
            std::string_view{"RestartCube"},
            std::string_view{"Steam"},
            std::string_view{"Coin"},
            std::string_view{"PurpleCoin"},
            std::string_view{"RailCoin"},
            std::string_view{"PurpleRailCoin"},
            std::string_view{"PurpleCoinStarter"},
            std::string_view{"StarPieceFlow"},
            std::string_view{"StarPieceGroup"},
        };
        for (const auto name : cUnavailableCreators) {
            const auto support = smgpc::scene::nameobj::describe_name_obj_creator_support(name);
            require(support.kind == smgpc::scene::nameobj::NameObjCreatorSupportKind::RuntimeClosureUnavailable &&
                        !support.reason.empty() && NameObjFactory::getCreator(std::string(name).c_str()) == nullptr &&
                        smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, name).empty(),
                    "a retail creator with a mandatory unavailable init dependency must remain absent");
        }
        require(
            smgpc::scene::nameobj::describe_name_obj_creator_support("RestartCube").reason ==
                "real_mario_update_and_restart_dispatch_runtime_unavailable",
            "RestartCube must identify its real Mario update/restart-dispatch closure as the remaining blocker");

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
        require(fixed_then_aliases.mCount ==
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
            require(collector.mCount == 1 &&
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

        const auto placements = smgpc::scene::resolve_stage_placement_objects(dvd, "HeavensDoorGalaxy", 1);
        const auto passive_area_placement = std::ranges::find_if(placements, [](const auto &placement) {
            if (smgpc::scene::find_complete_area_obj_placement_descriptor(placement.object_name) == nullptr) {
                return false;
            }
            for (const auto *field : {"FollowId", "SW_A", "SW_B", "SW_APPEAR", "SW_DEAD", "SW_SLEEP"}) {
                auto value = s32{-1};
                if (placement.jmap_info.getValue(placement.jmap_entry_index, field, &value) && value >= 0) {
                    return false;
                }
            }
            return true;
        });
        require(passive_area_placement != placements.end(),
                "Gateway must retain a passive placement for the completed AreaObj subset");
        {
            auto holder = SceneObjHolder{};
            auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            require(holder.create(SceneObj_AreaObjContainer) != nullptr,
                    "the exact placement test requires its scene-owned retail AreaObj container");

            auto object = smgpc::scene::nameobj::create_name_obj(
                dvd, passive_area_placement->object_name, "localized actor label");
            auto *area = dynamic_cast<AreaObj *>(object.get());
            require(area != nullptr, "the completed passive descriptor must construct its exact AreaObj type");
            object->init(JMapInfoIter(
                &passive_area_placement->jmap_info, passive_area_placement->jmap_entry_index));

            const auto *descriptor = smgpc::scene::find_complete_area_obj_placement_descriptor(
                passive_area_placement->object_name);
            auto *manager = MR::getAreaObjContainer()->getManager(descriptor->manager_name.data());
            require(std::string_view(object->getName()) == passive_area_placement->object_name &&
                        manager->getAreaObj(0) == area,
                    "exact placement init must apply JMap naming before entering the canonical retail manager");
            binding.init_after_placement();
        }
        const auto archive_only_placement = std::ranges::find_if(placements, [](const auto &placement) {
            return !placement.intentionally_ignored && !placement.factory_supported &&
                   !placement.object_archive_path.empty();
        });
        if (archive_only_placement != placements.end()) {
            require(archive_only_placement->support_kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::Unsupported,
                    "the Gateway resolver should retain an archive-backed non-factory row only as unsupported diagnostics");
        } else {
            std::cout << "[skip] Gateway has no remaining archive-backed non-factory ordinary placement\n";
        }
        const auto roots = smgpc::scene::resolve_stage_root_placements(dvd, "HeavensDoorGalaxy", 1);
        require(!roots.empty() && std::ranges::all_of(roots, [](const auto &placement) {
            return placement.factory_supported &&
                   placement.support_kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::OriginalFactory;
        }),
                "the resolved stage root list should contain only real factory placements");

        auto collision = smgpc::scene::StageCollisionService{};
        collision.clear();
        collision.build();
        if (!direct_archive_only_name.empty()) {
            require(collision.empty() && collision.stats().mesh_count == 0U &&
                        collision.stats().triangle_count == 0U,
                    "an unsupported placement must remain absent from the explicit collision registry");
        }

        const auto coin_archive = dvd.find_object_archive("Coin");
        require(coin_archive.has_value(), "the real disc fixture should contain Coin.arc");
        require(collision.empty() && collision.stats().mesh_count == 0U &&
                    collision.stats().triangle_count == 0U,
                "a real factory archive must remain absent until Game/CollisionParts explicitly registers KCL");
    }

    [[nodiscard]] bool line_query_hits_registered_wall(
        const smgpc::scene::StageCollisionService &collision,
        smgpc::scene::StageCollisionHit *hit = nullptr) {
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
                        if (collision.line_cast(start, offset, hit)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    void test_real_file_select_invisible_wall_collision_lifecycle() {
        const auto disc_path = find_real_disc();
        if (!disc_path.has_value()) {
            std::cout << "[skip] real FileSelect InvisiblePolygonObj test (set SMGPC_REAL_DISC or place RMGK01.iso in a workspace ancestor)\n";
            return;
        }

        aurora_dvd_close();
        const auto disc_path_string = disc_path->string();
        require(aurora_dvd_open(disc_path_string.c_str()),
                "the selected real-disc fixture should be a readable SMG image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        const auto placements =
            smgpc::scene::resolve_stage_placement_objects(dvd, "FileSelect", 1);
        require(placements.size() == 4U,
                "retail FileSelect scenario 1 must retain its four actor-bearing rows");
        const auto wall = std::ranges::find_if(placements, [](const auto &placement) {
            return placement.object_name == "InvisibleWall10x10";
        });
        require(wall != placements.end() && wall->factory_supported &&
                    wall->object_archive_path == "/ObjectData/InvisibleWall10x10.arc",
                "the real FileSelect wall row must resolve through its exact creator and archive");
        const auto blocked_count = std::ranges::count_if(placements, [](const auto &placement) {
            return !placement.intentionally_ignored && !placement.factory_supported;
        });
        require(blocked_count == 2U,
                "FileSelect must retain only FileSelector and SphereSelectorHandle as blocked rows");

#ifndef NDEBUG
        const auto report_path = std::filesystem::temp_directory_path() /
                                 "smgpc-file-select-invisible-wall-preflight.md";
        {
            const auto report_environment = ScopedEnvironmentVariable(
                "SMGPC_STAGE_PLACEMENT_REPORT_PATH", report_path.string());
            require_throws(
                [&] {
                    smgpc::scene::preflight_stage_placements_or_throw(
                        "FileSelect", 1, placements);
                },
                "strict FileSelect preflight must remain red for its two real blockers");
        }
        auto report_stream = std::ifstream(report_path);
        const auto report = std::string(std::istreambuf_iterator<char>(report_stream),
                                        std::istreambuf_iterator<char>());
        std::filesystem::remove(report_path);
        require(report.find("total_objects: 4\n") != std::string::npos &&
                    report.find("complete_objects: 2\n") != std::string::npos &&
                    report.find("blocked_objects: 2\n") != std::string::npos &&
                    report.find("- status: complete\n  object: InvisibleWall10x10\n") !=
                        std::string::npos &&
                    report.find("created_objects:") == std::string::npos,
                "strict preflight must report the exact 2-blocker closure without fabricating roots");
#endif

        auto resource_holders = smgpc::compat::ResourceHolderService{dvd};
        auto *wall_resources =
            resource_holders.create_and_add("InvisibleWall10x10.arc");
        require(wall_resources->resource_data("InvisibleWall10x10.kcl").size() == 1222U &&
                    wall_resources->resource_data("InvisibleWall10x10.pa").size() == 96U &&
                    wall_resources->resource_data("CollisionVersion").size() == 7U,
                "the ResourceHolder must expose the real RMGK01 KCL, attribute, and version resources");

        auto collision = smgpc::scene::StageCollisionService{};
        collision.clear();
        collision.build();
        collision.activate();
        require(collision.empty(),
                "strict preflight must not synthesize collision before exact actor construction");

        auto scene_objects = SceneObjHolder{};
        auto scene_binding = smgpc::scene::SceneObjHolderBinding(scene_objects);
        require(scene_objects.create(SceneObj_PlacementStateChecker) != nullptr,
                "collision construction requires the original placement-zone state owner");
        auto object = smgpc::scene::nameobj::create_name_obj(
            dvd, wall->object_name, wall->object_name.c_str());
        auto *actor = dynamic_cast<InvisiblePolygonObj *>(object.get());
        require(actor != nullptr, "the real FileSelect row must construct InvisiblePolygonObj");
        const auto iter = JMapInfoIter(&wall->jmap_info, wall->jmap_entry_index);
        const auto archives = smgpc::scene::nameobj::preload_name_obj_archives(
            dvd, wall->object_name, &iter);
        require(archives.size() == 1U && archives.front().loaded,
                "the exact wall lifecycle must preload its real retail archive");
        {
            const auto zone_scope = smgpc::scene::PlacementZoneNameScope(wall->zone_id, wall->zone_name);
            actor->init(iter);
        }
        require(MR::getCurrentPlacementZoneId() == -1,
                "the collision fixture must finish its construction zone scope before querying provenance");
        require(smgpc::compat::has_actor_collision_parts(actor) &&
                    smgpc::compat::actor_collision_parts_source(actor).find(
                        "InvisibleWall10x10.arc") != std::string_view::npos &&
                    MR::getCollisionBoundingSphereRange(actor) > 0.0F,
                "exact actor init must retain real CollisionParts ownership and clipping bounds");
        require(!line_query_hits_registered_wall(collision),
                "registered KCL must remain unqueryable before the generic post-placement build");

        actor->initAfterPlacement();
        collision.build();
        auto hit = smgpc::scene::StageCollisionHit{};
        require(collision.stats().mesh_count == 1U &&
                    collision.stats().triangle_count > 0U &&
                    line_query_hits_registered_wall(collision, &hit),
                "the post-placement build must expose the exact wall KCL to map queries");
        const auto triangle = smgpc::compat::make_collision_triangle(collision, hit.triangle_index);
        require(triangle.getHostName() == actor->mName &&
                    triangle.getHostPlacementZoneID() == wall->zone_id &&
                    collision.surface(hit.triangle_index)->source_name ==
                        smgpc::compat::actor_collision_parts_source(actor),
                "the original actor's CollisionParts must retain its host/zone separately from the exact KCL path");

        actor->makeActorDead();
        require(!line_query_hits_registered_wall(collision),
                "dead CollisionParts owners must stop contributing map collision");
        actor->makeActorAppeared();
        require(line_query_hits_registered_wall(collision),
                "reappeared CollisionParts owners must restore their retained map collision");
        object.reset();
        require(!line_query_hits_registered_wall(collision) &&
                    !triangle.isValid() && triangle.getHostName() == nullptr,
                "destroyed CollisionParts owners must invalidate retained BVH registrations");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"factory-only placement support", test_factory_is_the_only_constructible_support_kind},
        TestCase{"DemoRabbit creator/archive callback atomicity", test_demo_rabbit_creator_and_placement_archive_callback_are_atomic},
        TestCase{"optional real-disc archive-only rejection", test_optional_real_disc_archive_presence_does_not_create_support},
        TestCase{"real FileSelect invisible wall collision lifecycle", test_real_file_select_invisible_wall_collision_lifecycle},
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
