#include "Game/AreaObj/MercatorTransformCube.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/RailUtil.hpp"

namespace MR {
    bool isInAreaObj(const char*, const TVec3f&) {
        // AreaObj placement is not hosted yet. An empty container contains no point.
        return false;
    }

    bool isInWater(const TVec3f& rPosition) {
        return MR::isInAreaObj("Water", rPosition);
    }

    void getDivideMercatorRailPosition(DivideMercatorRailPosInfo* pInfo, const LiveActor* pActor, u32 pointCount, f32, u32) {
        if (pInfo == nullptr || pActor == nullptr || pointCount == 0 || pActor->mRailRider == nullptr) {
            return;
        }

        const auto divisor = pointCount > 1 ? static_cast< f32 >(pointCount - (MR::isLoopRail(pActor) ? 0 : 1)) : 1.0F;
        const auto spacing = MR::getRailTotalLength(pActor) / divisor;
        for (u32 point = 0; point < pointCount; ++point) {
            auto position = TVec3f{};
            MR::calcRailPosAtCoord(&position, pActor, spacing * static_cast< f32 >(point));
            pInfo->setPosition(static_cast< s32 >(point), position);
        }
    }
}  // namespace MR
