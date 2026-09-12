#include <aurora/exception.hpp>
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SystemUtil.hpp"

#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "scene/PlacementZoneScope.hpp"

#include <revolution.h>

#include <exception>
#include <stdexcept>
#include <utility>

namespace {
    thread_local smgpc::scene::PlacementZoneScope *sCurrentPlacementScope = nullptr;
}  // namespace

namespace smgpc::scene {

    PlacementZoneScope::PlacementZoneScope(s32 zone_id)
        : _checker(MR::getPlacementStateChecker()),
          _previous_zone_id(_checker != nullptr ? _checker->getCurrentPlacementZoneId() : -1),
          _previous(sCurrentPlacementScope) {
        if (_checker == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "A retail placement lifecycle requires SceneObj_PlacementStateChecker.");
        }
        _checker->setCurrentPlacementZoneId(zone_id);
        sCurrentPlacementScope = this;
    }

    PlacementZoneScope::~PlacementZoneScope() {
        if (sCurrentPlacementScope != this) {
            std::terminate();
        }
        sCurrentPlacementScope = _previous;
        if (_previous_zone_id >= 0) {
            _checker->setCurrentPlacementZoneId(_previous_zone_id);
        } else {
            _checker->clearCurrentPlacementZoneId();
        }
    }

}  // namespace smgpc::scene

namespace MR {

    bool isDisplayEncouragePal60Window() {
        return VIGetTvFormat() == VI_PAL;
    }

}  // namespace MR
