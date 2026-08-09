// Retail calls the out-of-line JMA entry point; the current SDK header marks it inline.
#define JMAAcosRadian JMAAcosRadianInline

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHang.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"

#undef JMAAcosRadian

f32 JMAAcosRadian(f32);

bool Mario::checkPressDamage() {
    if (_5FC != 0) {
        return false;
    }

    if (isSwimming()) {
        return false;
    }

    if (checkVerticalPress(false)) {
        if (isStatusActive(5)) {
            mHang->forceDrop();
        } else {
            mActor->setPress(0, 0);

            if (mActor->_390 != 0) {
                return true;
            }

            _350.zero();
            _35C.zero();
            return true;
        }
    }

    if (checkSidePressPre()) {
        mActor->setPress(1, 0);
        return true;
    }

    if (checkSidePress()) {
        return true;
    }

    if (checkWallFloorCode(0x1D) && mMovementStates._1 && getFloorCode() != 0x1D) {
        mActor->_3B4 = _368;
        mActor->setPress(4, 120);
        return true;
    }

    if (_72C < 200.0f) {
        if (mMovementStates._8) {
            if (mMovementStates.jumping && getFrontWallNorm().dot(getAirGravityVec()) > 0.707f) {
                goto SidePress;
            }

            if (_72C < 100.0f) {
                _72C = 100.0f;
            }

            f32 force = _4E4 * (200.0f / _72C) - _4E4;
            addVelocity(getFrontWallNorm() * force);
        }

    SidePress:
        if (mMovementStates._1A) {
            if (mMovementStates.jumping && getSideWallNorm().dot(getAirGravityVec()) > 0.707f) {
                goto Finish;
            }

            if (_72C < 100.0f) {
                _72C = 100.0f;
            }

            f32 force = 60.0f * (200.0f / _72C) - 60.0f;
            addVelocity(getSideWallNorm() * force);
        }
    }

Finish:
    return false;
}

bool Mario::checkVerticalPress(bool recursive) {
    if (mMovementStates.debugMode) {
        return false;
    }

    TVec3f basePos;
    if (isStatusActive(5)) {
        basePos = mActor->_2A0;
    } else {
        basePos = mPosition;
    }

    if (!recursive && mActor->_390 == 0) {
        basePos += mVelocity;
    }

    f32 checkDistance;
    if (mMovementStates._A) {
        checkDistance = 80.0f;
    } else {
        checkDistance = 160.0f;
    }

    bool useHitPos = false;
    TVec3f up = -*getGravityVec();
    f32 hitDistance = 0.0f;
    Triangle firstTriangle;
    Triangle secondTriangle;

    TVec3f hitPos;
    bool hasFirst = MR::getFirstPolyOnLineToMap(&hitPos, &firstTriangle, basePos - up * 10.0f, up * checkDistance);

    if (hasFirst) {
        hitDistance = up.dot(hitPos - basePos);
        if (hitDistance < 150.0f) {
            useHitPos = true;
        }
    }

    if (!hasFirst) {
        goto Failure;
    }

    TVec3f secondBase;
    if (useHitPos) {
        secondBase = hitPos;
    } else {
        secondBase = basePos;
    }

    if (MR::getFirstPolyOnLineToMap(&hitPos, &secondTriangle, secondBase - *firstTriangle.getNormal(0) * 10.0f,
                                    *firstTriangle.getNormal(0) * checkDistance)) {
        if (firstTriangle.mSensor == secondTriangle.mSensor) {
            goto Failure;
        }

        if (!MR::isSensorPressObj(firstTriangle.mSensor) && !MR::isSensorPressObj(secondTriangle.mSensor)) {
            goto Failure;
        }

        if (!recursive && mActor->_390 == 0) {
            if (!checkVerticalPress(true)) {
                mVelocity.zero();
                return false;
            }

            return true;
        }

        mActor->_3B4 = _368;
        return true;
    }

    if (recursive || !MR::isSensorPressObj(firstTriangle.mSensor)) {
        goto Failure;
    }

    mDrawStates._1E = true;

    TVec3f pushVec;
    if (hitDistance < 0.0f) {
        pushVec = hitPos - basePos;
        pushVec.setLength(checkDistance - hitDistance);
    } else {
        pushVec = basePos - hitPos;
        pushVec.setLength(150.0f - hitDistance);
    }

    TVec3f pushDir;
    bool hasPushDir = MR::vecBlendSphere(*firstTriangle.getNormal(0), *secondTriangle.getNormal(0), &pushDir, 0.5f);

    if (mActor->_390 != 0) {
        goto Failure;
    }

    if (mMovementStates._1) {
        if (mGroundPolygon->mSensor == firstTriangle.mSensor) {
            if (!hasPushDir) {
                goto Failure;
            }

            push(pushDir * pushVec.length());
        } else {
            mActor->_F48 = firstTriangle.mSensor;
            mActor->_3B4 = _368;
            return true;
        }
    } else if (hasPushDir) {
        push(pushDir * pushVec.length());
    }

Failure:
    return false;
}

bool Mario::checkSidePressPre() {
    if (_5FC != 0) {
        return false;
    }

    if (mMovementStates._8 && mMovementStates._19) {
        if (!((_4F4 - _4E8).length() < 100.0f)) {
            goto Failure;
        }

        if (mFrontWallTriangle->mSensor == mBackWallTriangle->mSensor) {
            goto Failure;
        }

        if (!MR::isSensorPressObj(mFrontWallTriangle->mSensor) && !MR::isSensorPressObj(mBackWallTriangle->mSensor)) {
            goto Failure;
        }

        if (isDossun(mFrontWallTriangle) || isDossun(mBackWallTriangle)) {
            return false;
        }

        if (mFrontWallTriangle->getNormal(0)->dot(*mBackWallTriangle->getNormal(0)) < -0.5f) {
            mActor->_3B4 = getWallNorm();
            return true;
        }
    } else if (isStatusActive(5) && mMovementStates._19 && _8C8->isValid()) {
        if (_8C8->mSensor == mBackWallTriangle->mSensor) {
            goto Failure;
        }

        if (!MR::isSensorPressObj(_8C8->mSensor) && !MR::isSensorPressObj(mBackWallTriangle->mSensor)) {
            goto Failure;
        }

        if (isDossun(_8C8) || isDossun(mBackWallTriangle)) {
            return false;
        }

        TVec3f horizontal;
        MR::vecKillElement(mPosition - _4F4, getAirGravityVec(), &horizontal);
        if (horizontal.length() < 100.0f) {
            mActor->_3B4 = getWallNorm();
            return true;
        }
    }

Failure:
    return false;
}

bool Mario::checkSidePress() {
    TVec3f basePos;
    if (isStatusActive(5)) {
        basePos = mActor->_2A0;
    } else {
        basePos = mPosition;
    }

    f32 radius;
    f32 headOffset;
    if (mMovementStates._A) {
        radius = 50.0f;
        headOffset = 40.0f;
    } else {
        radius = 80.0f;
        headOffset = 70.0f;
    }

    f32 ceilDistance = calcDistToCeil(false);
    TVec3f probeCenter = basePos + mHeadVec * headOffset;
    u32 hitCount = Collision::checkStrikeBallToMap(probeCenter, radius, nullptr, nullptr);

    if (hitCount < 2) {
        return false;
    }

    for (u32 i = 0; i < hitCount; i++) {
        const HitInfo* firstInfo = Collision::getStrikeInfoMap(i);
        TVec3f firstNormal(*firstInfo->mParentTriangle.getNormal(0));

        for (u32 j = i + 1; j < hitCount; j++) {
            const HitInfo* secondInfo = Collision::getStrikeInfoMap(j);
            TVec3f secondNormal(*secondInfo->mParentTriangle.getNormal(0));

            if (firstNormal.dot(secondNormal) >= -0.707f) {
                continue;
            }

            TVec3f secondFromCenter = secondInfo->mHitPos - probeCenter;
            TVec3f firstFromCenter = firstInfo->mHitPos - probeCenter;
            if (firstFromCenter.dot(secondFromCenter) > 0.0f) {
                continue;
            }

            f32 hitDistance = (firstInfo->mHitPos - secondInfo->mHitPos).length();
            TVec3f firstVelocity;
            TVec3f secondVelocity;
            MR::calcVelocityMovingPoint(&firstInfo->mParentTriangle, firstInfo->mHitPos, &firstVelocity);
            MR::calcVelocityMovingPoint(&secondInfo->mParentTriangle, secondInfo->mHitPos, &secondVelocity);

            if (!MR::isNearZero(firstVelocity, 0.001f) || !MR::isNearZero(secondVelocity, 0.001f)) {
                if (!MR::isNearZero(firstVelocity, 0.001f)) {
                    MR::normalize(&firstVelocity);
                    if (firstVelocity.dot(firstNormal) < 0.2f || firstVelocity.dot(secondNormal) > -0.2f) {
                        continue;
                    }
                }

                if (!MR::isNearZero(secondVelocity, 0.001f)) {
                    MR::normalize(&secondVelocity);
                    if (secondVelocity.dot(secondNormal) < 0.2f || secondVelocity.dot(firstNormal) > -0.2f) {
                        continue;
                    }
                }

                if (firstInfo->mParentTriangle.mSensor != secondInfo->mParentTriangle.mSensor &&
                    (MR::isSensorPressObj(firstInfo->mParentTriangle.mSensor) || MR::isSensorPressObj(secondInfo->mParentTriangle.mSensor))) {
                    if (isStatusActive(5)) {
                        mMovementStates._1 = false;
                        closeStatus(mHang);
                    }

                    if (!mMovementStates._1 && __fabsf(firstNormal.dot(*getGravityVec())) < 0.707f) {
                        if (!firstInfo->isCollisionAtFace()) {
                            TVec3f horizontal;
                            f32 gravityDistance = MR::vecKillElement(firstInfo->mHitPos - probeCenter, *getGravityVec(), &horizontal);
                            if (gravityDistance != 0.0f) {
                                addVelocity(*getGravityVec() * -gravityDistance);
                                mDrawStates._1E = true;
                                continue;
                            }
                        }

                        if (!secondInfo->isCollisionAtFace()) {
                            TVec3f horizontal;
                            f32 gravityDistance = MR::vecKillElement(secondInfo->mHitPos - probeCenter, *getGravityVec(), &horizontal);
                            if (gravityDistance != 0.0f) {
                                addVelocity(*getGravityVec() * -gravityDistance);
                                mDrawStates._1E = true;
                                continue;
                            }
                        }
                    }

                    if (!isStatusActive(27) && _735 == 0 && ceilDistance >= 180.0f) {
                        TVec3f firstDirection = firstInfo->mHitPos - probeCenter;
                        TVec3f secondDirection = probeCenter - secondInfo->mHitPos;
                        if (!MR::isNearZero(MR::diffAngleAbs(firstDirection, secondDirection), 0.017453292f)) {
                            MR::normalizeOrZero(&firstDirection);
                            MR::normalizeOrZero(&secondDirection);
                            addVelocity(-firstDirection * 5.0f);
                            addVelocity(secondDirection * 5.0f);
                            continue;
                        }
                    }

                    f32 pressAngle = JMAAcosRadian(-firstNormal.dot(secondNormal));
                    if (mActor->selectTiltPress(firstInfo->mParentTriangle.mSensor) || mActor->selectTiltPress(secondInfo->mParentTriangle.mSensor)) {
                        pressAngle = 0.0f;
                    }

                    if (!isStatusActive(27) && _735 == 0 && pressAngle > 0.34906587f) {
                        f32 moveDistance = (160.0f - ceilDistance) / MR::sin(pressAngle);
                        if (moveDistance < 0.0f) {
                            continue;
                        }

                        TVec3f firstHorizontal;
                        TVec3f secondHorizontal;
                        MR::vecKillElement(firstNormal, *getGravityVec(), &firstHorizontal);
                        MR::vecKillElement(secondNormal, *getGravityVec(), &secondHorizontal);

                        if (firstHorizontal.length() > secondHorizontal.length()) {
                            firstHorizontal.setLength(moveDistance);
                            addVelocity(firstHorizontal);
                        } else {
                            secondHorizontal.setLength(moveDistance);
                            addVelocity(secondHorizontal);
                        }

                        continue;
                    }

                    s32 shadowCode = _95C->getCode(_45C);
                    s32 groundCode = _95C->getCode(mGroundPolygon);
                    if (shadowCode != groundCode && MR::isSensorPressObj(mGroundPolygon->mSensor) && mVerticalSpeed >= 200.0f) {
                        mDrawStates._A = true;
                        TVec3f horizontal;
                        MR::vecKillElement(mShadowPos - mGroundPos, getAirGravityVec(), &horizontal);
                        horizontal.setLength(10.0f);
                        addVelocity(horizontal);
                        return false;
                    }

                    if (__fabsf(firstNormal.dot(mHeadVec)) > 0.707f) {
                        mActor->setPress(0, 0);
                    } else {
                        mActor->setPress(1, 0);
                    }

                    mActor->_3B4 = firstNormal;
                    return true;
                }
            }

            if (getPlayer()->mMovementStates.jumping) {
                MR::diffAngleAbsHorizontal(firstNormal, secondNormal, getAirGravityVec());
                f32 normalDot = firstNormal.dot(secondNormal);

                TVec3f axis;
                PSVECCrossProduct(&firstNormal, &getAirGravityVec(), &axis);
                MR::normalizeOrZero(&axis);
                TVec3f firstTangent;
                PSVECCrossProduct(&axis, &firstNormal, &firstTangent);
                MR::normalizeOrZero(&firstTangent);

                PSVECCrossProduct(&secondNormal, &getAirGravityVec(), &axis);
                MR::normalizeOrZero(&axis);
                TVec3f secondTangent;
                PSVECCrossProduct(&axis, &secondNormal, &secondTangent);
                MR::normalizeOrZero(&secondTangent);

                TVec3f firstTop;
                firstTop = firstInfo->mHitPos + firstTangent * headOffset;
                TVec3f secondTop;
                secondTop = secondInfo->mHitPos + secondTangent * headOffset;
                f32 topDistance = (firstTop - secondTop).length();

                if (isRising()) {
                    if (normalDot < -0.707f && topDistance < 120.0f && hitDistance < 100.0f) {
                        cutGravityElementFromJumpVec(true);
                    }
                } else if (normalDot < -0.707f && hitDistance < 100.0f && topDistance < 80.0f) {
                    cutGravityElementFromJumpVec(true);
                    mMovementStates._1 = true;
                    stopJump();
                }
            }
        }
    }

    return false;
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
