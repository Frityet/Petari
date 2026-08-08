#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SystemUtil.hpp"

#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Util/JMapInfo.hpp"

#include <revolution.h>

#include <stdexcept>

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
