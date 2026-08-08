#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Map/FileSelector.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

static_assert(std::is_base_of_v<LiveActor, FileSelector>,
              "the exact retail FileSelector declaration must coexist with the host boundary");

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
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
            std::string_view{"DemoRabbit"},
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

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"factory-only placement support", test_factory_is_the_only_constructible_support_kind},
        TestCase{"optional real-disc archive-only rejection", test_optional_real_disc_archive_presence_does_not_create_support},
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
