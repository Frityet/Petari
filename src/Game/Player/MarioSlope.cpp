#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"

void Mario::lockGroundCheck(void* pOwner, bool useSimple) {
    _10._5 = useSimple;
    mMovementStates._36 = useSimple;
    _10._4 = true;
    _574 = pOwner;
}

void Mario::unlockGroundCheck(void* pOwner) {
    if (pOwner != nullptr && _574 != pOwner) {
        return;
    }

    _574 = nullptr;
    _10._4 = false;
}

bool Mario::isUseSimpleGroundCheck() const {
    if (_10._4) {
        return _10._5;
    }

    if (mMovementStates._23) {
        return true;
    }

    if (isPlayerModeTeresa()) {
        return true;
    }

    if (!mMovementStates._1) {
        return mMovementStates._23;
    }

    if (calcAngleD(_368) > mActor->getConst().getTable()->mFlatAngle - 5.0f) {
        Triangle triangle;

        TVec3f startOffset(*getGravityVec());
        startOffset *= 30.0f;
        TVec3f ray(*getGravityVec());
        ray *= 100.0f;
        TVec3f start = mPosition - startOffset;
        if (MR::getFirstPolyOnLineBFast(start, ray, nullptr, &triangle)) {
            if (calcAngleD(*triangle.getNormal(0)) < mActor->getConst().getTable()->mFlatAngle - 5.0f) {
                return false;
            }
        }

        return true;
    }

    getAnimator()->isLandingAnimationRun();
    return false;
}

bool Mario::checkGroundOnSlope() {
    TVec3f groundNormal;
    if (isAnimationRun("崖ふんばり")) {
        groundNormal = -*getGravityVec();
    } else {
        groundNormal = *_45C->getNormal(0);
    }

    if (MR::isNearZero(groundNormal, 0.001f)) {
        return false;
    }

    TVec3f groundBase;
    MR::vecKillElement(mFrontVec, groundNormal, &groundBase);
    if (MR::isNearZero(groundBase, 0.001f)) {
        return false;
    }

    Triangle triangle;
    bool noGround = false;
    mMovementStates._14 = true;

    f32 verticalLimit;
    if (mMovementStates.jumping && isRising()) {
        verticalLimit = 10.0f;
    } else if (_414 != 0) {
        verticalLimit = 20.0f;
    } else {
        verticalLimit = 60.0f;
    }

    if (mMovementStates._1) {
        f32 angle = calcAngleD(_368) - 30.0f;
        if (angle < 0.0f) {
            angle = 0.0f;
        }
        if (angle > 40.0f) {
            angle = 40.0f;
        }
        verticalLimit += 2.0f * angle;
    }

    if (getCurrentStatus() == 7) {
        verticalLimit = 100.0f;
    }

    TVec3f hitAverage(0.0f, 0.0f, 0.0f);
    bool hasGround = false;
    TVec3f start = mPosition - *getGravityVec() * 30.0f;
    TVec3f ray = *getGravityVec() * verticalLimit;
    TVec3f hitPos;
    bool isHit = MR::getFirstPolyOnLineBFast(start, ray, &hitPos, &triangle);

    if (isHit) {
        if (calcAngleD(*triangle.getNormal(0)) >= 80.0f) {
            isHit = false;
        }

        f32 gravityDot = getGravityVec()->dot(*triangle.getNormal(0));
        if (getCurrentStatus() != 7 && gravityDot > -0.1908f) {
            isHit = false;
        }

        if (isHit) {
            hitAverage += hitPos;
            hasGround = true;
            setGroundNorm(*triangle.getNormal(0));

            Triangle* pGroundPolygon = mGroundPolygon;
            pGroundPolygon->mParts = triangle.mParts;
            pGroundPolygon->mIdx = triangle.mIdx;
            pGroundPolygon->mSensor = triangle.mSensor;
            pGroundPolygon->mNormals[0] = triangle.mNormals[0];
            pGroundPolygon->mNormals[1] = triangle.mNormals[1];
            pGroundPolygon->mNormals[2] = triangle.mNormals[2];
            pGroundPolygon->mNormals[3] = triangle.mNormals[3];
            pGroundPolygon->mPos[0] = triangle.mPos[0];
            pGroundPolygon->mPos[1] = triangle.mPos[1];
            pGroundPolygon->mPos[2] = triangle.mPos[2];

            TVec3f groundPos(hitPos);
            mGroundPos = groundPos;
            recordLastGround();
        }
    }

    if (!hasGround) {
        noGround = true;
    }

    if (mMovementStates._D) {
        mMovementStates._D = false;

        TVec3f offset = mGroundPos - mPosition;
        f32 gravityOffset = MR::vecKillElement(offset, *getGravityVec(), &offset);
        TVec3f correction = *getGravityVec() * gravityOffset;
        addTrans(correction, "force Trans");
        return true;
    }

    if (mMovementStates.jumping && isRising()) {
        return false;
    }

    if (noGround) {
        mMovementStates._14 = true;
        return false;
    }

    mMovementStates._14 = false;
    TVec3f groundUp = -_368;
    TVec3f groundPos(mGroundPos);

    if (_10._22) {
        _10._22 = false;
        _1C._C = true;
    } else if ((mMovementStates._23 || _71C != 0) && getPlayerMode() != 6) {
        TVec3f toGround = groundPos - mPosition;
        if (__fabsf(toGround.dot(groundUp)) < verticalLimit) {
            TVec3f horizontal = groundPos - mPosition;
            f32 gravityOffset = MR::vecKillElement(horizontal, groundUp, &horizontal);

            if (!MR::isNearZero(gravityOffset, 1.0f)) {
                mDrawStates._0 = false;

                TVec3f correction = groundUp * gravityOffset;
                f32 speed;
                if (_278 < 0.0f) {
                    speed = 0.0f;
                } else if (_278 > 1.0f) {
                    speed = 1.0f;
                } else {
                    speed = _278;
                }
                if (!mMovementStates._23) {
                    correction *= speed;
                }

                const bool isNotSlipFloor = !isSlipFloorCode(_960);
                if (!isNotSlipFloor && !mDrawStates._4 && !mPrevDrawStates._4) {
                    mVelocityAfter += correction;
                }
            }

            return true;
        }
    }

    if (mVerticalSpeed < 5.0f) {
        return true;
    }

    if (mDrawStates._6) {
        return true;
    }

    if (hasGround) {
        return true;
    }

    return mDrawStates._0;
}

namespace NrvMarioActor {
    INIT_NERVE(MarioActorNrvWait);
    INIT_NERVE(MarioActorNrvGameOver);
    INIT_NERVE(MarioActorNrvGameOverAbyss);
    INIT_NERVE(MarioActorNrvGameOverAbyss2);
    INIT_NERVE(MarioActorNrvGameOverFire);
    INIT_NERVE(MarioActorNrvGameOverBlackHole);
    INIT_NERVE(MarioActorNrvGameOverNonStop);
    INIT_NERVE(MarioActorNrvGameOverSink);
    INIT_NERVE(MarioActorNrvTimeWait);
    INIT_NERVE(MarioActorNrvNoRush);
};  // namespace NrvMarioActor
