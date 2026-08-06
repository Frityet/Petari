#include "Game/Util/SceneUtil.hpp"

#include "Game/Util/JMapInfo.hpp"

namespace MR {
    s32 getPlacedZoneId(const JMapInfoIter& rIter) {
        return rIter.mInfo != nullptr ? rIter.mInfo->getPlacedZoneId() : -1;
    }

    void getRailInfo(JMapInfoIter* pPathIter, const JMapInfo** pPointInfo, const JMapInfoIter& rPlacementIter) {
        if (pPathIter != nullptr) {
            *pPathIter = JMapInfoIter{};
        }
        if (pPointInfo != nullptr) {
            *pPointInfo = nullptr;
        }
        if (rPlacementIter.mInfo == nullptr) {
            return;
        }

        const JMapInfo* path_info = nullptr;
        const JMapInfo* point_info = nullptr;
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
}  // namespace MR
