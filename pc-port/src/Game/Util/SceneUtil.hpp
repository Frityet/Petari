#pragma once

#include <revolution/types.h>

class JMapInfoIter;
class JMapInfo;

namespace MR {
    void incCoin(int amount);
    void incPurpleCoin();
    s32 getPlacedZoneId(const JMapInfoIter& rIter);
    void getRailInfo(JMapInfoIter* pPathIter, const JMapInfo** pPointInfo, const JMapInfoIter& rPlacementIter);
    void getCameraRailInfo(JMapInfoIter* pPathIter, const JMapInfo** pPointInfo, s32 zoneId, s32 railId);
}  // namespace MR
