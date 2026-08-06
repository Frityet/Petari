#include "Game/Util/SceneUtil.hpp"

#include "Game/Util/JMapInfo.hpp"

namespace MR {
    s32 getPlacedZoneId(const JMapInfoIter& rIter) {
        return rIter.mInfo != nullptr ? rIter.mInfo->getPlacedZoneId() : -1;
    }
}  // namespace MR
