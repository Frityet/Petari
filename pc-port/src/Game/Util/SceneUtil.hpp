#pragma once

#include <revolution/types.h>

class JMapInfoIter;
class JMapInfo;

namespace MR {
    s32 getPlacedZoneId(const JMapInfoIter& rIter);
    void getRailInfo(JMapInfoIter* pPathIter, const JMapInfo** pPointInfo, const JMapInfoIter& rPlacementIter);
}  // namespace MR
