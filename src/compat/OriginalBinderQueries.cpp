// Original Binder-backed LiveActorUtil and ActorMovementUtil queries.
// Every query observes the original actor Binder directly.
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"

namespace MR {
    f32 calcHitPowerToWall(const LiveActor* pActor) {
        if (!isBindedWall(pActor)) {
            return 0.0f;
        }

        f32 dot = pActor->mVelocity.dot(*getWallNormal(pActor));
        if (-dot > 0.0f) {
            return -dot;
        }

        return 0.0f;
    }

    bool isPressedRoofAndGround(const LiveActor* pActor) {
        if (!isBindedRoof(pActor) || !isBindedGround(pActor)) {
            return false;
        }

        if (!MR::isSensorPressObj(MR::getGroundSensor(pActor))) {
            if (!MR::isSensorPressObj(MR::getRoofSensor(pActor))) {
                goto LABEL_FALSE;
            }
        }

        {
            const Binder* binder = pActor->mBinder;
            const HitInfo* pRoofInfo = &binder->mRoofInfo;
            const HitInfo* pGroundInfo = &binder->mGroundInfo;
            TVec3f roofPower;
            TVec3f groundPower;
            pRoofInfo->mParentTriangle.calcForceMovePower(&roofPower, pRoofInfo->mHitPos);
            pGroundInfo->mParentTriangle.calcForceMovePower(&groundPower, pGroundInfo->mHitPos);

            TVec3f diff(roofPower);
            diff.sub(groundPower);
            if (0.0f < diff.dot(pActor->mGravity)) {
                return true;
            }
        }

    LABEL_FALSE:
        return false;
    }

    bool isOnGround(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;
        if (binder == nullptr) {
            return false;
        }

        if (!binder->isBindedGround()) {
            return false;
        }

        const TVec3f* normal = binder->mGroundInfo.mParentTriangle.getNormal(0);
        return !(0.0f < pActor->mVelocity.dot(*normal));
    }

    bool isBindedGround(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;
        if (binder == nullptr) {
            return false;
        }

        return binder->isBindedGround();
    }

    bool isBindedWall(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;
        if (binder == nullptr) {
            return false;
        }

        return binder->isBindedWall();
    }

    bool isBindedRoof(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        return pActor->mBinder->isBindedRoof();
    }

    const TVec3f* getGroundNormal(const LiveActor* pActor) {
        return pActor->mBinder->mGroundInfo.mParentTriangle.getNormal(0);
    }

    const TVec3f* getWallNormal(const LiveActor* pActor) {
        return pActor->mBinder->mWallInfo.mParentTriangle.getNormal(0);
    }

    const TVec3f* getRoofNormal(const LiveActor* pActor) {
        return pActor->mBinder->mRoofInfo.mParentTriangle.getNormal(0);
    }

    void offBind(LiveActor* pActor) {
        pActor->mFlag.mIsNoBind = true;
    }
}  // namespace MR
