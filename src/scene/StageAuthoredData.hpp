#pragma once

#include "scene/NameObjLifecycleService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::scene {

    // Owns every copied JMap row used by a stage session. Contexts returned by
    // this object remain valid until the StageAuthoredData is moved or destroyed.
    // Keeping this owner above placement actors prevents their retained
    // JMapInfoIter, child-object table, and rail references from becoming stale.
    class StageAuthoredData final {
    public:
        StageAuthoredData(std::string stage_name, s32 scenario_no,
                          std::vector<StagePlacementTable> tables,
                          std::vector<StagePlacementObject> placements,
                          std::vector<StageGeneralPos> general_positions,
                          std::optional<StageStartInfo> start_info,
                          std::vector<StageHolderOccurrence> holders = {});

        [[nodiscard]] static StageAuthoredData resolve(
            smgpc::runtime::DvdFileSystemService &dvd,
            std::string_view stage_name, s32 scenario_no, s32 start_id = 0,
            s32 start_zone_id = 0);

        StageAuthoredData(const StageAuthoredData &) = delete;
        StageAuthoredData &operator=(const StageAuthoredData &) = delete;
        StageAuthoredData(StageAuthoredData &&) noexcept = default;
        StageAuthoredData &operator=(StageAuthoredData &&) noexcept = default;

        [[nodiscard]] std::string_view stage_name() const noexcept;
        [[nodiscard]] s32 scenario_no() const noexcept;
        [[nodiscard]] std::span<const StagePlacementTable> tables() const noexcept;
        [[nodiscard]] std::span<const StageHolderOccurrence> holders() const noexcept;
        [[nodiscard]] std::span<const StagePlacementObject> placements() const noexcept;
        [[nodiscard]] std::span<const StageGeneralPos> general_positions() const noexcept;
        [[nodiscard]] const std::optional<StageStartInfo> &start_info() const noexcept;

        [[nodiscard]] NameObjPlacementContext placement_context(
            std::size_t placement_index) const;
        [[nodiscard]] NameObjPlacementContext start_context() const;

    private:
        std::string _stage_name;
        s32 _scenario_no = 1;
        std::vector<StagePlacementTable> _tables;
        std::vector<StageHolderOccurrence> _holders;
        std::vector<StagePlacementObject> _placements;
        std::vector<StageGeneralPos> _general_positions;
        std::optional<StageStartInfo> _start_info;
    };

}  // namespace smgpc::scene
