#include "runtime/RuntimeServices.hpp"
#include "scene/nameobj/PlanetMapCatalog.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

    [[nodiscard]] std::filesystem::path require_real_disc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            require(std::filesystem::is_regular_file(path),
                    "SMGPC_REAL_DISC must name the real RMGK01 image");
            return path;
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        require(!error, "the PlanetMap catalog proof requires a readable working directory");
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
        throw std::runtime_error(
            "the PlanetMap catalog proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    [[nodiscard]] const smgpc::scene::nameobj::PlanetMapCatalogEntry &require_entry(
        const smgpc::scene::nameobj::PlanetMapCatalog &catalog, std::string_view name) {
        const auto *entry = catalog.find(name);
        require(entry != nullptr, std::string("PlanetMapDataTable is missing ") + std::string(name));
        return *entry;
    }

    [[nodiscard]] std::string format_force_low_slots(
        const std::array<std::string, 8U> &slots) {
        auto result = std::string{"["};
        for (auto index = std::size_t{}; index < slots.size(); ++index) {
            if (index != 0U) {
                result += ',';
            }
            result += '"';
            result += slots[index];
            result += '"';
        }
        result += ']';
        return result;
    }

    void test_force_low_prefix_semantics() {
        auto entry = smgpc::scene::nameobj::PlanetMapCatalogEntry{};
        require(!entry.has_force_low_scenarios(),
                "eight empty force-low cells must not block an ordinary row");

        entry.force_low_scenarios = {"Low", "UnreachableGalaxy_1"};
        require(!entry.has_force_low_scenarios(),
                "the retail Low sentinel must terminate the force-low prefix");

        entry.force_low_scenarios = {"", "AuthoredGalaxy_1", "Low"};
        require(entry.has_force_low_scenarios(),
                "a scenario name before the Low sentinel must retain force-low precedence");
    }

    void test_real_planet_map_catalog() {
        using smgpc::scene::nameobj::PlanetMapCatalog;
        using smgpc::scene::nameobj::PlanetMapCatalogCreatorKind;
        using smgpc::scene::nameobj::PlanetMapSubmodelKind;

        const auto disc_path = require_real_disc();
        aurora_dvd_close();
        const auto disc_string = disc_path.string();
        require(aurora_dvd_open(disc_string.c_str()),
                "the selected RMGK01 image must open through Aurora DVD");
        struct DiscCloseGuard final {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } disc_close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        require(PlanetMapCatalog::active() == nullptr,
                "no PlanetMap catalog may leak across scene lifetimes");
        {
            const auto catalog = PlanetMapCatalog{dvd};
            require(PlanetMapCatalog::active() == &catalog,
                    "the scene-owned PlanetMap catalog must expose its active lookup boundary");
            require(catalog.source_row_count() == 258U,
                    "RMGK01 PlanetMapDataTable must retain all 258 source rows");
            const auto expected_blank_rows = std::array<std::size_t, 2U>{150U, 188U};
            require(catalog.entries().size() == 256U &&
                        std::ranges::equal(catalog.blank_source_rows(), expected_blank_rows),
                    "RMGK01 PlanetMapDataTable must retain its exact two fully blank source rows");
            require_throws(
                [&] {
                    (void)PlanetMapCatalog{dvd};
                },
                "a second catalog must not replace the active scene-owned catalog");
            require(PlanetMapCatalog::active() == &catalog,
                    "a failed catalog construction must leave the original active binding intact");

            const auto &gateway = require_entry(catalog, "HeavensDoorMysteriousPlanet");
            const auto empty_force_low_slots = std::array<std::string, 8U>{};
            require(gateway.source_row == 209U && !gateway.has_authored_submodels() &&
                        !gateway.has_retained_submodels() && !gateway.has_force_low_scenarios() &&
                        gateway.force_low_scenarios == empty_force_low_slots &&
                        gateway.creator_kind == PlanetMapCatalogCreatorKind::OrdinaryPlanetMap &&
                        catalog.is_ordinary_planet(gateway.planet_name),
                    "Gateway's mysterious planet must be a zero-submodel ordinary PlanetMap row");
            require(catalog.archive_names(gateway) ==
                        std::vector<std::string>{"HeavensDoorMysteriousPlanet"},
                    "the ordinary Gateway planet must request only its same-name archive");

            const auto &inside = require_entry(catalog, "HeavensDoorInsidePlanet");
            require(inside.source_row == 213U && !inside.has_retained_submodels() &&
                        inside.creator_kind ==
                            PlanetMapCatalogCreatorKind::UniqueCreatorRuntimeUnavailable &&
                        !catalog.is_ordinary_planet(inside.planet_name),
                    "a zero-submodel unique name must not fall through to ordinary PlanetMap");

            const auto &unique = require_entry(catalog, "BeamGoRoundPlanet");
            require(unique.source_row == 1U &&
                        unique.creator_kind ==
                            PlanetMapCatalogCreatorKind::UniqueCreatorRuntimeUnavailable &&
                        catalog.archive_names(unique) == std::vector<std::string>{
                            "BeamGoRoundPlanet",
                            "BeamGoRoundBeam",
                            "BeamGoRoundBeamVolume",
                            "BeamGoRoundBeamBloom",
                        },
                    "unique creator precedence and its exact extra archive list must be retained");

            const auto &optional = require_entry(catalog, "AsteroidBlockPlanet");
            require(optional.source_row == 0U && optional.has_authored_submodels() &&
                        optional.submodel_name(PlanetMapSubmodelKind::Low).has_value() &&
                        *optional.submodel_name(PlanetMapSubmodelKind::Low) ==
                            "AsteroidBlockPlanetLow" &&
                        optional.creator_kind ==
                            PlanetMapCatalogCreatorKind::OptionalSubmodelsRuntimeUnavailable &&
                        catalog.archive_names(optional) == std::vector<std::string>{
                            "AsteroidBlockPlanet",
                            "AsteroidBlockPlanetLow",
                        },
                    "a retained Low submodel row must remain unavailable in tranche one");

            const auto &force_low = require_entry(catalog, "DandelionHillPlanet");
            const auto dandelion_force_low_slots = std::array<std::string, 8U>{
                "CosmosGardenGalaxy_3", "", "", "", "", "", "", "",
            };
            require(force_low.source_row == 16U && force_low.has_force_low_scenarios() &&
                        force_low.force_low_scenarios == dandelion_force_low_slots &&
                        force_low.creator_kind ==
                            PlanetMapCatalogCreatorKind::ForceLowRuntimeUnavailable &&
                        !catalog.is_ordinary_planet(force_low.planet_name),
                    "a force-low-capable row must stay unavailable before creator selection");

            require(catalog.find("heavensdoormysteriousplanet") == nullptr &&
                        catalog.find("NotAPlanet") == nullptr &&
                        catalog.archive_names("NotAPlanet").empty(),
                    "catalog lookup must preserve retail exact-name matching and absence");

            std::cout << "[proof] disc=" << disc_path.string()
                      << ";source_rows=" << catalog.source_row_count()
                      << ";named_entries=" << catalog.entries().size()
                      << ";blank_rows=150,188"
                      << ";ordinary_gateway=HeavensDoorMysteriousPlanet"
                      << ";optional_blocked=AsteroidBlockPlanet"
                      << ";force_low_blocked=DandelionHillPlanet"
                      << ";unique_blocked=HeavensDoorInsidePlanet"
                      << ";gateway_force_low_raw="
                      << format_force_low_slots(gateway.force_low_scenarios)
                      << ";dandelion_force_low_raw="
                      << format_force_low_slots(force_low.force_low_scenarios) << '\n';
        }
        require(PlanetMapCatalog::active() == nullptr,
                "destroying the scene-owned catalog must clear its active lookup boundary");
    }

}  // namespace

int main() {
    try {
        test_force_low_prefix_semantics();
        test_real_planet_map_catalog();
        std::cout << "[ok] PlanetMap catalog bounds the ordinary zero-submodel tranche\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] PlanetMap catalog: " << error.what() << '\n';
        return 1;
    }
}
