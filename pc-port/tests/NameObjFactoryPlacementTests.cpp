#include "Game/NameObj/NameObj.hpp"
#include "runtime/RuntimeServices.hpp"
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
#include <utility>

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

        require(smgpc::scene::nameobj::can_create_name_obj("Coin"),
                "an object in the real NameObjFactory table should be constructible");
        require(!smgpc::scene::nameobj::can_create_name_obj(cUnsupportedFixture),
                "the synthetic unsupported fixture must remain absent from the real factory table");

        const auto factory = smgpc::scene::nameobj::describe_name_obj_placement_support(
            dvd, "Coin", "jmp/placement/common/objinfo");
        require(factory.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::OriginalFactory &&
                    factory.reason == "original_factory",
                "ordinary placement support should come only from the real factory");

        const auto unsupported = smgpc::scene::nameobj::describe_name_obj_placement_support(
            dvd, cUnsupportedFixture, "jmp/placement/common/objinfo");
        require(unsupported.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::Unsupported &&
                    unsupported.reason == "no_original_factory",
                "an ordinary placement without a real creator should be unsupported");

        const auto helper = smgpc::scene::nameobj::describe_name_obj_placement_support(
            dvd, cUnsupportedFixture, "jmp/placement/common/stageobjinfo");
        require(helper.kind == smgpc::scene::nameobj::NameObjPlacementSupportKind::IntentionallyIgnored,
                "non-renderable helper tables should remain intentionally ignored");

        const auto factory_archives = smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, "Coin");
        require(factory_archives.size() == 1U && factory_archives.front().archive_name == "Coin",
                "real creators should retain their original archive-list metadata even without a mounted disc");
        require(smgpc::scene::nameobj::collect_name_obj_archive_requests(dvd, cUnsupportedFixture).empty(),
                "unsupported objects should not produce preload requests");

        require_throws(
            [&] {
                (void)smgpc::scene::nameobj::create_name_obj(dvd, cUnsupportedFixture, cUnsupportedFixture);
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
        if (!direct_archive_only_name.empty()) {
            auto unsupported_placement = smgpc::scene::StagePlacementObject{};
            unsupported_placement.object_name = direct_archive_only_name;
            unsupported_placement.object_archive_path = direct_archive_only_path.generic_string();
            unsupported_placement.factory_supported = false;
            const auto stats = collision.load(dvd, std::array{unsupported_placement});
            require(stats.placement_count == 0U && stats.archive_count == 0U && stats.mesh_count == 0U,
                    "an unsupported placement must not be counted or contribute invisible archive-backed collision");
        }

        const auto coin_archive = dvd.find_object_archive("Coin");
        require(coin_archive.has_value(), "the real disc fixture should contain Coin.arc");
        auto factory_placement = smgpc::scene::StagePlacementObject{};
        factory_placement.object_name = "Coin";
        factory_placement.object_archive_path = coin_archive->generic_string();
        factory_placement.factory_supported = true;
        const auto factory_stats = collision.load(dvd, std::array{factory_placement});
        require(factory_stats.placement_count == 1U && factory_stats.archive_count == 1U,
                "collision loading should continue to inspect archives for real factory placements");
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
