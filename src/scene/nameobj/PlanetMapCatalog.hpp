#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace smgpc::runtime {
    class DvdFileSystemService;
}  // namespace smgpc::runtime

namespace smgpc::scene::nameobj {

    enum class PlanetMapSubmodelKind : std::size_t {
        Low,
        Middle,
        Bloom,
        Water,
        Indirect,
        Count,
    };

    enum class PlanetMapCatalogCreatorKind {
        OrdinaryPlanetMap,
        ForceLowRuntimeUnavailable,
        UniqueCreatorRuntimeUnavailable,
        OptionalSubmodelsRuntimeUnavailable,
    };

    struct PlanetMapCatalogEntry {
        std::size_t source_row = 0U;
        std::string planet_name;
        std::array<std::int32_t, static_cast<std::size_t>(PlanetMapSubmodelKind::Count)>
            authored_submodel_flags{};
        std::array<std::optional<std::string>, static_cast<std::size_t>(PlanetMapSubmodelKind::Count)>
            submodel_names{};
        std::array<std::string, 8U> force_low_scenarios{};
        PlanetMapCatalogCreatorKind creator_kind = PlanetMapCatalogCreatorKind::OrdinaryPlanetMap;

        [[nodiscard]] bool has_authored_submodels() const noexcept;
        [[nodiscard]] bool has_retained_submodels() const noexcept;
        [[nodiscard]] bool has_force_low_scenarios() const noexcept;
        [[nodiscard]] bool requires_scenario_selected_archive_load() const
            noexcept;
        [[nodiscard]] const std::optional<std::string> &submodel_name(PlanetMapSubmodelKind kind) const;
    };

    // Scene-owned view of PlanetMapDataTable. The active pointer exists only to
    // bridge the original global PlanetMapCreator lookup boundary; ownership and
    // all parsed strings remain with the current scene's catalog instance.
    class PlanetMapCatalog final {
    public:
        explicit PlanetMapCatalog(smgpc::runtime::DvdFileSystemService &dvd);
        ~PlanetMapCatalog();

        PlanetMapCatalog(const PlanetMapCatalog &) = delete;
        PlanetMapCatalog &operator=(const PlanetMapCatalog &) = delete;
        PlanetMapCatalog(PlanetMapCatalog &&) = delete;
        PlanetMapCatalog &operator=(PlanetMapCatalog &&) = delete;

        [[nodiscard]] static PlanetMapCatalog *active() noexcept;
        [[nodiscard]] std::size_t source_row_count() const noexcept;
        [[nodiscard]] std::span<const std::size_t> blank_source_rows() const noexcept;
        [[nodiscard]] std::span<const PlanetMapCatalogEntry> entries() const noexcept;
        [[nodiscard]] const PlanetMapCatalogEntry *find(std::string_view planet_name) const;
        [[nodiscard]] bool is_ordinary_planet(std::string_view planet_name) const;
        // Exact PlanetMapCreatorFunction archive-timing predicate: a
        // registered row is deferred when any of its eight raw scenario cells
        // is nonempty. This is intentionally distinct from scenario force-low
        // selection semantics.
        [[nodiscard]] bool requires_scenario_selected_archive_load(
            std::string_view planet_name) const;
        [[nodiscard]] std::vector<std::string> archive_names(const PlanetMapCatalogEntry &entry) const;
        [[nodiscard]] std::vector<std::string> archive_names(std::string_view planet_name) const;

    private:
        std::size_t _source_row_count = 0U;
        std::vector<std::size_t> _blank_source_rows;
        std::vector<PlanetMapCatalogEntry> _entries;
        std::unordered_map<std::string, std::size_t> _indices;
    };

}  // namespace smgpc::scene::nameobj
