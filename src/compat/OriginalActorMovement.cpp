// Original ActorMovementUtil functions; all simulation state belongs to Game.
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/HitSensor.hpp"

namespace MR {
    bool isNearPlayerHorizontal(const LiveActor* pActor, f32 dist) {
        TVec3f stack_14;
        if (isPlayerHidden()) {
            return false;
        }
        TVec3f stack_8 = pActor->mPosition - *MR::getPlayerPos();
        f32 f1 = vecKillElement(stack_8, pActor->mGravity, &stack_14);
        return (stack_14.squared() < dist * dist);
    }

    void calcVecFromPlayerH(TVec3f* pFromPlayerHVec, const LiveActor* pActor) {
        calcVecToTargetPosH(pFromPlayerHVec, pActor, *MR::getPlayerPos(), nullptr);
        pFromPlayerHVec->scale(-1.0f);
    }

    void makeQuatAndFrontFromRotate(TQuat4f* pQuat, TVec3f* pVec, const LiveActor* pActor) {
        makeQuatRotateDegree(pQuat, pActor->mRotation);
        pQuat->getZDir(*pVec);
        return;
    }

    void addVelocityClockwiseToDirection(LiveActor* pActor, const TVec3f& a2, f32 a3) {
        TVec3f stack_14;
        if (normalizeOrZero(a2, &stack_14)) {
            return;
        }
        stack_14.cross(pActor->mGravity, stack_14);
        TVec3f stack_8;
        calcVelocityMoveToDirection(&stack_8, pActor, stack_14, a3);
        (pActor->mVelocity).add(stack_8);
    }

    void setVelocitySeparateHV(LiveActor* pActor, const TVec3f& a2, f32 a3, f32 a4) {
        TVec3f stack_2c;
        stack_2c.killElement(a2, pActor->mGravity);
        normalizeOrZero(&stack_2c);
        TVec3f stack_20 = stack_2c * a3 - pActor->mGravity * a4;
        pActor->mVelocity.set(stack_20);
    }

    bool reboundVelocityFromEachCollision(LiveActor* pActor, f32 a2, f32 a3, f32 a4, f32 a5) {
        if (!isBinded(pActor)) {
            return false;
        }
        TVec3f bindedReactnVec = *getBindedFixReactionVector(pActor);
        if (isNearZero(bindedReactnVec)) {
            return false;
        }
        normalize(&bindedReactnVec);
        f32 f31 = bindedReactnVec.dot(pActor->mGravity);
        f32 f30 = 0.0f;
        if (isFloorPolygon(f31)) {
            f30 = a2;
        } else if (isWallPolygon(f31)) {
            f30 = a3;
        } else if (isCeilingPolygon(f31)) {
            f30 = a4;
        }
        f31 = bindedReactnVec.dot(pActor->mVelocity);
        f32 f0 = -a5;
        if (f31 < f0) {
            pActor->mVelocity.sub(bindedReactnVec * f31 * (1.0f + f30));
            return true;
        }
        if (f31 < 0.0f) {
            pActor->mVelocity.sub(bindedReactnVec * f31);
            return false;
        }
        return false;
    }

    bool sendMsgPushAndKillVelocityToTarget(LiveActor* pActor, HitSensor* pSensor1, HitSensor* pSensor2) {
        if (sendMsgPush(pSensor1, pSensor2)) {
            TVec3f stack_8 = pSensor2->mPosition - pSensor1->mPosition;
            normalizeOrZero(&stack_8);
            if (pActor->mVelocity.dot(stack_8) < 0.0f) {
                TVec3f* pVelocity = &pActor->mVelocity;
                pActor->mVelocity.killElement(*pVelocity, stack_8);
            }
            return true;
        }
        return false;
    }

    void addVelocityFromPush(LiveActor* pActor, f32 a2, HitSensor* pSensor1, HitSensor* pSensor2) {
        TVec3f stack_14 = pSensor2->mPosition - pSensor1->mPosition;
        if (a2 < (pActor->mVelocity).dot(-pActor->mGravity)) {
            vecKillElement(stack_14, pActor->mGravity, &stack_14);
        }
        stack_14.setLength(a2);
        addVelocityLimit(pActor, stack_14);
    }

    void turnDirectionFromTargetDegree(const LiveActor* pActor, TVec3f* a2, const TVec3f& a3, f32 a4) {
        TVec3f stack_8 = pActor->mPosition - a3;
        turnVecToVecCosOnPlane(a2, stack_8, pActor->mGravity, cosDegree(a4));
    }

    void setVelocitySeparateHV(LiveActor* pActor, HitSensor* pSensor1, HitSensor* pSensor2, f32 a4, f32 a5) {
        TVec3f stack_8 = pSensor2->mPosition - pSensor1->mPosition;
        setVelocitySeparateHV(pActor, stack_8, a4, a5);
    }
}

namespace MR {
    void calcVecToTargetPosH(TVec3f* pToTargetHVec, const LiveActor* pActor, const TVec3f& a3, const TVec3f* a4) {
        pToTargetHVec->set< f32 >(a3);
        pToTargetHVec->sub(pActor->mPosition);

        if (a4 == nullptr) {
            MR::vecKillElement(*pToTargetHVec, pActor->mGravity, pToTargetHVec);
        } else {
            MR::vecKillElement(*pToTargetHVec, *a4, pToTargetHVec);
        }

        MR::normalizeOrZero(pToTargetHVec);
    }

    void calcVelocityMoveToDirectionHorizon(TVec3f* a1, const LiveActor* pActor, const TVec3f& a3, f32 a4) {
        a1->killElement(a3, pActor->mGravity);
        normalizeOrZero(a1);
        a1->scale(a4);
    }

    void calcVelocityMoveToDirection(TVec3f* a1, const LiveActor* pActor, const TVec3f& a3, f32 a4) {
        calcVelocityMoveToDirectionHorizon(a1, pActor, a3, a4);
        if (isOnGround(pActor)) {
            a1->orthogonalize(*getGroundNormal(pActor));
        }
    }

    bool addVelocityLimit(LiveActor* pActor, const TVec3f& a2) {
        TVec3f stack_18;
        f32 stack_8;
        separateScalarAndDirection(&stack_8, &stack_18, a2);
        if (isNearZero(stack_18)) {
            return false;
        }
        f32 f1 = stack_18.dot(pActor->mVelocity);
        if (stack_8 <= f1) {
            return false;
        }
        pActor->mVelocity.add(stack_18 * (stack_8 - f1));
        return true;
    }
}
