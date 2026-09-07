#include "Game/Util/RailUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

#include <algorithm>

namespace MR {
    f32 calcNearestRailCoord(const LiveActor* pActor, const TVec3f& rPos) {
        return pActor->mRailRider->calcNearestPos(rPos);
    }

    f32 calcNearestRailDirection(TVec3f* pOutDir, const LiveActor* pActor, const TVec3f& rPos) {
        f32 coord = calcNearestRailCoord(pActor, rPos);
        pActor->mRailRider->calcDirectionAtCoord(pOutDir, coord);
        return coord;
    }

    bool getRailArg3NoInit(const LiveActor* pActor, s32* pArg) {
        return pActor->mRailRider->getRailArgNoInit("path_arg3", pArg);
    }

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

// Original RailUtil query bodies; operate on the actor's actual RailRider state.
namespace {
    const char* getRailPointArgName(s32 argNum) {
        if (argNum == 0) {
            return "point_arg0";
        }
        if (argNum == 1) {
            return "point_arg1";
        }
        if (argNum == 2) {
            return "point_arg2";
        }
        if (argNum == 3) {
            return "point_arg3";
        }
        if (argNum == 4) {
            return "point_arg4";
        }
        if (argNum == 5) {
            return "point_arg5";
        }
        if (argNum == 6) {
            return "point_arg6";
        }
        if (argNum == 7) {
            return "point_arg7";
        }
        return nullptr;
    }

    bool getRailCurrentPointArgF32NoInit(const LiveActor* pActor, s32 argNum, f32* pArg) NO_INLINE {
        RailRider* railRider = pActor->mRailRider;
        s32 arg = *pArg;
        bool b = railRider->getCurrentPointArgS32NoInit(getRailPointArgName(argNum), &arg);
        *pArg = arg;
        return b;
    }

    bool getRailNextPointArgF32NoInit(const LiveActor* pActor, s32 argNum, f32* pArg) NO_INLINE {
        RailRider* railRider = pActor->mRailRider;
        s32 arg = *pArg;
        bool b = railRider->getNextPointArgS32NoInit(getRailPointArgName(argNum), &arg);
        *pArg = arg;
        return b;
    }
}  // namespace

namespace MR {
    void calcRailDirectionAtCoord(TVec3f* pDir, const LiveActor* pActor, f32 coord) {
        pActor->mRailRider->calcDirectionAtCoord(pDir, coord);
    }

    void calcRailPosAndDirectionAtCoord(TVec3f* pPos, TVec3f* pDir, const LiveActor* pActor, f32 coord) {
        calcRailPosAtCoord(pPos, pActor, coord);
        calcRailDirectionAtCoord(pDir, pActor, coord);
    }

    void calcDistanceToCurrentAndNextRailPoint(const LiveActor* pActor, f32* pCurrDist, f32* pNextDist) {
        // FIXME : regswap and improper re-load of pActor->mRailRider
        // https://decomp.me/scratch/Z1FEl

        RailRider* railRider = pActor->mRailRider;
        f32 currPointCoord = railRider->getCurrentPointCoord();
        f32 nextPointCoord = railRider->getNextPointCoord();

        if (isNearZero(currPointCoord)) {
            if (isRailGoingToEnd(pActor)) {
                f32 coord = railRider->mCoord;
                *pCurrDist = coord;

            } else {
                f32 coord = railRider->mCoord;
                *pCurrDist = railRider->getTotalLength() - coord;
            }
        } else {
            f32 coord = railRider->mCoord;
            *pCurrDist = MR::abs(coord - currPointCoord);
        }

        if (isNearZero(nextPointCoord)) {
            if (isRailGoingToEnd(pActor)) {
                f32 coord = railRider->mCoord;
                *pNextDist = railRider->getTotalLength() - coord;
            } else {
                f32 coord = railRider->mCoord;
                *pNextDist = coord;
            }
        } else {
            f32 coord = railRider->mCoord;
            *pNextDist = MR::abs(railRider->getNextPointCoord() - coord);
        }
    }

    const TVec3f& getRailDirection(const LiveActor* pActor) {
        return pActor->mRailRider->mCurDirection;
    }

    bool getCurrentRailPointArg1NoInit(const LiveActor* pActor, f32* pArg) {
        return ::getRailCurrentPointArgF32NoInit(pActor, 1, pArg);
    }

    bool getNextRailPointArg1NoInit(const LiveActor* pActor, f32* pArg) {
        return ::getRailNextPointArgF32NoInit(pActor, 1, pArg);
    }
}  // namespace MR

namespace MR {
    bool getCurrentRailPointArg0NoInit(const LiveActor* pActor, f32* pArg) {
        return ::getRailCurrentPointArgF32NoInit(pActor, 0, pArg);
    }
}

namespace MR {
    bool getNextRailPointArg0NoInit(const LiveActor* pActor, f32* pArg) {
        return ::getRailNextPointArgF32NoInit(pActor, 0, pArg);
    }
}
