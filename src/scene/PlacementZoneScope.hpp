#pragma once

#include <revolution/types.h>

class PlacementStateChecker;

namespace smgpc::scene {

    // Scoped placement operations borrow the original checker ID. Zone names
    // are resolved by SceneUtil through the actual galaxy status accessor.
    class PlacementZoneScope final {
    public:
        PlacementZoneScope(s32 zone_id);
        ~PlacementZoneScope();

        PlacementZoneScope(const PlacementZoneScope &) = delete;
        PlacementZoneScope &operator=(const PlacementZoneScope &) = delete;
        PlacementZoneScope(PlacementZoneScope &&) = delete;
        PlacementZoneScope &operator=(PlacementZoneScope &&) = delete;

    private:
        PlacementStateChecker *_checker = nullptr;
        s32 _previous_zone_id = -1;
        PlacementZoneScope *_previous = nullptr;
    };

}  // namespace smgpc::scene
