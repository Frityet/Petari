#include "scene/nameobj/PlanetMapCatalog.hpp"

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace smgpc::scene::nameobj {
    namespace {

        constexpr auto cPlanetMapTableArchivePath = std::string_view{"ObjectData/PlanetMapDataTable.arc"};
        constexpr auto cPlanetMapTableFileName = std::string_view{"PlanetMapDataTable.bcsv"};

        constexpr auto cSubmodelFlagNames = std::to_array<std::string_view>({
            "LowFlag",
            "MiddleFlag",
            "BloomFlag",
            "WaterFlag",
            "IndirectFlag",
        });

        constexpr auto cSubmodelSuffixes = std::to_array<std::string_view>({
            "Low",
            "Middle",
            "Bloom",
            "Water",
            "Indirect",
        });

        constexpr auto cUniquePlanetNames = std::to_array<std::string_view>({
            "BeamGoRoundPlanet",
            "BumpAppearPlanet",
            "ChoConveyorPlanetB",
            "ChoConveyorPlanetD",
            "DinoPackunBattlePlanet",
            "DarkHopperPlanetA",
            "DarkHopperPlanetB",
            "DarkHopperPlanetC",
            "DarkHopperPlanetD",
            "DarknessRoomPlanet",
            "FlagDiscPlanetB",
            "FlagDiscPlanetC",
            "FlagDiscPlanetD",
            "FlagDiscPlanetE",
            "HatchWaterPlanet",
            "HeavensDoorInsidePlanet",
            "HoneyQueenPlanet",
            "LavaJamboSunPlanet",
            "LavaRotatePlanet",
            "LavaDomedPlanet",
            "MarblePlanet",
            "PeachCastleGardenPlanet",
            "SandCapsuleInsidePlanet",
            "ScaleDownRelayPlanet",
            "SnowCapsulePlanet",
            "TeresaRoomPlanet",
            "TridentPlanet",
            "QuestionBoxPlanetA",
            "QuestionBoxPlanetB",
            "QuestionBoxPlanetC",
            "QuestionBoxPlanetD",
            "QuestionBoxPlanetE",
            "Quicksand2DPlanet",
            "ReverseGravityRoomPlanet",
            "SandStreamHighTowerPlanet",
            "SandStreamJointPlanetA",
            "SandStreamJointPlanetB",
            "StarDustStartPlanet",
            "WormEatenPlanet",
        });

        struct UniqueArchiveName {
            std::string_view planet_name;
            std::string_view archive_name;
        };

        constexpr auto cUniqueArchiveNames = std::to_array<UniqueArchiveName>({
            {"BeamGoRoundPlanet", "BeamGoRoundBeam"},
            {"BeamGoRoundPlanet", "BeamGoRoundBeamVolume"},
            {"BeamGoRoundPlanet", "BeamGoRoundBeamBloom"},
            {"MarblePlanet", "MarblePlanetCore"},
            {"MarblePlanet", "MarblePlanetElectron"},
            {"MarblePlanet", "MarblePlanetElectronShadow"},
            {"OceanRingPlanet", "OceanRingPlanetLowInWater"},
            {"WormEatenPlanet", "GreenCaterpillarBigFace"},
            {"WormEatenPlanet", "GreenCaterpillarBigFaceLow"},
            {"WormEatenPlanet", "GreenCaterpillarBigBody"},
            {"WormEatenPlanet", "GreenCaterpillarBigBodyLow"},
            {"WormEatenPlanet", "WormEatenHill"},
            {"WormEatenPlanet", "WormEatenPlanetLow"},
        });

        PlanetMapCatalog *sActiveCatalog = nullptr;

        [[nodiscard]] bool is_string_field(smgpc::resource::BcsvFieldType type) {
            return type == smgpc::resource::BcsvFieldType::InlineString ||
                   type == smgpc::resource::BcsvFieldType::StringOffset;
        }

        [[nodiscard]] bool is_integer_field(smgpc::resource::BcsvFieldType type) {
            return type == smgpc::resource::BcsvFieldType::Int32 ||
                   type == smgpc::resource::BcsvFieldType::UInt32 ||
                   type == smgpc::resource::BcsvFieldType::Int16 ||
                   type == smgpc::resource::BcsvFieldType::Int8;
        }

        void require_field(const smgpc::resource::BcsvTable &table, std::string_view field_name,
                           bool (*accepts)(smgpc::resource::BcsvFieldType), std::string_view expected_type) {
            const auto index = table.field_index(field_name);
            if (!index.has_value()) {
                throw std::runtime_error("PlanetMapDataTable.bcsv is missing required field " +
                                         std::string(field_name));
            }
            if (!accepts(table.fields()[*index].type)) {
                throw std::runtime_error("PlanetMapDataTable.bcsv field " + std::string(field_name) +
                                         " must be " + std::string(expected_type));
            }
        }

        [[nodiscard]] std::string force_low_field_name(std::size_t index) {
            return "ForceLowScenarioName" + std::to_string(index);
        }

        [[nodiscard]] bool is_unique_planet(std::string_view planet_name) {
            return std::ranges::find(cUniquePlanetNames, planet_name) != cUniquePlanetNames.end();
        }

        void append_unique(std::vector<std::string> &names, std::string_view name) {
            const auto found = std::ranges::find_if(names, [name](const auto &candidate) {
                return candidate == name;
            });
            if (found == names.end()) {
                names.emplace_back(name);
            }
        }

    }  // namespace

    bool PlanetMapCatalogEntry::has_authored_submodels() const noexcept {
        return std::ranges::any_of(authored_submodel_flags, [](std::int32_t flag) {
            return flag != 0;
        });
    }

    bool PlanetMapCatalogEntry::has_retained_submodels() const noexcept {
        return std::ranges::any_of(submodel_names, [](const auto &name) {
            return name.has_value();
        });
    }

    bool PlanetMapCatalogEntry::has_force_low_scenarios() const noexcept {
        // Retail scans slots in order and treats the literal "Low" as an
        // immediate end sentinel. Only authored scenario names in the prefix
        // before that sentinel can ever select PlanetMapWithoutHighModel.
        for (const auto &scenario : force_low_scenarios) {
            if (scenario == "Low") {
                return false;
            }
            if (!scenario.empty()) {
                return true;
            }
        }
        return false;
    }

    const std::optional<std::string> &PlanetMapCatalogEntry::submodel_name(PlanetMapSubmodelKind kind) const {
        const auto index = static_cast<std::size_t>(kind);
        if (index >= submodel_names.size()) {
            throw std::out_of_range("PlanetMap submodel kind is outside the authored table");
        }
        return submodel_names[index];
    }

    PlanetMapCatalog::PlanetMapCatalog(smgpc::runtime::DvdFileSystemService &dvd) {
        if (sActiveCatalog != nullptr) {
            throw std::runtime_error("A PlanetMapCatalog is already active for the current scene");
        }

        const auto &archive = dvd.archive(cPlanetMapTableArchivePath);
        const auto *table_entry = archive.find_resource(cPlanetMapTableFileName);
        if (table_entry == nullptr) {
            throw std::runtime_error("PlanetMapDataTable.arc does not contain PlanetMapDataTable.bcsv");
        }

        const auto table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*table_entry));
        require_field(table, "PlanetName", is_string_field, "a string field");
        for (const auto field_name : cSubmodelFlagNames) {
            require_field(table, field_name, is_integer_field, "an integer field");
        }
        for (auto index = std::size_t{}; index < 8U; ++index) {
            require_field(table, force_low_field_name(index), is_string_field, "a string field");
        }

        _source_row_count = table.entry_count();
        _entries.reserve(table.entry_count());
        _indices.reserve(table.entry_count());
        for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
            const auto raw_planet_name = table.get_string(row, "PlanetName");
            if (!raw_planet_name.has_value()) {
                throw std::runtime_error("PlanetMapDataTable.bcsv row " + std::to_string(row) +
                                         " has no PlanetName");
            }

            auto entry = PlanetMapCatalogEntry{};
            entry.source_row = row;
            entry.planet_name = smgpc::resource::decode_cp932(*raw_planet_name);

            for (auto index = std::size_t{}; index < cSubmodelFlagNames.size(); ++index) {
                const auto flag = table.get_s32(row, cSubmodelFlagNames[index]);
                if (!flag.has_value()) {
                    throw std::runtime_error("PlanetMapDataTable.bcsv row " + std::to_string(row) +
                                             " has no integer " + std::string(cSubmodelFlagNames[index]));
                }
                entry.authored_submodel_flags[index] = *flag;
                if (*flag == 0 || entry.planet_name.empty()) {
                    continue;
                }

                auto submodel_name = entry.planet_name + std::string(cSubmodelSuffixes[index]);
                if (dvd.find_object_archive(submodel_name).has_value()) {
                    entry.submodel_names[index] = std::move(submodel_name);
                }
            }

            for (auto index = std::size_t{}; index < entry.force_low_scenarios.size(); ++index) {
                const auto field_name = force_low_field_name(index);
                const auto raw_scenario = table.get_string(row, field_name);
                if (!raw_scenario.has_value()) {
                    throw std::runtime_error("PlanetMapDataTable.bcsv row " + std::to_string(row) +
                                             " has no string " + field_name);
                }
                entry.force_low_scenarios[index] = smgpc::resource::decode_cp932(*raw_scenario);
            }

            if (entry.planet_name.empty()) {
                if (entry.has_authored_submodels() ||
                    std::ranges::any_of(entry.force_low_scenarios,
                                        [](const auto &scenario) { return !scenario.empty(); })) {
                    throw std::runtime_error(
                        "PlanetMapDataTable.bcsv row " + std::to_string(row) +
                        " is unnamed but retains submodel flags or force-low scenarios");
                }
                _blank_source_rows.push_back(row);
                continue;
            }
            if (_indices.contains(entry.planet_name)) {
                throw std::runtime_error("PlanetMapDataTable.bcsv contains duplicate PlanetName " +
                                         entry.planet_name);
            }

            // Force-low selection is performed before the unique creator table in
            // retail. Within the non-force-low path, a unique name must never fall
            // through to the ordinary PlanetMap creator, even when it has no extras.
            if (entry.has_force_low_scenarios()) {
                entry.creator_kind = PlanetMapCatalogCreatorKind::ForceLowRuntimeUnavailable;
            } else if (is_unique_planet(entry.planet_name)) {
                entry.creator_kind = PlanetMapCatalogCreatorKind::UniqueCreatorRuntimeUnavailable;
            } else if (entry.has_retained_submodels()) {
                entry.creator_kind = PlanetMapCatalogCreatorKind::OptionalSubmodelsRuntimeUnavailable;
            }

            const auto index = _entries.size();
            _entries.push_back(std::move(entry));
            _indices.emplace(_entries.back().planet_name, index);
        }

        sActiveCatalog = this;
    }

    PlanetMapCatalog::~PlanetMapCatalog() {
        if (sActiveCatalog == this) {
            sActiveCatalog = nullptr;
        }
    }

    PlanetMapCatalog *PlanetMapCatalog::active() noexcept {
        return sActiveCatalog;
    }

    std::size_t PlanetMapCatalog::source_row_count() const noexcept {
        return _source_row_count;
    }

    std::span<const std::size_t> PlanetMapCatalog::blank_source_rows() const noexcept {
        return _blank_source_rows;
    }

    std::span<const PlanetMapCatalogEntry> PlanetMapCatalog::entries() const noexcept {
        return _entries;
    }

    const PlanetMapCatalogEntry *PlanetMapCatalog::find(std::string_view planet_name) const {
        const auto found = _indices.find(std::string(planet_name));
        return found == _indices.end() ? nullptr : &_entries[found->second];
    }

    bool PlanetMapCatalog::is_ordinary_planet(std::string_view planet_name) const {
        const auto *entry = find(planet_name);
        return entry != nullptr && entry->creator_kind == PlanetMapCatalogCreatorKind::OrdinaryPlanetMap;
    }

    std::vector<std::string> PlanetMapCatalog::archive_names(const PlanetMapCatalogEntry &entry) const {
        auto names = std::vector<std::string>{};
        names.reserve(1U + entry.submodel_names.size());
        append_unique(names, entry.planet_name);
        for (const auto &submodel_name : entry.submodel_names) {
            if (submodel_name.has_value()) {
                append_unique(names, *submodel_name);
            }
        }
        for (const auto &unique : cUniqueArchiveNames) {
            if (entry.planet_name == unique.planet_name) {
                append_unique(names, unique.archive_name);
            }
        }
        return names;
    }

    std::vector<std::string> PlanetMapCatalog::archive_names(std::string_view planet_name) const {
        const auto *entry = find(planet_name);
        return entry != nullptr ? archive_names(*entry) : std::vector<std::string>{};
    }

}  // namespace smgpc::scene::nameobj
