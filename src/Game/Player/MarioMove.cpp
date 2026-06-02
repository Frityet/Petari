#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioSkate.hpp"
#include "Game/Util/MathUtil.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

#define MARIO_MOVE_TURNING_MASK 0x10000000
#define MARIO_MOVE_LOCK_TURN_MASK 0x00080000

void Mario::mainMove() {
    TVec3f moveDir;
    MR::setNan(moveDir);

    bool inhibitTurn = false;

    if (mDrawStates._5) {
        inhibitTurn = true;
    }

    if (mMovementStates._23) {
        _71C = 0;
        _278 = 0.0f;
        inhibitTurn = true;
        playSound("坂滑り", -1);
    }

    if (_754 != 0) {
        inhibitTurn = true;
    }

    mMovementStates._35 = false;

    if (isSkatableFloor()) {
        mMovementStates._35 = true;
    }

    if (_10._1A) {
        tryJump();
        beforeJumping2D();

        if (isAnimationRun("その場足踏み上半身")) {
            stopAnimationUpper(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr));
        }

        _420 = 0;
        mJumpVec = _184;
        _10._1A = false;
        return;
    }

    bool doJump = false;
    bool requestJump = mActor->isRequestJump();

    if (!requestJump && mActor->isRequestJump2P() && _3C8 == 0) {
        _3C8 = 6;
    }

    if (requestJump) {
    } else if (_3C8 != 0) {
        _3C8--;

        if (_3C8 == 0) {
            requestJump = true;
            _1C_WORD |= 0x200000;
        }
    }

    if (requestJump && _3C8 != 0) {
        _3C8 = 0;
        _1C_WORD |= 0x100000;
    }

    if (requestJump || mMovementStates._38) {
        if (mMovementStates._A && calcDistToCeil(false) < 160.0f) {
            mActor->sendMsgUpperPunch(reinterpret_cast<HitSensor*>(_730));
            changeAnimation("しゃがみアッパー", static_cast<const char*>(nullptr));
        } else if (isStatusActive(0x1F)) {
            mSkate->exitJump();
            closeStatus(mSkate);
        } else {
            doJump = true;
        }
    }

    if (_436 != 0) {
        _436--;

        if (doJump && checkSquat(false) && mStickPos.z >= 0.5f) {
            _436 = 0;
            _278 = 5.0f;
            _434 = 180;
            doJump = false;
            playSound("スペシャルダッシュ強", -1);
            playSound("ダッシュ加速強成功", -1);
            playSound("声物ジャンプ", -1);
        } else if (doJump && mStickPos.z >= 0.5f) {
            _436 = 0;
            _278 = 2.0f;
            _434 = 60;
            doJump = false;
            playSound("スペシャルダッシュ弱", -1);
            playSound("声物ジャンプ", -1);
        }
    }

    if (doJump) {
        saveLastSafetyTrans();
        tryJump();
        beforeJumping2D();

        if (isAnimationRun("その場足踏み上半身")) {
            stopAnimationUpper(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr));
        }

        _420 = 0;
        return;
    }

    if (!mMovementStates._1) {
        if (_3C0 != 0 && !mMovementStates._23 && MR::isNearZero(_8F8, 0.001f)) {
            _3C0--;
        } else if (!mActor->_EA4) {
            tryDrop();
            beforeJumping2D();
            _420 = 0;
            return;
        }
    } else {
        const MarioConstTable* dropTable = mActor->mConst->getTable();

        if (mStickPos.z < dropTable->mWallBackHangStickPower) {
            _3C0 = 0;
        } else if (mDrawStates._A) {
            _3C0 = 0;
        } else {
            if (_3C0 < static_cast<u16>(dropTable->mDropWaitTime)) {
                _3C0 = dropTable->mDropWaitTime;
            }
        }
    }

    if (_420 != 0) {
        _420--;
        _10_HIGH_WORD |= 0x800000;
        return;
    }

    checkTornado();
    getAnimator()->setWalkMode();

    if (MR::isNearZero(mStickPos.x, 0.001f) && MR::isNearZero(mStickPos.y, 0.001f)) {
        _328 = _334;

        if (MR::isNearZero(_328, 0.001f)) {
            _328 = mFrontVec;
        }

        moveDir = _328;
        calcShadowDir(_328, &_22C);
        _40C = 10;
    } else {
        if ((!_71C || !_71D) && !isAnimationRun("ブレーキ") && !isAnimationRun("ターンブレーキ")
            && !mMovementStates._10 && !mMovementStates._F && _278 < 0.999f && !mDrawStates._5) {
            bool allowMove = true;

            if (_40C != 0) {
                if (mStickPos.z < 0.5f) {
                    _40C--;
                    allowMove = false;
                    mStickPos.zero();
                } else {
                    _40C = 0;
                }
            }

            if (allowMove) {
                calcMoveDir(mStickPos.x, mStickPos.y, &moveDir, true);
                calcShadowDir(moveDir, &_22C);
                calcShadowDir(mFrontVec, &_214);

                f32 turnAngle;
                f32 turnLimit = 0.7853982f;
                if (mMovementStates._37) {
                    turnAngle = MR::diffAngleAbsHorizontal(_214, _22C, _6A0);
                    turnLimit = 1.5707964f;
                } else {
                    turnAngle = MR::diffAngleAbsHorizontal(_214, _22C, *getGravityVec());
                }

                if (turnAngle > turnLimit) {
                    mDrawStates._E = true;

                    u32 standingTurnTime = mActor->mConst->getTable()->mStandingTurnTime;
                    if (mActor->mAlphaEnable) {
                        standingTurnTime = 30;
                    }

                    if (!inhibitTurn) {
                        setFrontVecKeepUp(_22C, standingTurnTime);
                    }

                    if (isEnableTurn()) {
                        changeAnimation("その場足踏み", static_cast<const char*>(nullptr));

                        if (!isAnimationRun("カリカリ限界")) {
                            changeAnimationUpperWeak("その場足踏み上半身", static_cast<const char*>(nullptr));
                        }

                        return;
                    }
                } else if (!inhibitTurn) {
                    setFrontVecKeepUp(_22C);
                }
            } else {
                moveDir = _328;
            }
        } else {
            f32 stickX = mStickPos.x;
            f32 stickY = mStickPos.y;

            if (!getPlayer()->_10._11) {
                const MarioConstTable* table = mActor->mConst->getTable();
                const f32 absStickY = __fabsf(stickY);

                if (absStickY > table->mStickMarginYstart) {
                    const f32 absStickX = __fabsf(stickX);

                    if (absStickX < table->mStickMarginX) {
                        stickX = 0.0f;
                    } else if (stickX > 0.0f) {
                        stickX = (stickX - table->mStickMarginX) / (1.0f - table->mStickMarginX);
                    } else {
                        stickX = (stickX + table->mStickMarginX) / (1.0f - table->mStickMarginX);
                    }
                } else {
                    const f32 absStickX = __fabsf(stickX);

                    if (absStickX > table->mStickMarginXstart) {
                        if (absStickY < table->mStickMarginY) {
                            stickY = 0.0f;
                        } else if (stickY > 0.0f) {
                            stickY = (stickY - table->mStickMarginY) / (1.0f - table->mStickMarginY);
                        } else {
                            stickY = (stickY + table->mStickMarginY) / (1.0f - table->mStickMarginY);
                        }
                    }
                }
            }

            calcMoveDir(stickX, stickY, &moveDir, true);
            calcShadowDir(moveDir, &_22C);
        }
    }

    calcShadowDir(mFrontVec, &_214);

    const f32 frontDot = _214.dot(_22C);
    f32 turnSlipDot = frontDot;

    if (_3D2 != 0) {
        turnSlipDot = _3E4.dot(_22C);
    }

    if (frontDot >= MR::cos(mActor->mConst->getTable()->mTurnSlipAngle) && !mMovementStates._4
        && _278 > mActor->mConst->getTable()->mSlipSpeed && _38 > mStickPos.z && !mDrawStates._D) {
        recordTurnSlipAngle();
    } else if (_3D2 != 0) {
        _3D2--;
    }

    if (mMovementStates._F || mMovementStates._34) {
        _3D2 = 0;
        mMovementStates._10 = false;
    }

    if (_3D2 != 0) {
        if (mStickPos.z < mActor->mConst->getTable()->mTurnSlipNeutral) {
            mMovementStates._10 = true;
        }

        if (_278 < 0.1f) {
            _3D2 = 0;
            _278 = 0.0f;
        }

        if (isAnimationRun("ブレーキ")) {
            inhibitTurn = true;
        }

        if (mMovementStates._10) {
            inhibitTurn = true;
        }
    } else if (!mMovementStates._34 && !mMovementStates._A && mMovementStates._10
        && !mMovementStates._F
        && !mMovementStates._23 && !isStatusActive(0x1F) && turnSlipDot < MR::cos(mActor->mConst->getTable()->mTurnSlipAngle)
        && !mMovementStates._4) {
        if (mMovementStates._35) {
            inhibitTurn = true;
        }

        _220 = _3E4;

        if (!inhibitTurn) {
            TVec3f backTurn(-_220);
            setFrontVecKeepUp(backTurn);
        }

        if (mMovementStates._35) {
            _3D0 = mActor->mConst->getTable()->mTurnSlipTimeB;
            mMovementStates_LOW_WORD |= 0x08000000;
            changeAnimation("ターンブレーキ滑り床", static_cast<const char*>(nullptr));
            _2B8 = mActor->getLastMove();
            stopWalk();
            _754 = 10;
            pushTask(reinterpret_cast<Task>(&Mario::taskOnSlipTurn), 1);
        } else {
            _3D0 = mActor->mConst->getTable()->mTurnSlipTime;
            mMovementStates_LOW_WORD |= 0x08000000;
            _278 = 0.0f;
            changeAnimation("ターンブレーキ", static_cast<const char*>(nullptr));
            playEffect("共通ブレーキ");
        }
    }

    mMovementStates._10 = false;

    if (_3D0 != 0) {
        if (!mMovementStates._35) {
            playSound("ブレーキ", -1);
        }

        if (_3D0 == mActor->mConst->getTable()->mTurnSlipTime) {
            startPadVib(2ul);
        }

        _3D0--;

        if (!mMovementStates._35) {
            _278 = 0.0f;
        }
    }

    if (_3D0 == 0 && mMovementStates._4) {
        if (MR::diffAngleAbsHorizontal(getWorldPadDir(), mFrontVec, *getGravityVec()) > 2.3561945f) {
            stopAnimation("ターンブレーキ", static_cast<const char*>(nullptr));

            TVec3f backFront(-mFrontVec);
            setFrontVecKeepUp(backFront);
            mMovementStates_LOW_WORD &= ~0x08000000;
        }

        if (isAnimationTerminate("ターンブレーキ")) {
            stopAnimation("ターンブレーキ", static_cast<const char*>(nullptr));
            mMovementStates_LOW_WORD &= ~0x08000000;

            if (mStickPos.z < 0.1f) {
                stopWalk();
            }
        }
    }

    if (isActiveTask(reinterpret_cast<Task>(&Mario::taskOnSlipTurn)) && isAnimationRun("ターンブレーキ滑り床")) {
        TVec3f backSlipTurn(-_220);
        setFrontVecKeepUp(backSlipTurn);
        inhibitTurn = true;
        stopAnimation("ターンブレーキ滑り床", static_cast<const char*>(nullptr));
        _754 = 0;
        _74C = 0.0f;
        _278 = 0.0f;
        popTask(reinterpret_cast<Task>(&Mario::taskOnSlipTurn));
    }

    const MarioConstTable* table = mActor->mConst->getTable();
    f32 turnAngleSpeed = table->mTurnAngleSpeed;

    if (_278 > table->mFastTurnSpeed) {
        turnAngleSpeed = table->mTurnAngleSpeed2;

        if (_71C < 5) {
            turnAngleSpeed = table->mTurnAngleSpeedSlowWalk;
        }

        if (isStatusActive(0x1F)) {
            turnAngleSpeed *= 0.5f;
        }
    }

    if (mMovementStates._23) {
        turnAngleSpeed = mActor->mConst->getTable()->mTurnAngleSpeed3;
    }

    if (mMovementStates._F) {
        turnAngleSpeed = mActor->mConst->getTable()->mTurnAngleSpeedTornado;
    }

    if (!mMovementStates._4) {
        TVec3f turnAxis;
        turnAxis.cross(_238, _22C);

        TVec3f rawStick;
        mActor->getStickValue(&rawStick.x, &rawStick.y);
        rawStick.z = 0.0f;

        turnAxis.cross(_250, rawStick);

        bool allowWeakTurn = true;
        if (mMovementStates._37) {
            allowWeakTurn = false;
        }

        if (MR::isNearZero(mStickPos.z, 0.001f)) {
            _10_LOW_WORD &= ~MARIO_MOVE_LOCK_TURN_MASK;
            mMovementStates_LOW_WORD &= ~MARIO_MOVE_TURNING_MASK;
        } else if (MR::isNearZero(_244, 0.001f) && allowWeakTurn) {
            bool stickEqual = false;
            bool stickXYEqual = false;

            if (JGeometry::TUtil< f32 >::epsilonEquals(_250.x, rawStick.x, 0.001f)) {
                if (JGeometry::TUtil< f32 >::epsilonEquals(_250.y, rawStick.y, 0.001f)) {
                    stickXYEqual = true;
                }
            }

            if (stickXYEqual) {
                if (JGeometry::TUtil< f32 >::epsilonEquals(_250.z, rawStick.z, 0.001f)) {
                    stickEqual = true;
                }
            }

            if (!stickEqual) {
                mMovementStates_LOW_WORD |= MARIO_MOVE_TURNING_MASK;
            }
        } else if (turnAxis.dot(_244) > 0.0f && allowWeakTurn) {
            mMovementStates_LOW_WORD |= MARIO_MOVE_TURNING_MASK;
        }

        if (_3D0 != 0) {
            _10_LOW_WORD &= ~MARIO_MOVE_LOCK_TURN_MASK;
            mMovementStates_LOW_WORD &= ~MARIO_MOVE_TURNING_MASK;
        }

        if (MR::isInRange(MR::diffAngleAbs(_214, _22C), 0.0f, 0.707f)) {
            TVec3f frontTurnSide;
            frontTurnSide.cross(_214, _22C);

            if (_274) {
                if (frontTurnSide.dot(mHeadVec) > 0.0f) {
                    _10_LOW_WORD &= ~MARIO_MOVE_LOCK_TURN_MASK;
                }
            } else if (frontTurnSide.dot(mHeadVec) < 0.0f) {
                _10_LOW_WORD &= ~MARIO_MOVE_LOCK_TURN_MASK;
            }
        }

        _250 = rawStick;

        if (mMovementStates_LOW_WORD & MARIO_MOVE_TURNING_MASK) {
            TVec3f turnSide;
            turnSide.cross(_214, _22C);

            if (turnSide.dot(_3D8) < 0.0f) {
                if (turnAxis.dot(_244) <= 0.0f && _214.dot(_22C) >= 0.0f) {
                    _3D4 = 0;
                }
            } else if (_3D4 < mActor->mConst->getTable()->mWeakTurnTime) {
                _3D4++;
            }

            _3D8 = turnSide;
            _244 = turnAxis;
            _238 = _22C;

            f32 weakRatio = 1.0f;
            if (_3D4 < mActor->mConst->getTable()->mWeakTurnTime && _71C > 4) {
                weakRatio = static_cast<f32>(_3D4) / static_cast<f32>(mActor->mConst->getTable()->mWeakTurnTime);
            }

            turnAngleSpeed *= MR::sqrt(weakRatio);
        } else {
            if (_3D4 != 0) {
                _3D4--;
            }

            if (_3D4 == 0) {
                turnAngleSpeed *= 0.01f;
                _244.zero();
                _3D8.cross(_214, _22C);
            }

            _238 = _22C;
        }

        if (mDrawStates._C) {
            TVec3f backFront(-mFrontVec);

            if (MR::diffAngleAbsHorizontal(_8F8, backFront, *getGravityVec()) < 0.7853982f) {
                turnAngleSpeed *= 0.1f;
            } else {
                turnAngleSpeed *= 0.3f;
            }
        }

        TVec3f nextFront;

        if (mMovementStates._37) {
            mDrawStates._D = false;
            _40E = 0;
            _10_LOW_WORD &= ~MARIO_MOVE_LOCK_TURN_MASK;
            nextFront = _22C;
        } else if (_10._C) {
            f32 lockTurn = 0.12f;

            if (!_274) {
                lockTurn = -lockTurn;
            }

            TMtx34f rotMtx;
            PSMTXRotAxisRad(rotMtx, &mHeadVec, lockTurn);
            PSMTXMultVecSR(rotMtx, &_214, &nextFront);
        } else {
            f32 frontTurnAngle = MR::diffAngleAbs(mFrontVec, _22C);

                if (frontTurnAngle < 0.1f) {
                    if (_524 == _528) {
                        nextFront = _22C;
                    } else {
                        nextFront = mFrontVec;
                    }
                } else {
                    frontTurnAngle = MR::diffAngleAbs(mFrontVec, _22C);

                    if (frontTurnAngle > 0.0f) {
                        f32 blend = turnAngleSpeed / frontTurnAngle;
                        MR::clamp01(&blend);

                        if (mActor->mAlphaEnable && blend > 0.1f) {
                            blend = 0.1f;
                        }

                        if (getPlayer()->_10._12) {
                            const MarioConstTable* heavyTable = mActor->mConst->getTable();
                            f32 heavyRatio = heavyTable->mStickHeavyMinRatio;

                            if (frontTurnAngle >= heavyTable->mStickHeavyMaxAngle) {
                                heavyRatio = 1.0f;
                            } else if (frontTurnAngle > heavyTable->mStickHeavyMinAngle) {
                                heavyRatio += (1.0f - heavyRatio)
                                    * ((frontTurnAngle - heavyTable->mStickHeavyMinAngle)
                                       / (heavyTable->mStickHeavyMaxAngle - heavyTable->mStickHeavyMinAngle));
                            }

                            blend *= heavyRatio;

                            f32 brake = 1.0f - ((PI - frontTurnAngle) / PI);
                            MR::clamp01(&brake);
                            _278 *= 1.0f - (0.1f * brake);
                        }

                        if (!MR::vecBlendSphere(mFrontVec, _22C, &nextFront, blend)) {
                            _3D4 = 0;
                            MR::vecRotAxis(_214, _22C, mHeadVec, &nextFront, 0.3926991f);
                        }

                        if (frontTurnAngle > 2.5f && _3D4 == mActor->mConst->getTable()->mWeakTurnTime && !mDrawStates._D) {
                            TVec3f frontSide;
                            frontSide.cross(_214, nextFront);
                            _274 = frontSide.dot(mHeadVec) > 0.0f;
                            _10_LOW_WORD |= MARIO_MOVE_LOCK_TURN_MASK;
                        }
                    }
                }
            }

        mMovementStates_LOW_WORD |= MARIO_MOVE_TURNING_MASK;

        if (!isAnimationRun("その場足踏み") && frontDot > 0.99f) {
            mMovementStates_LOW_WORD &= ~MARIO_MOVE_TURNING_MASK;
        }

        if (mMovementStates._37 && mFrontVec.dot(nextFront) < 0.0f && !isAnimationRun("ブレーキ")) {
            _750 = 10;
            _74C = PI;
        }

        if (!inhibitTurn) {
            setFrontVecKeepUp(nextFront);
        }
    } else {
        if (_3D0 == 0) {
            TVec3f nextFront;

            if (!MR::vecBlendSphere(_214, _22C, &nextFront, turnAngleSpeed)) {
                MR::vecRotAxis(_214, _22C, mHeadVec, &nextFront, 0.3926991f);
            }

            if (!inhibitTurn) {
                setFrontVecKeepUp(nextFront);
            }
        }

        _10_LOW_WORD &= ~MARIO_MOVE_LOCK_TURN_MASK;
        mMovementStates_LOW_WORD &= ~MARIO_MOVE_TURNING_MASK;
    }

    if (_750 != 0 && _71C == 0 && isEnableTurn()) {
        changeAnimation("その場足踏み", static_cast<const char*>(nullptr));

        if (!isAnimationRun("カリカリ限界")) {
            changeAnimationUpperWeak("その場足踏み上半身", static_cast<const char*>(nullptr));
        }
    } else if (_71C != 0) {
        stopAnimation("その場足踏み", static_cast<const char*>(nullptr));
    } else if (isAnimationRun("その場足踏み上半身")) {
        stopAnimationUpper(static_cast<const char*>(nullptr), static_cast<const char*>(nullptr));
    }

    TVec3f velocity;

    if (mMovementStates._34) {
        TVec3f target(mFrontVec);
        TVec3f base(mFrontVec);

        if (!MR::isNearZero(_16C, 0.001f)) {
            base = _16C;
        }

        MR::normalize(&base);

        f32 blend = (1.1f - _278) * mActor->mConst->getTable()->mInertiaIceTurn;
        MR::clamp01(&blend);
        MR::vecBlendSphere(base, target, &velocity, blend);
        MR::normalize(&velocity);
        velocity.scale(_278 * mActor->mConst->getTable()->mWalkSpeed);
    } else if (mDrawStates._5) {
        f32 speedRatio = _278;
        f32 walkSpeed = mActor->mConst->getTable()->mWalkSpeed;
        TVec3f scaled(_22C);
        scaled.scale(walkSpeed);

        TVec3f result(scaled);
        result.scale(speedRatio);
        velocity = result;
    } else {
        f32 speedRatio = _278;
        f32 walkSpeed = mActor->mConst->getTable()->mWalkSpeed;
        TVec3f scaled(mFrontVec);
        scaled.scale(walkSpeed);

        TVec3f result(scaled);
        result.scale(speedRatio);
        velocity = result;
    }

    addVelocity(velocity);
    _328 = moveDir;

    if (mMovementStates._A) {
        if (_71C != 0) {
            _334 = moveDir;
        }
    } else if (_3D2 == 0) {
        _334 = moveDir;
    }

    if (mMovementStates._32 && _4E0 > 120.0f) {
        if (mVelocity.dot(*mFrontWallTriangle->getNormal(0)) < 0.0f) {
            const TVec3f& wallNormal = *mFrontWallTriangle->getNormal(0);
            TVec3f wallTangent(mPosition - _4E8);
            MR::vecKillElement(wallTangent, wallNormal, &wallTangent);
            MR::vecKillElement(wallTangent, *getGravityVec(), &wallTangent);
            MR::vecKillElement(wallTangent, mFrontVec, &wallTangent);
            MR::normalizeOrZero(&wallTangent);

            if (!MR::isNearZero(wallTangent, 0.001f)) {
                f32 speed = mVelocity.length();
                TVec3f nextVelocity(wallTangent);
                nextVelocity.scale(speed);
                mVelocity = nextVelocity;
            }
        }
    } else if (mMovementStates._8) {
        if (mVelocity.dot(*mFrontWallTriangle->getNormal(0)) < 0.0f) {
            f32 wallDot = -mFrontVec.dot(*mFrontWallTriangle->getNormal(0));
            f32 wallPushAngle = mActor->mConst->getTable()->mWallPushAngleRange;

            if (wallDot < MR::cosDegree(wallPushAngle)) {
                MR::vecKillElement(mVelocity, *mFrontWallTriangle->getNormal(0), &mVelocity);
            }
        }
    }

    TVec3f velocityDir(mVelocity);
    f32 velocitySpeed = velocityDir.length();
    MR::normalizeOrZero(&velocityDir);

    TVec3f velocitySide;
    velocitySide.cross(_368, velocityDir);
    MR::normalizeOrZero(&velocitySide);
    velocityDir.cross(velocitySide, _368);
    velocityDir.setLength(velocitySpeed);
    mVelocity = velocityDir;

    checkLockOnHoming();
    fixPositionInTower();
}

bool Mario::isEnableTurn() {
    if (!mMovementStates._1) {
        return false;
    }

    if (mMovementStates._4) {
        return false;
    }

    if (mDrawStates._5) {
        return false;
    }

    if (mMovementStates._23) {
        return false;
    }

    if (mMovementStates._34) {
        return false;
    }

    if (mDrawStates._A) {
        return false;
    }

    if (mMovementStates._A) {
        return false;
    }

    if (isStatusActive(0x11)) {
        return false;
    }

    if (isAnimationRun("坂すべり上向きうつぶせ", 2)) {
        return false;
    }

    if (isAnimationRun("坂すべり下向きあおむけ", 3)) {
        return false;
    }

    if (isAnimationRun("坂すべり下向き終了")) {
        return false;
    }

    if (isAnimationRun("坂すべり上向き終了")) {
        return false;
    }

    if (isAnimationRun("飛び込み失敗着地")) {
        return false;
    }

    if (isAnimationRun("飛び込み失敗回転着地")) {
        return false;
    }

    if (mActor->_480) {
        return false;
    }

    if (mActor->isPunching()) {
        return false;
    }

    if (mActor->isItemSwinging()) {
        return false;
    }

    if (mActor->mAlphaEnable) {
        return false;
    }

    if (mActor->_3C0) {
        return false;
    }

    if (mActor->_EA4) {
        return false;
    }

    if (mMovementStates._37) {
        return false;
    }

    if (_10._15) {
        return false;
    }

    return true;
}

void Mario::recordTurnSlipAngle() {
    if (!mActor->mAlphaEnable) {
        _3E4 = mFrontVec;
        _3D2 = mActor->getConst().getTable()->mTurnReadyTime;
    }
}

f32 Mario::decideInertia(f32 stickPower) {
    if (isStatusActive(0x1F)) {
        return decideInertiaOnIce(stickPower);
    }

    if (_278 > 1.0f) {
        return mActor->mConst->getTable()->mInertiaOverSpeed;
    }

    if (mMovementStates._34) {
        return decideInertiaOnIce(stickPower);
    }

    if (mMovementStates._35) {
        return decideInertiaOnSlip(stickPower);
    }

    MarioConst* consts = mActor->mConst;
    const MarioConstTable* table = consts->getTable();
    f32 inertia = ((1.0f - _278) * table->mInertiaStandardStop + _278 * table->mInertiaStandardMax) * (1.0f - _3F4)
        + _3F4 * table->mInertiaStartSpin;

    if (stickPower == 0.0f) {
        inertia = table->mInertiaStop;

        if (mMovementStates._10) {
            inertia = consts->getTable()->mInertiaTurnSlip;
        }

        if (mMovementStates._4) {
            inertia = mActor->mConst->getTable()->mInertiaTurning;
        }

        if (_3CE < 30) {
            inertia = mActor->mConst->getTable()->mInertiaJumpFinish;
        }

        if (mMovementStates._A) {
            inertia = mActor->mConst->getTable()->mInertiaSquat;
        }
    }

    if (mMovementStates._F) {
        if (_278 >= stickPower) {
            inertia = mActor->mConst->getTable()->mInertiaTornadoBrake;
        } else {
            inertia = mActor->mConst->getTable()->mInertiaTornadoAccel;
        }
    }

    if (_3F8 != 0) {
        _3F8--;
        inertia = mActor->mConst->getTable()->mInertiaReflectSlip;
    }

    if (_278 < 0.08f && _3CE > 10 && stickPower > 0.5f && (mMovementStates._A || mMovementStates._C)) {
        _3FA = consts->getTable()->mStartSpinTime;
        _278 = 0.08f;
    }

    if (_3FA != 0) {
        _3FA--;
        inertia = mActor->mConst->getTable()->mInertiaStartSpin;

        if (_3FA == 0) {
            _3FC = 60;
        }
    }

    if (_3FC != 0) {
        _3FC--;
    }

    if (getFloorCode() == 0x20 && _278 > 0.4f) {
        inertia *= 0.5f;
    }

    return inertia;
}

f32 Mario::decideInertiaOnIce(f32 stickPower) {
    if (stickPower > 1.0f) {
        stickPower = 1.0f;
    }

    const MarioConstTable* table = mActor->mConst->getTable();
    f32 standard = (1.0f - stickPower) * table->mInertiaIceStandardStop + stickPower * table->mInertiaIceStandardMax;
    f32 inertia = (1.0f - _3F4) * standard + _3F4 * table->mInertiaIceStartSpin;

    if (0.0f == stickPower) {
        return table->mInertiaIceStop;
    }

    return inertia;
}

f32 Mario::decideInertiaOnSlip(f32 stickPower) {
    const MarioConstTable* table = mActor->mConst->getTable();
    f32 standard = (1.0f - _278) * table->mInertiaSlipStandardStop + _278 * table->mInertiaSlipStandardMax;
    f32 inertia = (1.0f - _3F4) * standard + _3F4 * table->mInertiaSlipStartSpin;

    if (stickPower == 0.0f) {
        inertia = table->mInertiaSlipStop;
    }

    return inertia;
}

void Mario::calcShadowDir(const TVec3f& rMoveDir, TVec3f* pOut) {
    TVec3f moveDir(rMoveDir);

    if (!MR::isNormalize(moveDir, 0.001f)) {
        MR::normalizeOrZero(&moveDir);
    }

    if (mMovementStates._37 || _10._15) {
        calcShadowDir2D(moveDir, pOut);
        return;
    }

    TVec3f base(-*getGravityVec());
    const f32 gravityDot = moveDir.dot(base);
    moveDir.dot(mHeadVec);
    const f32 airDot = moveDir.dot(_368);

    if (__fabsf(gravityDot) > __fabsf(airDot)) {
        base = _368;
    }

    TVec3f side;
    side.cross(base, moveDir);
    pOut->cross(side, base);
    MR::normalizeOrZero(pOut);
}

bool Mario::retainMoveDir(f32 stickX, f32 stickY, TVec3f* pOut) {
    f32 stickAngle = JMath::sAtanTable.atan2_(stickY, stickX);
    f32 stickAngleDiff = MR::diffAngleAbs(stickAngle, _2B4);

    if (isAnimationRun("ターン")) {
        stickAngleDiff = 1.0f;
    }

    if (isAnimationRun("ターンブレーキ")) {
        stickAngleDiff = 1.0f;
    }

    if (isSwimming()) {
        stickAngleDiff = 1.0f;
    }

    if (mActor->isRequestSpin() && mMovementStates.jumping && mWorldPadDir.dot(_16C) < 0.0f) {
        stickAngleDiff = 1.0f;
    }

    f32 retainAngle = 0.99f;
    if (_3CE < 2) {
        retainAngle = 0.1f;
    }

    if (_10._B) {
        if (isStickOn()) {
            _10._B = false;
        }
        stickAngleDiff = 1.0f;
    }

    bool enableRetain = true;
    mDrawStates._9 = true;

    if (mMovementStates.jumping && !_10._A) {
        enableRetain = false;
    }

    if (mMovementStates.jumping && mMovementStates._8) {
        enableRetain = false;
    }

    if (mMovementStates.jumping && !isAnimationRun(nullptr)) {
        enableRetain = false;
    }

    if (stickAngleDiff < retainAngle && isStickOn() && enableRetain) {
        if (_40E == 0 || _10._A && mMovementStates.jumping) {
            _40E = 0;
            mDrawStates._D = true;
        } else {
            _40E--;
            _29C = _368;
            _2A8 = *getGravityVec();
        }

        _2B4 = stickAngle;
        return false;
    }

    if (mMovementStates.jumping) {
        if (isStickOn()) {
            _10._A = false;
        }
    } else {
        _10._A = true;
    }

    if (_40E == 0) {
        _3D4 = 0;
    }

    _40E = 30;

    if (!isStickOn()) {
        _10._B = true;
    }

    _2B4 = stickAngle;
    _29C = _368;
    _2A8 = *getGravityVec();
    return false;
}

void Mario::calcMoveDir(f32 stickX, f32 stickY, TVec3f* pOut, bool doRetain) {
    if (mMovementStates._37) {
        calcDir2D(stickX, stickY, pOut);
        return;
    }

    if (_10._15) {
        calcMoveDir2D(stickX, stickY, pOut);
        return;
    }

    if (mMovementStates._3A) {
        calcMoveDir25D(stickX, stickY, pOut);
        return;
    }

    if (doRetain) {
        if (retainMoveDir(stickX, stickY, pOut)) {
            return;
        }
    }

    TVec3f camX(getCamDirX());
    const TVec3f& camY = getCamDirY();
    TVec3f camZ(-getCamDirZ());
    TVec3f minusGravity(-*getGravityVec());

    if (!MR::isNearZero(minusGravity)) {
        if (_398.dot(minusGravity) <= 0.0f) {
            _398 = minusGravity;
        } else {
            MR::vecBlendSphere(_398, minusGravity, &_398, 0.1f);
        }

        MR::normalizeOrZero(&_398);
    }

    const f32 yDot = camY.dot(_398);
    const f32 zDot = camZ.dot(_398);

    TVec3f base;
    TVec3f side;

    if (__fabsf(zDot) > __fabsf(yDot)) {
        base = camY;

        if (zDot < 0.0f) {
            base = -base;
        }

        side.cross(base, _398);
    } else {
        base = -camZ;

        if (yDot < 0.0f) {
            base = -base;
            side.cross(camZ, _398);
        } else {
            TVec3f negCamZ(-camZ);
            side.cross(negCamZ, _398);
        }
    }

    TVec3f front;
    front.cross(camX, _398);
    MR::normalizeOrZero(&side);

    if (MR::isNearZero(side)) {
        side = camX;
    }

    MR::normalizeOrZero(&front);

    if (MR::isNearZero(front)) {
        front = base;
    }

    TVec3f stickYVec(front);
    stickYVec.scale(stickY);

    TVec3f stickXVec(side);
    stickXVec.scale(stickX);

    *pOut = stickXVec - stickYVec;

    if ((_10_LOW_WORD & 0x00001000) != 0) {
        TVec3f towerDiff(mPosition - _6F4);
        MR::vecKillElement(towerDiff, _700, &towerDiff);
        MR::normalizeOrZero(&towerDiff);
        MR::vecKillElement(*pOut, _700, pOut);
        MR::vecKillElement(*pOut, towerDiff, pOut);
    }
}

bool Mario::checkLockOnHoming() {
    if (mStickPos.z != 0.0f) {
        return false;
    }

    if (!checkPreLvlZ()) {
        return false;
    }

    if (isSwimming()) {
        return false;
    }

    if (mActor->_470 == nullptr) {
        return false;
    }

    mMovementStates_LOW_WORD &= ~0x00200000;
    mDrawStates_WORD |= 0x04000000;
    doLockOnHoming();
    return true;
}

void Mario::doLockOnHoming() {
    TVec3f targetDiff(*reinterpret_cast<TVec3f*>(mActor->_470 + 4) - mPosition);
    TVec3f front;
    front.x = targetDiff.x;
    front.y = targetDiff.y;
    front.z = targetDiff.z;
    MR::normalize(&front);

    TVec3f shadowFront;
    calcShadowDir(front, &shadowFront);

    if (MR::diffAngleAbsHorizontal(mFrontVec, front, mHeadVec) >= 0.05235988f && getAnimator()->isAnimationStop()) {
        changeAnimation("その場足踏み", static_cast<const char*>(nullptr));

        if (_750 == 0) {
            setFrontVecKeepUp(front, 15ul);
            _334 = front;
        }
    }
}

#pragma dont_inline on
void Mario::fixPositionInTower() {
    if ((_10_LOW_WORD & 0x00001000) == 0) {
        return;
    }

    TVec3f towerDiff(mPosition - _6F4);
    const f32 axialLength = MR::vecKillElement(towerDiff, _700, &towerDiff);
    towerDiff.setLength(_718 - 200.0f);

    TVec3f axial(_700);
    axial.scale(axialLength);
    towerDiff += axial;

    TVec3f nextPos(_6F4);
    nextPos += towerDiff;
    mPosition = nextPos;
}
#pragma dont_inline reset
