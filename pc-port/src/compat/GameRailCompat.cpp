#include "Game/Util/RailUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/Util/MathUtil.hpp"

namespace MR {
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
