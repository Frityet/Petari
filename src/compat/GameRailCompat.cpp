#include "Game/Util/RailUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

#include <algorithm>

namespace MR {
    void initAndSetRailClipping(TVec3f* pCenter, LiveActor* pActor, f32, f32 padding) {
        if (pCenter == nullptr || pActor == nullptr || pActor->mRailRider == nullptr) {
            return;
        }
        const auto point_count = pActor->mRailRider->getPointNum();
        if (point_count <= 0) {
            pCenter->set(pActor->mPosition);
            return;
        }

        auto minimum = TVec3f{};
        auto maximum = TVec3f{};
        pActor->mRailRider->copyPointPos(&minimum, 0);
        maximum.set(minimum);
        for (auto index = s32{1}; index < point_count; ++index) {
            auto point = TVec3f{};
            pActor->mRailRider->copyPointPos(&point, index);
            minimum.x = std::min(minimum.x, point.x);
            minimum.y = std::min(minimum.y, point.y);
            minimum.z = std::min(minimum.z, point.z);
            maximum.x = std::max(maximum.x, point.x);
            maximum.y = std::max(maximum.y, point.y);
            maximum.z = std::max(maximum.z, point.z);
        }
        pCenter->set((minimum + maximum) * 0.5F);
        const auto radius = (maximum - minimum).length() * 0.5F + padding;
        MR::setClippingTypeSphere(pActor, radius, pCenter);
    }

    void moveCoordToNearestPos(LiveActor* pActor, const TVec3f& rPos) {
        pActor->mRailRider->moveToNearestPos(rPos);
    }

    void moveCoordToStartPos(LiveActor* pActor) {
        setRailCoord(pActor, 0.0f);
    }

    void calcRailEndPos(TVec3f* pPos, const LiveActor* pActor) {
        calcRailPosAtCoord(pPos, pActor, getRailTotalLength(pActor));
    }

    void calcRailPosAtCoord(TVec3f* pPos, const LiveActor* pActor, f32 coord) {
        pActor->mRailRider->calcPosAtCoord(pPos, coord);
    }

    void calcRailPointPos(TVec3f* pPos, const LiveActor* pActor, int index) {
        if (pPos != nullptr && pActor != nullptr && pActor->mRailRider != nullptr) {
            pActor->mRailRider->copyPointPos(pPos, index);
        }
    }

    void calcRailPosFrontCoord(TVec3f* pPos, const LiveActor* pActor, f32 frontDist) {
        f32 coord = getRailCoord(pActor);
        coord = isRailGoingToEnd(pActor) ? coord + frontDist : coord - frontDist;

        if (isLoopRail(pActor)) {
            coord = MR::repeat(coord, 0.0f, getRailTotalLength(pActor));
        } else {
            coord = MR::clamp(coord, 0.0f, getRailTotalLength(pActor));
        }

        calcRailPosAtCoord(pPos, pActor, coord);
    }

    f32 getRailTotalLength(const LiveActor* pActor) {
        return pActor->mRailRider->getTotalLength();
    }

    s32 getRailPointNum(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mRailRider != nullptr ? pActor->mRailRider->getPointNum() : 0;
    }

    const TVec3f& getRailPos(const LiveActor* pActor) {
        return pActor->mRailRider->mCurPos;
    }

    f32 getRailCoord(const LiveActor* pActor) {
        return pActor->mRailRider->mCoord;
    }

    void setRailCoord(LiveActor* pActor, f32 coord) {
        pActor->mRailRider->setCoord(coord);
    }

    void setRailCoordSpeed(LiveActor* pActor, f32 speed) {
        pActor->mRailRider->setSpeed(MR::abs(speed));
    }

    void moveRailRider(LiveActor* pActor) {
        pActor->mRailRider->move();
    }

    bool isLoopRail(const LiveActor* pActor) {
        return pActor->mRailRider->isLoop();
    }

    bool isRailReachedNearGoal(const LiveActor* pActor, f32 range) {
        if (isRailGoingToEnd(pActor)) {
            f32 length = getRailTotalLength(pActor);
            if (length - range <= getRailCoord(pActor)) {
                return true;
            }
        } else if (getRailCoord(pActor) < range) {
            return true;
        }
        return false;
    }

    bool isRailGoingToEnd(const LiveActor* pActor) {
        return pActor->mRailRider->mIsNotReverse;
    }
}  // namespace MR
