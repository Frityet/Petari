#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SystemUtil.hpp"

#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "scene/PlacementZoneNameScope.hpp"

#include <revolution.h>

#include <exception>
#include <stdexcept>
#include <utility>

namespace {
    thread_local const std::string *sCurrentPlacementZoneName = nullptr;
}  // namespace

namespace smgpc::scene {

    PlacementZoneNameScope::PlacementZoneNameScope(s32 zone_id, std::string_view zone_name)
        : _checker(MR::getPlacementStateChecker()),
          _previous_zone_id(_checker != nullptr ? _checker->getCurrentPlacementZoneId() : -1),
          _zone_name(zone_name), _previous(sCurrentPlacementZoneName) {
        if (_checker == nullptr) {
            throw std::logic_error(
                "A retail placement lifecycle requires SceneObj_PlacementStateChecker.");
        }
        if (_zone_name.empty()) {
            throw std::invalid_argument("A placement-zone scope requires a copied zone name.");
        }
        _checker->setCurrentPlacementZoneId(zone_id);
        sCurrentPlacementZoneName = &_zone_name;
    }

    PlacementZoneNameScope::~PlacementZoneNameScope() {
        if (sCurrentPlacementZoneName != &_zone_name) {
            std::terminate();
        }
        sCurrentPlacementZoneName = _previous;
        if (_previous_zone_id >= 0) {
            _checker->setCurrentPlacementZoneId(_previous_zone_id);
        } else {
            _checker->clearCurrentPlacementZoneId();
        }
    }

    const char *try_current_placement_zone_name() noexcept {
        return sCurrentPlacementZoneName != nullptr ? sCurrentPlacementZoneName->c_str() : nullptr;
    }

}  // namespace smgpc::scene

namespace MR {

    s32 getPlacedZoneId(const JMapInfoIter &rIter) {
        return rIter.mInfo != nullptr ? rIter.mInfo->getPlacedZoneId() : -1;
    }

    void setCurrentPlacementZoneId(s32 zoneId) {
        auto *checker = getPlacementStateChecker();
        if (checker == nullptr) {
            throw std::logic_error("Setting the current placement zone requires SceneObj_PlacementStateChecker.");
        }
        checker->setCurrentPlacementZoneId(zoneId);
    }

    void clearCurrentPlacementZoneId() {
        auto *checker = getPlacementStateChecker();
        if (checker == nullptr) {
            throw std::logic_error("Clearing the current placement zone requires SceneObj_PlacementStateChecker.");
        }
        checker->clearCurrentPlacementZoneId();
    }

    s32 getCurrentPlacementZoneId() {
        auto *checker = getPlacementStateChecker();
        if (checker == nullptr) {
            throw std::logic_error("Reading the current placement zone requires SceneObj_PlacementStateChecker.");
        }
        return checker->getCurrentPlacementZoneId();
    }

    const char* getCurrentPlacementZoneName() {
        const auto *zone_name = smgpc::scene::try_current_placement_zone_name();
        if (zone_name == nullptr) {
            throw std::logic_error(
                "Reading the current placement-zone name requires an active placement lifecycle.");
        }
        return zone_name;
    }

    void getRailInfo(JMapInfoIter *pPathIter, const JMapInfo **pPointInfo, const JMapInfoIter &rPlacementIter) {
        if (pPathIter != nullptr) {
            *pPathIter = JMapInfoIter{};
        }
        if (pPointInfo != nullptr) {
            *pPointInfo = nullptr;
        }
        if (rPlacementIter.mInfo == nullptr) {
            return;
        }

        const JMapInfo *path_info = nullptr;
        const JMapInfo *point_info = nullptr;
        auto path_info_index = s32{-1};
        if (!rPlacementIter.mInfo->getRailInfo(rPlacementIter.mIndex, &path_info, &point_info, &path_info_index)) {
            return;
        }

        if (pPathIter != nullptr) {
            *pPathIter = JMapInfoIter(path_info, path_info_index);
        }
        if (pPointInfo != nullptr) {
            *pPointInfo = point_info;
        }
    }

    void getCameraRailInfo(JMapInfoIter *pPathIter, const JMapInfo **pPointInfo, s32, s32) {
        static_cast<void>(pPathIter);
        static_cast<void>(pPointInfo);
        throw std::logic_error("Camera rail lookup is unavailable without parsed stage camera rail data.");
    }

    bool isDisplayEncouragePal60Window() {
        return VIGetTvFormat() == VI_PAL;
    }

}  // namespace MR
