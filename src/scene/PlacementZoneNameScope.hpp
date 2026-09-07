#pragma once

#include <string>
#include <string_view>

#include <revolution/types.h>

class PlacementStateChecker;

namespace smgpc::scene {

    // Retail derives the placement-zone name through StageDataHolder and the
    // PlacementStateChecker's current ID. The PC stage loader already owns the
    // exact copied zone name, so expose it only for the matching placement
    // construction lifetime. Nested construction restores its caller.
    class PlacementZoneNameScope final {
    public:
        PlacementZoneNameScope(s32 zone_id, std::string_view zone_name);
        ~PlacementZoneNameScope();

        PlacementZoneNameScope(const PlacementZoneNameScope &) = delete;
        PlacementZoneNameScope &operator=(const PlacementZoneNameScope &) = delete;
        PlacementZoneNameScope(PlacementZoneNameScope &&) = delete;
        PlacementZoneNameScope &operator=(PlacementZoneNameScope &&) = delete;

    private:
        PlacementStateChecker *_checker = nullptr;
        s32 _previous_zone_id = -1;
        std::string _zone_name;
        const std::string *_previous = nullptr;
    };

    [[nodiscard]] const char *try_current_placement_zone_name() noexcept;

}  // namespace smgpc::scene
