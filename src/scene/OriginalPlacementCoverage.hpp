#pragma once

#include "Game/Util/JMapInfo.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

class PlacementInfoOrdered;
class StageDataHolder;

namespace smgpc::scene {
    enum class OriginalPlacementAvailability {
        Supported,
        KnownUnlinked,
        Unknown,
        Metadata,
    };

    struct OriginalPlacementQueue {
        std::string_view phase;
        const PlacementInfoOrdered* ordered;
    };

    struct OriginalPlacementCoverageEntry {
        std::string phase;
        std::string table;
        std::string object;
        std::string reason;
        s32 zone = -1;
        s32 row = -1;
        s32 link_id = -1;
        s32 model_no = -1;
        OriginalPlacementAvailability availability = OriginalPlacementAvailability::Unknown;
    };

    // Reads the queues retained by original StageDataHolder; does not create,
    // sort, modify or reparse placements. The caller retains their source rows.
    [[nodiscard]] std::vector<OriginalPlacementCoverageEntry> inspect_original_placement_queues(
        std::span<const OriginalPlacementQueue> queues, const JMapInfoIter& player_start = {});
    void require_original_placement_closure(std::span<const OriginalPlacementCoverageEntry> entries);
    // Always reports a summary. Detailed JSON and strict validation are opt-in.
    void report_original_placement_coverage(const StageDataHolder& holder);
}  // namespace smgpc::scene
