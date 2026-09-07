#pragma once

#include "scene/StagePlacementResolver.hpp"

#include <JSystem/JGeometry/TMatrix.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace smgpc::compat {
    // Retains holder occurrences, including repeated and empty child zones.
    // Matrix addresses remain stable for the registry's complete lifetime.
    class StageZoneMatrixRegistry final {
    public:
        StageZoneMatrixRegistry(std::span<const scene::StageHolderOccurrence> holders,
                                std::span<const scene::StagePlacementTable> tables);

        [[nodiscard]] TPos3f* matrix_for_zone(s32 zone_id);
        [[nodiscard]] TPos3f* matrix_for_iter(const JMapInfoIter& iter);

    private:
        struct Holder {
            TPos3f matrix;
            s32 zone_id = 0;
            std::vector<std::size_t> children;
        };

        std::vector<Holder> _holders;
        std::optional<std::size_t> _root;
        std::unordered_map<const JMapInfo::DataCompat*, std::size_t> _table_holders;
    };

    class StageZoneMatrixBinding final {
    public:
        StageZoneMatrixBinding(std::span<const scene::StageHolderOccurrence> holders,
                               std::span<const scene::StagePlacementTable> tables);
        ~StageZoneMatrixBinding();

        StageZoneMatrixBinding(const StageZoneMatrixBinding&) = delete;
        StageZoneMatrixBinding& operator=(const StageZoneMatrixBinding&) = delete;

        [[nodiscard]] StageZoneMatrixRegistry& registry();

    private:
        StageZoneMatrixRegistry _registry;
        StageZoneMatrixBinding* _previous = nullptr;
    };

    [[nodiscard]] StageZoneMatrixRegistry& require_stage_zone_matrices();
}
