#include "Game/Enemy/KarikariDirector.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include <cstring>
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#include <stdexcept>
#else  // SMGPC_RETAIL_SOURCE
#endif  // SMGPC_PC_DIVERGENCE

extern u8 lbl_806B6288;

namespace {
    f32 sSpeedTableA[] = {0.15f, 0.3f, 0.45f, 0.6f, 0.7f, 0.85f, 0.99f};
    f32 sSpeedTableB[] = {0.02f, 0.2f, 0.4f, 0.5f, 0.65f, 0.75f, 0.98f};
    f32 sWalkTargetTable[] = {0.0f, 0.15f, 0.25f, 0.4f, 0.5f, 0.6f, 0.8f, 1.0f};
    f32 sWeightTable[][4] = {
        {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.0f},   {0.75f, 0.25f, 0.0f, 0.0f}, {0.25f, 0.75f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.75f, 0.25f, 0.0f}, {0.0f, 0.25f, 0.75f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f},
    };
    f32 sFootStep[] = {1.2f, 1.5f, 1.3f, 0.0f};
    f32 sFootStepBeeWallWalk[] = {0.5f, 0.5f, 0.5f, 0.0f};
};  // namespace

void Mario::stopWalk() {
    _71C = false;
    _278 = 0.0f;
    _71E = 0;
    _71F = 0;
    _3D4 = 0;
    getAnimator()->initWalkWeight();
    getAnimator()->resetTilt();
    mMovementStates._10 = false;
    mMovementStates._23 = false;
    cancelSquatMode();
    _3D2 = 0;
    mMovementStates._20 = false;
    _8F0 = 0.0f;
    _3F4 = 0.0f;
    stopEffect("共通スリップ坂");
    stopAnimation("歩行制動ブレーキ", 1);
    stopAnimation("ブレーキ", static_cast< const char* >(nullptr));
}

void Mario::cancelSquatMode() {
    if (!mMovementStates._A) {
        return;
    }

    calcDistToCeil(false);
    if ((_20 & 0x200000) != 0 || mMovementStates._A) {
        _10_LOW_WORD |= 0x10000;
    }

    mMovementStates._A = false;
    _20 &= ~0x200000;
    stopAnimation("しゃがみ基本", static_cast< const char* >(nullptr));

    if (mMovementStates._1 || !mMovementStates.jumping) {
        if (!isSwimming()) {
            changeAnimation(nullptr, "基本");
        }

        if (!isAnimationRun("サマーソルト") && (_10_LOW_WORD & 0x10000) != 0) {
            if (_278 > 0.1f) {
                changeAnimationUpperWeak("しゃがみ終了", nullptr);
            } else {
                changeAnimation("しゃがみ終了", static_cast< const char* >(nullptr));
            }
        }
    }

    mMovementStates._19 = false;
}

f32 Mario::getTargetWalkSpeed() const {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (mMovementStates._A || _735 != 0 || _2D0 != 0.0f || _434 != 0) {
        throw std::logic_error("modified Mario target speed is unavailable in the PC walk slice");
    }
    return sWalkTargetTable[_71C];
#else  // SMGPC_RETAIL_SOURCE
    if (mMovementStates._A) {
        return 0.0f;
    }

    f32 speed = sWalkTargetTable[_71C];
    if (_735 != 0) {
        if (checkCurrentFloorCodeSevere(25)) {
            speed = 0.0f;
        } else if (checkCurrentFloorCodeSevere(31)) {
            speed = 0.0f;
        } else {
            speed *= 1.0f - static_cast< f32 >(_735) * (1.0f / 256.0f);
        }
    }

    speed *= 1.0f - _2D0;
    if (_434 != 0) {
        speed *= mActor->getConst().getTable()->mItemDashRatio;
    }

    return speed;
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::decideSquatWalkAnimation() {
    f32 waitWeight[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    f32 walkWeight[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    _3F4 = 0.0f;

    if (!mMovementStates._A) {
        if (_278 > 0.1f) {
            stopAnimation("しゃがみ", "基本");
            changeAnimationUpperWeak("しゃがみ終了", nullptr);
        } else {
            changeAnimation("しゃがみ終了", "基本");
        }
        mMovementStates._19 = false;
        return;
    }

    if (isAnimationRun("壁押し", 0)) {
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
    }

    if ((_20 & 0x200000) == 0 || !isAnimationRun("しゃがみ基本")) {
        stopAnimation("歩行制動ブレーキ", 1);
        changeAnimation(nullptr, "しゃがみ基本");
        stopAnimation("飛び込み準備", 4);
        getAnimator()->setWalkWeight(waitWeight);
        mMovementStates._19 = false;

        if (!mMovementStates.jumping && !isAnimationRun("サマーソルト")) {
            playSound("声しゃがむ", -1);
        }
    }

    if (_278 < mActor->getConst().getTable()->mSpeedSquatWalkLower) {
        if (isStickOn()) {
            _278 = mActor->getConst().getTable()->mSquatWalkMinSpeed;
            getAnimator()->setWalkWeight(walkWeight);
            _71C = 1;
            mMovementStates._19 = true;
        } else {
            getAnimator()->setWalkWeight(waitWeight);
            _71C = 0;
            mMovementStates._19 = false;
        }
    } else if (!isStickOn()) {
        getAnimator()->setWalkWeight(waitWeight);
        _71C = 0;
    } else if (mMovementStates._19) {
        const MarioConstTable* pTable = mActor->getConst().getTable();
        _278 = pTable->mSquatWalkMinSpeed + (mStickPos.z * (pTable->mSquatWalkMaxSpeed - pTable->mSquatWalkMinSpeed));
        getAnimator()->setWalkWeight(walkWeight);
        _71C = 1;
    }

    if (isAnimationRun("しゃがみ基本")) {
        f32 animationSpeed = (60.0f / mActor->getConst().getTable()->mSquatWalkStep) * _278;
        if (!mMovementStates._19) {
            animationSpeed = 1.0f;
        }
        getAnimator()->mXanimePlayer->changeSpeed(animationSpeed);
    }

    if (getFloorCode() == 32 && _71C != 0) {
        _3F4 = 0.2f;
    }

    const f32 frame = getAnimator()->getFrame();
    if (_71C != 0) {
        if (mDrawStates.mIsUnderwater || mDrawStates._13) {
            if (_27C > frame) {
                playSound("水跳ね左足小", -1);
                playEffect("水はね左弱");
                const TVec3f effectPos = (mGroundPos - (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
            }

            if (_27C < 30.0f && frame >= 30.0f) {
                playSound("水跳ね右足小", -1);
                playEffect("水はね右弱");
                const TVec3f effectPos = (mGroundPos + (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
            }
        }
    } else if (mDrawStates.mIsUnderwater && (mActor->_37C & 0x3F) == 0) {
        const TVec3f randomSide = mSideVec * (MR::getRandom() - 0.5f);
        const TVec3f effectPos = mShadowPos + (randomSide * 20.0f) + (_368 * _738);
        playEffectSRT("水波紋", 0.2f, _73C, effectPos);
    }

    _27C = frame;
}

void Mario::decideWalkSpeed() {
    s32 lowerSpeed = 0;
    const u8 oldSpeed = _71C;
    if (oldSpeed != 0 && mStickPos.z < sSpeedTableB[oldSpeed - 1]) {
        lowerSpeed = 1;
    }

    u32 speed = 0;
    for (u32 i = 0; i < 7; i++) {
        if (mStickPos.z < sSpeedTableA[i]) {
            break;
        }
        speed++;
    }

    if (oldSpeed <= speed || lowerSpeed) {
        _71C = speed;
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    return;
#else  // SMGPC_RETAIL_SOURCE
    s32 clingNum = MR::getKarikariClingNum();
    if (clingNum != 0) {
        if (clingNum > 5) {
            clingNum = 5;
        }

        const s32 maxSpeed = 5 - clingNum;
        if (_71C > maxSpeed) {
            _71C = maxSpeed;
        }
    }

    if (mDrawStates.mIsUnderwater && _71C > 6) {
        _71C = 6;
    }

    if (mActor->mAlphaEnable && _71C > 4) {
        _71C = 4;
    }

    if (_960 == 32) {
        if (_71C > 3) {
            _71C = 3;
        }

        if (_71C != 0) {
            startPadVib(1);
        }

        if (_71C > 2) {
            XanimePlayer* pPlayer = getAnimator()->mXanimePlayer;
            pPlayer->_0C = 0.5f;
        } else {
            XanimePlayer* pPlayer = getAnimator()->mXanimePlayer;
            pPlayer->_0C = 1.0f;
        }
        return;
    }

    f32 speedRate = getAnimator()->mXanimePlayer->_0C + 0.1f;
    if (speedRate > 1.0f) {
        speedRate = 1.0f;
    }
    getAnimator()->mXanimePlayer->_0C = speedRate;
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::decideWalkAnimation() {
    if (_71C == 0 && _278 < 0.2f && isBlendWaitGround()) {
        getAnimator()->controlWaitAnimation();
    } else {
        getPlayer()->_10_LOW_WORD &= ~0x10000;
        if (getPlayer()->_71C == 0 && mSwim->_1B2 && !isPlayerModeBee()) {
            changeAnimation("飛び込み準備", 4);
            return;
        }

        if (_735 == 0) {
            if (getPlayer()->_71C != 0) {
                getAnimator()->stopWaitAnimation();
            }
            getAnimator()->setWalkWeight(sWeightTable[_71C]);
        } else {
            f32 weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            if (_71C != 0) {
                if (_71C <= 3) {
                    f32 weight = 1.0f;
                    if (_71C == 2) {
                        weight = 0.75f;
                    } else if (_71C == 3) {
                        weight = 0.5f;
                    }
                    weights[0] = weight;
                    weights[1] = 1.0f - weight;
                } else {
                    f32 weight = 1.0f;
                    if (_71C >= 5) {
                        weight = static_cast< f32 >(_735) / 100.0f;
                    }
                    weight = MR::clamp(weight, 0.0f, 1.0f);
                    weights[1] = weight;
                    weights[2] = 1.0f - weight;
                }
            } else {
                weights[3] = 1.0f;
            }
            getAnimator()->setWalkWeight(weights);
        }
    }

    stopAnimation("飛び込み準備", 4);

    const f32 frame = getAnimator()->getFrame();
    if (_71C != 0) {
        if (mDrawStates.mIsUnderwater || mDrawStates._13) {
            if (_27C > frame) {
                if (_71C >= 2) {
                    if (_71C < 6) {
                        playSound("水跳ね左足小", -1);
                        playEffect("水はね左弱");
                    } else {
                        playSound("水跳ね左足", -1);
                        playEffect("水はね左");
                    }
                }
                const TVec3f effectPos = (mGroundPos - (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
            }

            if (_27C < 30.0f && frame >= 30.0f) {
                if (_71C >= 2) {
                    if (_71C < 6) {
                        playSound("水跳ね右足小", -1);
                        playEffect("水はね右弱");
                    } else {
                        playSound("水跳ね右足", -1);
                        playEffect("水はね右");
                    }
                }
                const TVec3f effectPos = (mGroundPos + (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
            }
        }
    } else if (mDrawStates.mIsUnderwater && (mActor->_37C & 0x3F) == 0) {
        const TVec3f randomSide = mSideVec * (MR::getRandom() - 0.5f);
        const TVec3f effectPos = mShadowPos + (randomSide * 20.0f) + (_368 * _738);
        playEffectSRT("水波紋", 0.2f, _73C, effectPos);
    }
    _27C = frame;

    const f32* pFootStep = mActor->mAlphaEnable ? sFootStepBeeWallWalk : sFootStep;
    f32 stepLength = 0.0f;
    for (u32 i = 0; i < 4; i++) {
        stepLength += pFootStep[i] * sWeightTable[_71C][i];
    }

    f32 animationSpeed;
    if (stepLength == 0.0f) {
        animationSpeed = 0.33f;
    } else {
        animationSpeed = 0.5f * (60.0f * ((0.01f * (_278 * mActor->getConst().getTable()->mWalkSpeed)) / stepLength));
    }

    if (_8F0 > 0.0f && !mDrawStates._4) {
        animationSpeed *= 1.0f + ((_8F0 / 10.0f) * mActor->getConst().getTable()->mSlopeAnimeRatio);
        if (_8F0 > 5.0f && animationSpeed > 4.0f) {
            animationSpeed *= mActor->getConst().getTable()->mSlopeSpinAnimeRatio;
            changeAnimation("がんばり走り", static_cast< const char* >(nullptr));
            startBas("RunSlope", false, 0.0f, 0.0f);
        }
    } else if (_3FE != 0) {
        animationSpeed *= _8F4;
    }

    f32 speedDelta = getTargetWalkSpeed() - _278;
    if (_278 > sWalkTargetTable[5]) {
        const f32 blend = (_278 - sWalkTargetTable[5]) / (1.0f - sWalkTargetTable[5]);
        speedDelta = speedDelta * (1.0f - blend) + speedDelta * speedDelta * blend;
    } else if (_278 < sWalkTargetTable[3]) {
        const f32 adjustedDelta = MR::sqrt(speedDelta);
        const f32 blend = (sWalkTargetTable[3] - _278) / sWalkTargetTable[3];
        speedDelta = speedDelta * blend + adjustedDelta * (1.0f - blend);
    }

    const f32 startSpinAnimeRatio = mActor->getConst().getTable()->mStartSpinAnimeRatio;
    f32 speedBlend = 1.0f + 4.0f * speedDelta;
    if (speedBlend > 2.0f) {
        speedBlend = 2.0f;
    }
    if (speedBlend < 1.0f) {
        speedBlend = 1.0f;
    }
    if (getTargetWalkSpeed() < sWalkTargetTable[6]) {
        speedBlend = 1.0f;
    }
    if (getFloorCode() == 32 && _71C != 0) {
        speedBlend = 1.2f;
    }

    const f32 blend = 2.0f - speedBlend;
    const f32 minimumSpeed =
        startSpinAnimeRatio * (0.5f * (60.0f * ((0.01f * (getTargetWalkSpeed() * mActor->getConst().getTable()->mWalkSpeed)) / pFootStep[2])));
    if (animationSpeed < minimumSpeed) {
        animationSpeed = animationSpeed * blend + minimumSpeed * (1.0f - blend);
    }
    if (_735 != 0 && _71C != 0 && animationSpeed < 1.0f) {
        animationSpeed = 1.0f;
    }

    _3F4 = speedBlend - 1.0f;
    getAnimator()->mXanimePlayer->changeSpeed(animationSpeed * (1.0f - _2D0));

    if (!mActor->_EA4 && _71C == 0 && mActor->mHealth == 1 && mActor->mMaxHealth > 2 && (_970 == nullptr || strcmp(_970, "DamageWait") != 0)) {
        getAnimator()->mXanimePlayer->changeTrackAnimation(3, "ダメージウエイト");
        startBas("DamageWait", false, 0.0f, 0.0f);
        mActor->setBlink("DamageWait");
    }

    checkWallPush();
    f32 brakeSpeed = 0.9f;
    if (lbl_806B6288) {
        brakeSpeed = 0.3f;
    }
    if (_71C > 5 && _278 > brakeSpeed && !mDrawStates._4 && !mMovementStates._35) {
        _71E = mActor->getConst().getTable()->mBrakeFirstTimer;
    }
    if (_71E != 0) {
        _71E--;
    }

    if (_71C == 0) {
        if (_71E != 0) {
            if (!isSlipPolygon(mGroundPolygon) && !mDrawStates._5) {
                doBrakingAnimation();
                _71F = mActor->getConst().getTable()->mBrakeSecondTimer;
            }
        }
        _71E = 0;

        const s32 clingNum = MR::getKarikariClingNum();
        if (clingNum >= 1) {
            changeAnimationUpper("カリカリ限界", nullptr);
            stopAnimation("歩行制動ブレーキ", 1);
        }
        if (clingNum < 1 && isAnimationRun("カリカリ限界")) {
            stopAnimationUpper("カリカリ限界", nullptr);
        }
    }
}

void Mario::doBrakingAnimation() {
    changeAnimation("歩行制動ブレーキ", 1);
    getAnimator()->mXanimePlayer->_20->setAttribute(1);
    if (lbl_806B6288) {
        getAnimator()->mXanimePlayer->changeSpeed(0.5f);
    }
    playEffect("共通ブレーキ");
    _71F = 0;
}

void Mario::checkWallPush() {
    if (_71C != 0 && (mMovementStates._8 || mMovementStates._32) && checkWallJumpCode()) {
        return;
    }

    const TVec3f wallDir = -getWallNorm();
    const f32 angle = MR::diffAngleAbsHorizontal(mFrontVec, wallDir, *getGravityVec());
    const MarioConst& marioConst = mActor->getConst();
    bool pushWall = false;
    bool isPushing = false;
    const f32 wallPushAngleRange = marioConst.getTable()->mWallPushAngleRange;

    if (_71C != 0 && mMovementStates._8) {
        isPushing = true;
    }
    if (isPushing && angle < PI_180 * wallPushAngleRange) {
        pushWall = true;
    }

    if (mDrawStates._A) {
        pushWall = false;
    }
    if (mDrawStates._C) {
        pushWall = false;
    }

    if (calcAngleD(getWallNorm()) < marioConst.getTable()->mForceWallAngle) {
        pushWall = false;
        if (mMovementStates._8 && _71C != 0) {
            _71C = 1;
            _278 = 0.0f;
        }
    }

    if (!isAnimationRun("壁押し", 0) && pushWall) {
        doSideStep();
    }
}

void Mario::updateBrakeAnimation() {
    if (_71F != 0) {
        if (!isAnimationRun("歩行制動ブレーキ", 1)) {
            _71F = 0;
        } else {
            _71F--;
            if (!MR::isNearZero(mStickPos.z, 0.001f)) {
                _71F = 0;
            }

            if (_71F == 0) {
                stopAnimation(nullptr, static_cast< const char* >(nullptr));
                stopWalk();
            }
        }
    } else if (isAnimationRun("歩行制動ブレーキ", 1) && (isAnimationTerminate(nullptr) || _71C != 0)) {
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
    }

    if (!lbl_806B6288 || (!isAnimationRun("歩行制動ブレーキ", 1) && !isAnimationRun("ブレーキ"))) {
        return;
    }

    if (mMovementStates._8 || mMovementStates._32) {
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
        _71F = 0;
        _71E = 0;
        _3D0 = 0;
        _3D2 = 0;
    } else if (!MR::isDemoActive() && mMovementStates._1) {
        playSound("ルイージ滑り", -1);
    }
}

void Mario::updateWalkSpeed() {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (mMovementStates._F || mMovementStates._A || _735 != 0 || getPlayerMode() != 0) {
        throw std::logic_error("special Mario walk-speed mode is unavailable in the PC walk slice");
    }

    f32 targetSpeed = getTargetWalkSpeed();
    f32 startRatio = 1.0f;
    if (targetSpeed == 0.0f) {
        _404 = mActor->getConst().getTable()->mSlowStartTime;
    }
    if (_404 != 0) {
        const u16 slowStartTime = mActor->getConst().getTable()->mSlowStartTime;
        const u16 timer = _404;
        _404--;
        startRatio = static_cast< f32 >(slowStartTime - timer) / static_cast< f32 >(slowStartTime);
    }

    targetSpeed *= startRatio * startRatio;
    const f32 inertia = decideInertia(targetSpeed);
    _278 = (_278 * inertia) + (targetSpeed * (1.0f - inertia));
#else  // SMGPC_RETAIL_SOURCE
    f32 targetSpeed = getTargetWalkSpeed();
    f32 startRatio = 1.0f;

    if (targetSpeed == 0.0f) {
        _404 = mActor->getConst().getTable()->mSlowStartTime;
    }

    if (_404 != 0) {
        const u16 slowStartTime = mActor->getConst().getTable()->mSlowStartTime;
        const u16 timer = _404;
        _404--;
        startRatio = static_cast< f32 >(slowStartTime - timer) / static_cast< f32 >(slowStartTime);
    }

    targetSpeed *= startRatio * startRatio;
    if (mMovementStates._F || isStatusActive(17)) {
        targetSpeed *= mActor->getConst().getTable()->mTornadoMultiply;
    }

    const bool wasSquatting = mMovementStates._A;
    if (wasSquatting && _1C._F) {
        bool press = false;
        if (_95C->getCode(_4C8) == 29) {
            press = true;
        } else {
            HitSensor* pSensor = reinterpret_cast< HitSensor* >(_730);
            if (MR::isSensorPressObj(pSensor)) {
                TVec3f horizontal;
                if (MR::vecKillElement(_184, *getGravityVec(), &horizontal) < -0.5f) {
                    press = true;
                } else {
                    TVec3f currentTrans;
                    TVec3f previousTrans;
                    CollisionParts* pParts = pSensor->mHost->mCollisionParts;
                    MR::extractMtxTrans(pParts->mBaseMatrix.toMtxPtr(), &currentTrans);
                    MR::extractMtxTrans(pParts->mPrevBaseMatrix.toMtxPtr(), &previousTrans);

                    const TVec3f move = currentTrans - previousTrans;
                    if (MR::vecKillElement(move, *getGravityVec(), &horizontal) < -0.5f) {
                        press = true;
                    }
                }
            }
        }

        if (mMovementStates._1) {
            if (strstr(getGroundPolygon()->mSensor->mHost->mName, "TriPod") != nullptr ||
                strstr(getGroundPolygon()->mSensor->mHost->mName, "Tripod") != nullptr) {
                press = false;
            }
        }

        if (mMovementStates._1) {
            if (reinterpret_cast< HitSensor* >(_730) == getGroundPolygon()->mSensor) {
                press = false;
            }
        }

        if (_730 != 0 && press) {
            mActor->_3B4 = _368;
            mActor->setPress(0, 0);
        }
    } else {
        mMovementStates._A = false;
        if (_436 == 0 && _434 == 0 && checkSquat(false) && _735 <= 32 && !isStatusActive(31)) {
            if (!checkLockOnHoming()) {
                mMovementStates._A = true;
            }

            if (!wasSquatting && mMovementStates._A && (mMovementStates._8 || mMovementStates._32)) {
                _71C = 0;
                _278 = 0.0f;
            }
        }

        if (_1C._F && !mMovementStates._A && isAnimationRun("しゃがみ終了")) {
            mMovementStates._A = true;
        }

        if (!mMovementStates._A && wasSquatting) {
            mMovementStates._A = true;
            cancelSquatMode();
            _71E = 0;
        }
    }

    if (_3D0 != 0) {
        targetSpeed = 0.0f;
    }
    if (mMovementStates._10) {
        targetSpeed = 0.0f;
    }

    f32 inertia = decideInertia(targetSpeed);
    if (!mMovementStates._A && getPlayerMode() == 1) {
        if (_278 >= 0.9999f) {
            targetSpeed *= mActor->getConst().getTable()->mDashMultiply;
            if (targetSpeed > _278) {
                inertia = 0.99f;
            }

            if (getPlayer()->_278 >= 1.5f) {
                getAnimator()->mXanimePlayer->changeTrackAnimation(2, "メタルダッシュ");
            }
        } else {
            getAnimator()->stopWaitAnimation();
        }
    }

    _278 = (_278 * inertia) + (targetSpeed * static_cast< f32 >(256 - _735) * (1.0f / 256.0f) * (1.0f - inertia));
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::decideOnIceAnimation() {
    if (_71C == 0) {
        if (_278 > 0.2f && !isAnimationRun("氷上慣性走行")) {
            changeAnimationWithAttr("氷上慣性走行", 1);
            _734 = 1 - _734;
        }
    } else {
        decideWalkAnimation();
        if (_278 > 0.7f) {
            if (_734 != 0) {
                getAnimator()->mXanimePlayer->changeTrackAnimation(2, "氷上力行右");
            } else {
                getAnimator()->mXanimePlayer->changeTrackAnimation(2, "氷上力行左");
            }
        }
    }

    if (_71C != 0 || _278 <= 0.2f) {
        stopAnimation("氷上慣性走行", static_cast< const char* >(nullptr));
    }
}

void Mario::updateOnSand() {
    if (mMovementStates._1F) {
        return;
    }

    if (mMovementStates._1) {
        if (_960 == 27 || _960 == 28) {
            if (strcmp(MR::getSoundCodeString(_45C), "Sand") == 0 && _735 < 64) {
                _735++;
            }
        } else if (isCurrentFloorSink()) {
            if (_735 == 255) {
                mActor->forceGameOverSink();
                return;
            }

            _735++;
            if (_960 == 25 || _960 == 31) {
                if (_960 == 31) {
                    if (_735 == 1) {
                        playSound("声沼沈み", -1);
                    }
                    playSound("沼強制沈み", -1);
                } else {
                    if (_735 == 1) {
                        playSound("声砂沈み", -1);
                    }
                    playSound("砂強制沈み", -1);
                }

                stopWalk();
                _735 = static_cast< u8 >(MR::clamp(static_cast< s32 >(_735) + 3, 0, 255));

                if (getAirGravityVec().dot(_368) > -0.99f) {
                    TVec3f horizontal;
                    MR::vecKillElement(_368, getAirGravityVec(), &horizontal);

                    TVec3f side;
                    side.cross(horizontal, _368);
                    MR::normalize(&side);
                    horizontal.cross(_368, side);
                    addVelocity(horizontal * 6.0f);
                }
            } else {
                playSound("砂沈み", -1);
            }

            if (!isAnimationRun(nullptr)) {
                getAnimator()->mXanimePlayer->changeTrackAnimation(1, "埋まり歩行");
            }
        } else {
            if (_735 != 0 && !isAnimationRun(nullptr)) {
                getAnimator()->mXanimePlayer->changeTrackAnimation(1, "歩行");
            }
            _735 = 0;
        }
    }

    if (mMovementStates.jumping || isStatusActive(6)) {
        _735 = 0;
    }
}

void Mario::updateOnPoison() {
    if (mMovementStates._1) {
        if (checkCurrentFloorCodeSevere(18)) {
            if (_748 == 0) {
                mActor->decLife(0);
                playSound("毒沼ダメージ", -1);
                playSound("ダメージ", -1);
                playSound("声小ダメージ", -1);
                if (mActor->mHealth == 0) {
                    mActor->forceGameOver();
                }
                startCamVib(0);
                mActor->_BC4 = 1;
            }

            if (_748 < 255) {
                _748++;
            } else {
                _748 = 0;
            }
        } else {
            _748 = 0;
        }
    } else if (mMovementStates.jumping && _3BC > 10) {
        _748 = 0;
    }

    if (isStatusActive(6)) {
        _748 = 0;
    }
}

void Mario::updateOnWater() {
    if (mMovementStates._1) {
        const s32 floorCode = _960;
        if (floorCode < 23) {
            if (floorCode >= 20) {
                touchWater();
                _738 = 20.0f;
                _73C = _368;

                switch (_962) {
                case 20:
                    _738 += 20.0f;
                case 21:
                    _738 += 20.0f;
                case 22:
                    mDrawStates.mIsUnderwater = true;
                    break;
                }
            }
        }

        if (_960 == 23 && _962 == 23) {
            touchWater();
            mDrawStates._13 = true;
            _738 = 3.0f;
            _73C = _368;
        }
    }

    const s32 previousFloorCode = _962;
    if (previousFloorCode < 24) {
        if (previousFloorCode >= 20) {
            mDrawStates._1D = true;
        }
    }
}
