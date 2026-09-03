#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioShadow.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Util/MathUtil.hpp"
#include <revolution/mtx.h>

void Mario::beeMarioOnGround() {
    if (getPlayerMode() == 4 && !mDrawStates._C && mMovementStates._1 && !mMovementStates._23 && !isStatusActive(MarioStatus_Slider)) {
        getPlayer()->incAirWalkTimer();
        getPlayer()->incAirWalkTimer();
        getPlayer()->incAirWalkTimer();
        getPlayer()->incAirWalkTimer();
    }
}

void MarioActor::entryWallWalkMode(const TVec3f& rPosition, const TVec3f& rNormal) {
    if (mBeeWallWalk == 0 && _9F2 == 0) {
        mBeeWallWalk = 5;

        TVec3f gravity(-rNormal);
        _240 = gravity;
        mPosition = rPosition;

        mMario->setTrans(rPosition, nullptr);
        mMario->stopJump();
        mMario->stopAnimation(nullptr);
        mMario->stopWalk();

        TVec3f oldHead(mMario->mHeadVec);
        mMario->setGravityVec(gravity);
        mMario->setHeadVec(-gravity);
        mMario->setFrontVecKeepUp(oldHead, static_cast< u32 >(1));
        setBlendMtxTimer(2);

        _38C = 5;
        mMario->mMovementStates._38 = false;
        _214->_305 = true;
    }
}

bool Mario::beeMarioOnAir() {
    if (_774 != 0) {
        _774--;

        if (_774 == 0) {
            mMovementStates._2F = false;
        }
    }

    if (getPlayerMode() == 4) {
        if (_3BC == 1 && _402 != 0) {
            _402--;
        }

        if (isAnimationRun("ハチ壁ジャンプ") && isAnimationTerminate(nullptr)) {
            if (checkLvlA()) {
                changeAnimation("ハチ飛行中", "落下");
            } else {
                changeAnimation("ハチ飛行中無入力", "落下");
            }

            changeAnimationInterpoleFrame(30);
        }
    }

    if (getPlayerMode() == 4) {
        TVec3f horizontalVelocity;
        if (MR::vecKillElement(mJumpVec, getAirGravityVec(), &horizontalVelocity) > -5.0f || _76C != 0) {
            if (_774 == 0) {
                const f32 gravitySpeed = cutGravityElementFromJumpVec(true);
                const f32 horizontalSpeed = mJumpVec.length();
                MR::normalizeOrZero(&mJumpVec);

                if (!MR::vecBlendSphere(mJumpVec, mFrontVec, &mJumpVec, mActor->mConst->getTable()->mBeeSpeedRotateRatio)) {
                    TMtx34f rotation;
                    PSMTXRotAxisRad(rotation, &mHeadVec, 0.1f);
                    PSMTXMultVecSR(rotation, &mJumpVec, &mJumpVec);
                }

                mJumpVec.setLength(horizontalSpeed);

                TVec3f gravityVelocity(getAirGravityVec());
                gravityVelocity.scale(gravitySpeed);
                mJumpVec += gravityVelocity;
            }

            if (!mMovementStates._11 && _402 != 0) {
                _406 = 16;

                if (_76C < 30) {
                    _76C = 30;
                    _770 = 0.0f;
                }

                mMovementStates._12 = true;
                _4B0 = mPosition;
                mMovementStates._11 = true;
            }

            s16 airWalkInhibitTime = static_cast< s16 >(mActor->mConst->getTable()->mBeeAirWalkInhibitTime);
            s16 gravityPowerTime = static_cast< s16 >(mActor->mConst->getTable()->mBeeGravityPowerTime);

            TVec3f horizontalJump;
            MR::vecKillElement(mJumpVec, getAirGravityVec(), &horizontalJump);
            if (horizontalJump.length() < 5.0f) {
                airWalkInhibitTime = static_cast< s16 >(mActor->mConst->getTable()->mBeeAirWalkInhibitTimeV);
                gravityPowerTime = static_cast< s16 >(mActor->mConst->getTable()->mBeeGravityPowerTimeV);
            }

            if (checkLvlA() && _402 != 0 && _3BC > airWalkInhibitTime) {
                playSound("ハチ飛行中", -1);

                if (!MR::isNearZero(mStickPos.z, 0.001f)) {
                    setFrontVecKeepUp(getWorldPadDir(), mActor->mConst->getTable()->mBeeAirWalkTurnSpd);
                }

                const u16 previousAirWalkTime = _402;
                if (_402 != 0) {
                    if (!mMovementStates._F && _402 > mActor->mConst->getTable()->mAirWalkTime) {
                        _402 = mActor->mConst->getTable()->mAirWalkTime;
                    }

                    _402--;
                }

                if (_402 == 0) {
                    if (previousAirWalkTime != 0) {
                        playSound("ハチ体力切れ", -1);
                    }

                    mMovementStates._11 = false;
                    stopAnimation("ハチ飛行中", static_cast< const char* >(nullptr));
                } else {
                    if (!mMovementStates._F) {
                        if (!isAnimationRun("ハチ壁ジャンプ") && !isAnimationRun("ハチセブン空中")) {
                            changeAnimation("ハチ飛行中", "落下");
                        }

                        cancelSquatMode();
                        playSound("空中ふんばり", -1);

                        if (static_cast< s32 >(_402) < static_cast< s32 >(static_cast< u32 >(mActor->mConst->getTable()->mAirWalkTime) >> 1)) {
                            getAnimator()->setSpeed(1.5f);
                        }

                        if (_430 == 4) {
                            setFrontVecKeepUp(-_220);
                            _430 = 0;
                        }

                        if (_430 == 5) {
                            _430 = 0;
                        }
                    }

                    s16 gravityTimer = static_cast< s16 >(_408);
                    if (gravityTimer > gravityPowerTime) {
                        gravityTimer = gravityPowerTime;
                    }

                    const f32 gravityRatio = static_cast< f32 >(gravityTimer) / static_cast< f32 >(gravityPowerTime);
                    const f32 randomReduction = 0.9f * (gravityRatio * gravityRatio);
                    const f32 random = MR::getRandom() - randomReduction;
                    const f32 verticalAcceleration =
                        15.0f * random * mActor->mConst->getTable()->mBeeFlyRandomFactor - mActor->mConst->getTable()->mBeeFlyConstantFactor;

                    f32 gravityElement = cutGravityElementFromJumpVec(true);
                    TVec3f jumpDirection(mJumpVec);
                    f32 accelerationRatio = 1.0f;
                    if (!MR::isNearZero(mStickPos.z, 0.001f)) {
                        jumpDirection.dot(getWorldPadDir());
                    }

                    if (accelerationRatio < 0.0f) {
                        accelerationRatio *= mActor->mConst->getTable()->mBeeUpAccelRatio;
                    }

                    _770 += verticalAcceleration * mActor->mConst->getTable()->mBeeAccelRatio * accelerationRatio;
                    if (_770 > 0.0f) {
                        _770 *= mActor->mConst->getTable()->mBeeUpDownKiller;
                    }

                    if (_774 == 0) {
                        if (!getPlayer()->_1C._5) {
                            if (_770 < -mActor->mConst->getTable()->mBeeUpSpeedMax) {
                                _770 = -mActor->mConst->getTable()->mBeeUpSpeedMax;
                            }
                        } else if (_770 < -0.5f) {
                            _770 = -0.5f;
                        }
                    }

                    addVelocity(*getGravityVec(), _770);

                    if (!getPlayer()->_1C._5 && gravityElement > 0.0f) {
                        gravityElement *= mActor->mConst->getTable()->mBeePushRiseGravityEraser;
                    }

                    TVec3f gravityVelocity(getAirGravityVec());
                    gravityVelocity.scale(gravityElement);
                    mJumpVec += gravityVelocity;

                    _408++;
                    if (_408 > 120) {
                        _408 = 120;
                    }

                    _4B0 = mPosition;
                    return true;
                }
            } else {
                if (!isAnimationRun("ハチジャンプ") && !isAnimationRun("ハチ壁ジャンプ")) {
                    stopAnimation("ハチ飛行中", static_cast< const char* >(nullptr));

                    if (!isAnimationRun(nullptr) || isAnimationTerminate(nullptr)) {
                        changeAnimation("ハチ飛行中無入力", static_cast< const char* >(nullptr));
                    }
                }

                if (_408 != 0) {
                    _408--;
                }

                if (_770 < mActor->mConst->getTable()->mBeeFreeDropMaxSpd) {
                    _770 += mActor->mConst->getTable()->mBeeFreeDropAcc;
                }

                f32 gravityRatio = 1.0f;
                addVelocity(*getGravityVec(), _770 * gravityRatio);

                if (!MR::isNearZero(mStickPos.z, 0.001f)) {
                    setFrontVecKeepUp(getWorldPadDir(), mActor->mConst->getTable()->mBeeAirWalkTurnSpd);
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
