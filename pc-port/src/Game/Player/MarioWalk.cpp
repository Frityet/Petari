#include "Game/Enemy/KarikariDirector.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioMapCode.hpp"
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include <cstring>
#include <stdexcept>

extern u8 lbl_806B6288;
#else  // SMGPC_RETAIL_SOURCE
#include "Game/Player/MarioModule.hpp"
#include "Game/Player/MarioMove.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util.hpp"
#include <cstdio>
#endif  // SMGPC_PC_DIVERGENCE

namespace {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    f32 sSpeedTableA[] = {0.15f, 0.3f, 0.45f, 0.6f, 0.7f, 0.85f, 0.99f};
    f32 sSpeedTableB[] = {0.02f, 0.2f, 0.4f, 0.5f, 0.65f, 0.75f, 0.98f};
    f32 sWalkTargetTable[] = {0.0f, 0.15f, 0.25f, 0.4f, 0.5f, 0.6f, 0.8f, 1.0f};
    f32 sWeightTable[][4] = {
        {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.0f},   {0.75f, 0.25f, 0.0f, 0.0f}, {0.25f, 0.75f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.75f, 0.25f, 0.0f}, {0.0f, 0.25f, 0.75f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f},
    };
    f32 sFootStep[] = {1.2f, 1.5f, 1.3f, 0.0f};
    f32 sFootStepBeeWallWalk[] = {0.5f, 0.5f, 0.5f, 0.0f};
#else  // SMGPC_RETAIL_SOURCE
    static f32 sSpeedTableA[] = {0.15f, 0.3f, 0.45f, 0.6f, 0.7f, 0.85f, 0.99f};
    static f32 sSpeedTableB[] = {0.02f, 0.2f, 0.4f, 0.5f, 0.65f, 0.75f, 0.98f};
    static f32 sWalkTargetTable[] = {0.0f, 0.15f, 0.25f, 0.4f, 0.5f, 0.6f, 0.8f, 1.0f};
    static f32 sWeightTable[8][4] = {{0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f,  0.0f,  0.0f}, {0.75f, 0.25f, 0.0f,  0.0f}, {0.25f, 0.75f, 0.0f, 0.0f},
                                 {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.75f, 0.25f, 0.0f}, {0.0f,  0.25f, 0.75f, 0.0f}, {0.0f,  0.0f,  1.0f, 0.0f}};
    static f32 sFootStep[] = {1.2f, 1.5f, 1.3f, 0.0f};
    static f32 sFootStepBeeWallWalk[] = {0.5f, 0.5f, 0.5f, 0.0f};
#endif  // SMGPC_PC_DIVERGENCE
};  // namespace

void Mario::stopWalk() {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    mTargetWalkSpeedIndex = false;
#else  // SMGPC_RETAIL_SOURCE
    mTargetWalkSpeedIndex = 0;
#endif  // SMGPC_PC_DIVERGENCE
    mWalkSpeed = 0.0f;
    _71E = 0;
    _71F = 0;
    _3D4 = 0;
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#else  // SMGPC_RETAIL_SOURCE

#endif  // SMGPC_PC_DIVERGENCE
    getAnimator()->initWalkWeight();
    getAnimator()->resetTilt();
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#else  // SMGPC_RETAIL_SOURCE

#endif  // SMGPC_PC_DIVERGENCE
    mMovementStates._10 = false;
    mMovementStates._23 = false;
    cancelSquatMode();
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    _3D2 = 0;
    mMovementStates._20 = false;
#else  // SMGPC_RETAIL_SOURCE

    mMovementStates.turning = false;
    _3D2 = 0;
#endif  // SMGPC_PC_DIVERGENCE
    _8F0 = 0.0f;
    _3F4 = 0.0f;
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#else  // SMGPC_RETAIL_SOURCE

#endif  // SMGPC_PC_DIVERGENCE
    stopEffect("共通スリップ坂");
    stopAnimation("歩行制動ブレーキ", 1);
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    stopAnimation("ブレーキ", static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
    stopAnimation("ブレーキ");
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::cancelSquatMode() {
    if (!mMovementStates._A) {
        return;
    }

    calcDistToCeil(false);
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if ((_20_LOW_WORD & 0x200000) != 0 || mMovementStates._A) {
        _10_LOW_WORD |= 0x10000;
#else  // SMGPC_RETAIL_SOURCE

    if (_20._A || mMovementStates._A) {
        _10._F = true;
#endif  // SMGPC_PC_DIVERGENCE
    }

    mMovementStates._A = false;
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    _20_LOW_WORD &= ~0x200000;
    stopAnimation("しゃがみ基本", static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
    _20._A = false;
    stopAnimation("しゃがみ基本");
#endif  // SMGPC_PC_DIVERGENCE

    if (mMovementStates._1 || !mMovementStates.jumping) {
        if (!isSwimming()) {
            changeAnimation(nullptr, "基本");
        }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        if (!isAnimationRun("サマーソルト") && (_10_LOW_WORD & 0x10000) != 0) {
#else  // SMGPC_RETAIL_SOURCE
        if (!isAnimationRun("サマーソルト") && _10._F) {
#endif  // SMGPC_PC_DIVERGENCE
            if (mWalkSpeed > 0.1f) {
                changeAnimationUpperWeak("しゃがみ終了", nullptr);
            } else {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                changeAnimation("しゃがみ終了", static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
                changeAnimation("しゃがみ終了", (const char*)nullptr);
#endif  // SMGPC_PC_DIVERGENCE
            }
        }
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    mMovementStates._19 = false;
#else  // SMGPC_RETAIL_SOURCE
    mMovementStates._26 = false;
#endif  // SMGPC_PC_DIVERGENCE
}

f32 Mario::getTargetWalkSpeed() const {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (mMovementStates._A || mSinkTimer != 0 || _2D0 != 0.0f || _434 != 0) {
        throw std::logic_error("modified Mario target speed is unavailable in the PC walk slice");
    }
    return sWalkTargetTable[mTargetWalkSpeedIndex];
#else  // SMGPC_RETAIL_SOURCE
    if (mMovementStates._A) {
        return 0.0f;
    }

    f32 targetWalkSpeed = ::sWalkTargetTable[mTargetWalkSpeedIndex];

    if (mSinkTimer != 0) {
        if (checkCurrentFloorCodeSevere(25)) {
            targetWalkSpeed = 0.0f;
        } else if (checkCurrentFloorCodeSevere(31)) {
            targetWalkSpeed = 0.0f;
        } else {
            targetWalkSpeed *= 1.0f - static_cast< f32 >(mSinkTimer) / 256.0f;
        }
    }

    targetWalkSpeed *= 1.0f - _2D0;

    if (_434 != 0) {
        targetWalkSpeed *= mActor->getConst().getTable()->mItemDashRatio;
    }
    return targetWalkSpeed;
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::decideSquatWalkAnimation() {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    f32 waitWeight[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    f32 walkWeight[4] = {0.0f, 1.0f, 0.0f, 0.0f};
#else  // SMGPC_RETAIL_SOURCE
    const f32 walkWeights1[] = {1.0f, 0.0f, 0.0f, 0.0f};
    const f32 walkWeights2[] = {0.0f, 1.0f, 0.0f, 0.0f};
#endif  // SMGPC_PC_DIVERGENCE
    _3F4 = 0.0f;

    if (!mMovementStates._A) {
        if (mWalkSpeed > 0.1f) {
            stopAnimation("しゃがみ", "基本");
            changeAnimationUpperWeak("しゃがみ終了", nullptr);
        } else {
            changeAnimation("しゃがみ終了", "基本");
        }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        mMovementStates._19 = false;
#else  // SMGPC_RETAIL_SOURCE
        mMovementStates._26 = false;
#endif  // SMGPC_PC_DIVERGENCE
        return;
    }

    if (isAnimationRun("壁押し", 0)) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
        stopAnimation(nullptr);
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if ((_20_LOW_WORD & 0x200000) == 0 || !isAnimationRun("しゃがみ基本")) {
#else  // SMGPC_RETAIL_SOURCE
    if (!_20._A || !isAnimationRun("しゃがみ基本")) {
#endif  // SMGPC_PC_DIVERGENCE
        stopAnimation("歩行制動ブレーキ", 1);
        changeAnimation(nullptr, "しゃがみ基本");
        stopAnimation("飛び込み準備", 4);
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        getAnimator()->setWalkWeight(waitWeight);
        mMovementStates._19 = false;

        if (!mMovementStates.jumping && !isAnimationRun("サマーソルト")) {
            playSound("声しゃがむ", -1);
#else  // SMGPC_RETAIL_SOURCE
        getAnimator()->setWalkWeight(&walkWeights1[0]);
        mMovementStates._26 = false;
        if (!mMovementStates.jumping && !isAnimationRun("サマーソルト")) {
            playSound("声しゃがむ");
#endif  // SMGPC_PC_DIVERGENCE
        }
    }

    if (mWalkSpeed < mActor->getConst().getTable()->mSpeedSquatWalkLower) {
        if (isStickOn()) {
            mWalkSpeed = mActor->getConst().getTable()->mSquatWalkMinSpeed;
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            getAnimator()->setWalkWeight(walkWeight);
            mTargetWalkSpeedIndex = 1;
            mMovementStates._19 = true;
        } else {
            getAnimator()->setWalkWeight(waitWeight);
            mTargetWalkSpeedIndex = 0;
            mMovementStates._19 = false;
#else  // SMGPC_RETAIL_SOURCE
            getAnimator()->setWalkWeight(&walkWeights2[0]);
            mTargetWalkSpeedIndex = 1;
            mMovementStates._26 = true;
        } else {
            getAnimator()->setWalkWeight(&walkWeights1[0]);
            mTargetWalkSpeedIndex = 0;
            mMovementStates._26 = false;
#endif  // SMGPC_PC_DIVERGENCE
        }
    } else if (!isStickOn()) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        getAnimator()->setWalkWeight(waitWeight);
        mTargetWalkSpeedIndex = 0;
    } else if (mMovementStates._19) {
        const MarioConstTable* pTable = mActor->getConst().getTable();
        mWalkSpeed = pTable->mSquatWalkMinSpeed + (mStickPos.z * (pTable->mSquatWalkMaxSpeed - pTable->mSquatWalkMinSpeed));
        getAnimator()->setWalkWeight(walkWeight);
#else  // SMGPC_RETAIL_SOURCE
        getAnimator()->setWalkWeight(&walkWeights1[0]);
        mTargetWalkSpeedIndex = 0;
    } else if (mMovementStates._26) {
        MarioConstTable* table = mActor->getConst().getTable();
        mWalkSpeed = (table->mSquatWalkMaxSpeed - table->mSquatWalkMinSpeed) * mStickPos.z + table->mSquatWalkMinSpeed;
        getAnimator()->setWalkWeight(&walkWeights2[0]);
#endif  // SMGPC_PC_DIVERGENCE
        mTargetWalkSpeedIndex = 1;
    }

    if (isAnimationRun("しゃがみ基本")) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        f32 animationSpeed = (60.0f / mActor->getConst().getTable()->mSquatWalkStep) * mWalkSpeed;
        if (!mMovementStates._19) {
            animationSpeed = 1.0f;
        }
        getAnimator()->mXanimePlayer->changeSpeed(animationSpeed);
#else  // SMGPC_RETAIL_SOURCE
        f32 animspeed = mActor->getConst().getTable()->mSquatWalkStep;
        animspeed = 60.0f / animspeed * mWalkSpeed;
        if (!mMovementStates._26) {
            animspeed = 1.0f;
        }
        getAnimator()->getXanimePlayer()->changeSpeed(animspeed);
#endif  // SMGPC_PC_DIVERGENCE
    }

    if (getFloorCode() == 32 && mTargetWalkSpeedIndex != 0) {
        _3F4 = 0.2f;
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    const f32 frame = getAnimator()->getFrame();
#else  // SMGPC_RETAIL_SOURCE
    f32 animFrame = getAnimator()->getFrame();

#endif  // SMGPC_PC_DIVERGENCE
    if (mTargetWalkSpeedIndex != 0) {
        if (mDrawStates.mIsUnderwater || mDrawStates._13) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            if (mPrevAnimFrame > frame) {
                playSound("水跳ね左足小", -1);
                playEffect("水はね左弱");
                const TVec3f effectPos = (mGroundPos - (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
#else  // SMGPC_RETAIL_SOURCE
            if (mPrevAnimFrame > animFrame) {
                playSound("水跳ね左足小");
                playEffect("水はね左弱");
                playEffectSRT("水波紋", 0.2f, _73C, (mGroundPos - mSideVec * 20.0f) + _368 * _738);
#endif  // SMGPC_PC_DIVERGENCE
            }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            if (mPrevAnimFrame < 30.0f && frame >= 30.0f) {
                playSound("水跳ね右足小", -1);
                playEffect("水はね右弱");
                const TVec3f effectPos = (mGroundPos + (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
#else  // SMGPC_RETAIL_SOURCE
            if (mPrevAnimFrame < 30.0f && animFrame >= 30.0f) {
                playSound("水跳ね右足小");
                playEffect("水はね右弱");
                playEffectSRT("水波紋", 0.2f, _73C, (mGroundPos + mSideVec * 20.0f) + _368 * _738);
#endif  // SMGPC_PC_DIVERGENCE
            }
        }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    } else if (mDrawStates.mIsUnderwater && (mActor->_37C & 0x3F) == 0) {
        const TVec3f randomSide = mSideVec * (MR::getRandom() - 0.5f);
        const TVec3f effectPos = mShadowPos + (randomSide * 20.0f) + (_368 * _738);
        playEffectSRT("水波紋", 0.2f, _73C, effectPos);
#else  // SMGPC_RETAIL_SOURCE
    } else if (mDrawStates.mIsUnderwater && mActor->_37C % 64 == 0) {
        playEffectSRT("水波紋", 0.2f, _73C, (mShadowPos + mSideVec * (MR::getRandom() - 0.5f) * 20.0f) + _368 * _738);
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    mPrevAnimFrame = frame;
#else  // SMGPC_RETAIL_SOURCE
    mPrevAnimFrame = animFrame;
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::decideWalkSpeed() {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    s32 lowerSpeed = 0;
    const u8 oldSpeed = mTargetWalkSpeedIndex;
    if (oldSpeed != 0 && mStickPos.z < sSpeedTableB[oldSpeed - 1]) {
        lowerSpeed = 1;
#else  // SMGPC_RETAIL_SOURCE
    bool canIndexDecrease = mTargetWalkSpeedIndex != 0 && mStickPos.z < ::sSpeedTableB[mTargetWalkSpeedIndex - 1];

    u32 i;
    for (i = 0; i < ARRAY_SIZE(::sSpeedTableA); i++) {
        if (mStickPos.z < ::sSpeedTableA[i]) {
            break;
        }
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    u32 speed = 0;
    for (u32 i = 0; i < 7; i++) {
        if (mStickPos.z < sSpeedTableA[i]) {
            break;
        }
        speed++;
#else  // SMGPC_RETAIL_SOURCE
    if (mTargetWalkSpeedIndex <= i || canIndexDecrease) {
        mTargetWalkSpeedIndex = i;
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (oldSpeed <= speed || lowerSpeed) {
        mTargetWalkSpeedIndex = speed;
#else  // SMGPC_RETAIL_SOURCE
    s32 clingNum = MR::getKarikariClingNum();
    if (clingNum != 0) {
        if (clingNum > 5) {
            clingNum = 5;
        }

        if (mTargetWalkSpeedIndex > 5 - clingNum) {
            mTargetWalkSpeedIndex = 5 - clingNum;
        }
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    return;
#else  // SMGPC_RETAIL_SOURCE
    if (mDrawStates.mIsUnderwater && mTargetWalkSpeedIndex > 6) {
        mTargetWalkSpeedIndex = 6;
    }

    if (mActor->mBeeWallWalk != 0 && mTargetWalkSpeedIndex > 4) {
        mTargetWalkSpeedIndex = 4;
    }

    if (_960 == 32) {
        if (mTargetWalkSpeedIndex > 3) {
            mTargetWalkSpeedIndex = 3;
        }

        if (mTargetWalkSpeedIndex != 0) {
            startPadVib(1);
        }
        if (mTargetWalkSpeedIndex > 2) {
            getAnimator()->getXanimePlayer()->_0C = 0.5f;
        } else {
            getAnimator()->getXanimePlayer()->_0C = 1.0f;
        }
    } else {
        f32 new0C = 0.1f + getAnimator()->getXanimePlayer()->_0C;

        if (new0C > 1.0f) {
            new0C = 1.0f;
        }
        getAnimator()->getXanimePlayer()->_0C = new0C;
    }
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::decideWalkAnimation() {
    if (mTargetWalkSpeedIndex == 0 && mWalkSpeed < 0.2f && isBlendWaitGround()) {
        getAnimator()->controlWaitAnimation();
    } else {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        getPlayer()->_10_LOW_WORD &= ~0x10000;
        if (getPlayer()->mTargetWalkSpeedIndex == 0 && mSwim->_1B2 && !isPlayerModeBee()) {
#else  // SMGPC_RETAIL_SOURCE
        getPlayer()->_10._F = false;

        if (getPlayer()->mTargetWalkSpeedIndex == 0 && mSwim->_1B2 && isPlayerModeBee()) {
#endif  // SMGPC_PC_DIVERGENCE
            changeAnimation("飛び込み準備", 4);
            return;
        }

        if (mSinkTimer == 0) {
            if (getPlayer()->mTargetWalkSpeedIndex != 0) {
                getAnimator()->stopWaitAnimation();
            }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            getAnimator()->setWalkWeight(sWeightTable[mTargetWalkSpeedIndex]);
        } else {
            f32 weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            if (mTargetWalkSpeedIndex != 0) {
                if (mTargetWalkSpeedIndex <= 3) {
                    f32 weight = 1.0f;
                    if (mTargetWalkSpeedIndex == 2) {
                        weight = 0.75f;
                    } else if (mTargetWalkSpeedIndex == 3) {
                        weight = 0.5f;
                    }
                    weights[0] = weight;
                    weights[1] = 1.0f - weight;
                } else {
                    f32 weight = 1.0f;
                    if (mTargetWalkSpeedIndex >= 5) {
                        weight = static_cast< f32 >(mSinkTimer) / 100.0f;
                    }
                    weight = MR::clamp(weight, 0.0f, 1.0f);
                    weights[1] = weight;
                    weights[2] = 1.0f - weight;
#else  // SMGPC_RETAIL_SOURCE
            getAnimator()->setWalkWeight(::sWeightTable[mTargetWalkSpeedIndex]);
        } else {
            f32 weights[] = {0, 0, 0, 0};
            if (mTargetWalkSpeedIndex != 0) {
                f32 f1 = 1.0f;
                if (mTargetWalkSpeedIndex <= 3) {
                    if (mTargetWalkSpeedIndex == 2) {
                        f1 = 0.75f;
                    }

                    if (mTargetWalkSpeedIndex == 3) {
                        f1 = 0.5f;
                    }

                    weights[0] = f1;
                    weights[1] = 1.0f - f1;
                } else {
                    if (mTargetWalkSpeedIndex >= 5) {
                        f1 = static_cast< f32 >(mSinkTimer) / 100.0f;
                    }

                    f1 = MR::clamp(f1, 0.0f, 1.0f);
                    weights[1] = f1;
                    weights[2] = 1.0f - f1;
#endif  // SMGPC_PC_DIVERGENCE
                }
            } else {
                weights[3] = 1.0f;
            }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            getAnimator()->setWalkWeight(weights);
#else  // SMGPC_RETAIL_SOURCE

            getAnimator()->setWalkWeight(&weights[0]);
#endif  // SMGPC_PC_DIVERGENCE
        }
    }

    stopAnimation("飛び込み準備", 4);

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    const f32 frame = getAnimator()->getFrame();
#else  // SMGPC_RETAIL_SOURCE
    f32 animFrame = getAnimator()->getFrame();
#endif  // SMGPC_PC_DIVERGENCE
    if (mTargetWalkSpeedIndex != 0) {
        if (mDrawStates.mIsUnderwater || mDrawStates._13) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            if (mPrevAnimFrame > frame) {
#else  // SMGPC_RETAIL_SOURCE
            if (mPrevAnimFrame > animFrame) {
#endif  // SMGPC_PC_DIVERGENCE
                if (mTargetWalkSpeedIndex >= 2) {
                    if (mTargetWalkSpeedIndex < 6) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                        playSound("水跳ね左足小", -1);
#else  // SMGPC_RETAIL_SOURCE
                        playSound("水跳ね左足小");
#endif  // SMGPC_PC_DIVERGENCE
                        playEffect("水はね左弱");
                    } else {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                        playSound("水跳ね左足", -1);
#else  // SMGPC_RETAIL_SOURCE
                        playSound("水跳ね左足");
#endif  // SMGPC_PC_DIVERGENCE
                        playEffect("水はね左");
                    }
                }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                const TVec3f effectPos = (mGroundPos - (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
            }

            if (mPrevAnimFrame < 30.0f && frame >= 30.0f) {
#else  // SMGPC_RETAIL_SOURCE
                playEffectSRT("水波紋", 0.2f, _73C, (mGroundPos - mSideVec * 20.0f) + _368 * _738);
            }
            if (mPrevAnimFrame < 30.0f && animFrame >= 30.0f) {
#endif  // SMGPC_PC_DIVERGENCE
                if (mTargetWalkSpeedIndex >= 2) {
                    if (mTargetWalkSpeedIndex < 6) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                        playSound("水跳ね右足小", -1);
#else  // SMGPC_RETAIL_SOURCE
                        playSound("水跳ね右足小");
#endif  // SMGPC_PC_DIVERGENCE
                        playEffect("水はね右弱");
                    } else {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                        playSound("水跳ね右足", -1);
#else  // SMGPC_RETAIL_SOURCE
                        playSound("水跳ね右足");
#endif  // SMGPC_PC_DIVERGENCE
                        playEffect("水はね右");
                    }
                }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                const TVec3f effectPos = (mGroundPos + (mSideVec * 20.0f)) + (_368 * _738);
                playEffectSRT("水波紋", 0.2f, _73C, effectPos);
#else  // SMGPC_RETAIL_SOURCE
                playEffectSRT("水波紋", 0.2f, _73C, (mGroundPos + mSideVec * 20.0f) + _368 * _738);
#endif  // SMGPC_PC_DIVERGENCE
            }
        }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    } else if (mDrawStates.mIsUnderwater && (mActor->_37C & 0x3F) == 0) {
        const TVec3f randomSide = mSideVec * (MR::getRandom() - 0.5f);
        const TVec3f effectPos = mShadowPos + (randomSide * 20.0f) + (_368 * _738);
        playEffectSRT("水波紋", 0.2f, _73C, effectPos);
    }
    mPrevAnimFrame = frame;

    const f32* pFootStep = mActor->mBeeWallWalk ? sFootStepBeeWallWalk : sFootStep;
    f32 stepLength = 0.0f;
    for (u32 i = 0; i < 4; i++) {
        stepLength += pFootStep[i] * sWeightTable[mTargetWalkSpeedIndex][i];
#else  // SMGPC_RETAIL_SOURCE
    } else if (mDrawStates.mIsUnderwater && mActor->_37C % 64 == 0) {
        playEffectSRT("水波紋", 0.2f, _73C, (mShadowPos + mSideVec * (MR::getRandom() - 0.5f) * 20.0f) + _368 * _738);
    }

    mPrevAnimFrame = animFrame;

    f32* footStep = ::sFootStep;
    f32 f3 = 0.0f;

    if (mActor->mBeeWallWalk != 0) {
        footStep = ::sFootStepBeeWallWalk;
    }

    for (int i = 0; i < ARRAY_SIZE(*::sWeightTable); i++) {
        f3 += footStep[i] * ::sWeightTable[mTargetWalkSpeedIndex][i];
#endif  // SMGPC_PC_DIVERGENCE
    }

    f32 animationSpeed;
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (stepLength == 0.0f) {
#else  // SMGPC_RETAIL_SOURCE

    if (f3 == 0.0f) {
#endif  // SMGPC_PC_DIVERGENCE
        animationSpeed = 0.33f;
    } else {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        animationSpeed = 0.5f * (60.0f * ((0.01f * (mWalkSpeed * mActor->getConst().getTable()->mWalkSpeed)) / stepLength));
#else  // SMGPC_RETAIL_SOURCE
        animationSpeed = 0.5f * (60.0f * ((0.01f * (mWalkSpeed * mActor->getConst().getTable()->mWalkSpeed)) / f3));
#endif  // SMGPC_PC_DIVERGENCE
    }

    if (_8F0 > 0.0f && !mDrawStates._4) {
        animationSpeed *= 1.0f + ((_8F0 / 10.0f) * mActor->getConst().getTable()->mSlopeAnimeRatio);
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#else  // SMGPC_RETAIL_SOURCE

#endif  // SMGPC_PC_DIVERGENCE
        if (_8F0 > 5.0f && animationSpeed > 4.0f) {
            animationSpeed *= mActor->getConst().getTable()->mSlopeSpinAnimeRatio;
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            changeAnimation("がんばり走り", static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
            changeAnimation("がんばり走り", (const char*)nullptr);
#endif  // SMGPC_PC_DIVERGENCE
            startBas("RunSlope", false, 0.0f, 0.0f);
        }
    } else if (_3FE != 0) {
        animationSpeed *= _8F4;
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    f32 speedDelta = getTargetWalkSpeed() - mWalkSpeed;
    if (mWalkSpeed > sWalkTargetTable[5]) {
        const f32 blend = (mWalkSpeed - sWalkTargetTable[5]) / (1.0f - sWalkTargetTable[5]);
        speedDelta = speedDelta * (1.0f - blend) + speedDelta * speedDelta * blend;
    } else if (mWalkSpeed < sWalkTargetTable[3]) {
        const f32 adjustedDelta = MR::sqrt(speedDelta);
        const f32 blend = (sWalkTargetTable[3] - mWalkSpeed) / sWalkTargetTable[3];
        speedDelta = speedDelta * blend + adjustedDelta * (1.0f - blend);
#else  // SMGPC_RETAIL_SOURCE
    f32 diffFromTargetSpeed = getTargetWalkSpeed() - mWalkSpeed;

    if (mWalkSpeed > ::sWalkTargetTable[5]) {
        f32 squared = diffFromTargetSpeed * diffFromTargetSpeed;
        f32 factor = (mWalkSpeed - ::sWalkTargetTable[5]) / (1.0f - ::sWalkTargetTable[5]);
        diffFromTargetSpeed = (diffFromTargetSpeed * (1.0f - factor)) + (squared * factor);
    } else if (mWalkSpeed < ::sWalkTargetTable[3]) {
        f32 sqrt = MR::fastSqrtf(diffFromTargetSpeed);
        f32 factor = (::sWalkTargetTable[3] - mWalkSpeed) / ::sWalkTargetTable[3];
        diffFromTargetSpeed = (diffFromTargetSpeed * factor) + (sqrt * (1.0f - factor));
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
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
    if (getFloorCode() == 32 && mTargetWalkSpeedIndex != 0) {
        speedBlend = 1.2f;
#else  // SMGPC_RETAIL_SOURCE
    f32 f4 = mActor->getConst().getTable()->mStartSpinAnimeRatio;
    f32 f5 = 1.0f + 4.0f * diffFromTargetSpeed;

    if (f5 > 2.0f) {
        f5 = 2.0f;
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    const f32 blend = 2.0f - speedBlend;
    const f32 minimumSpeed =
        startSpinAnimeRatio * (0.5f * (60.0f * ((0.01f * (getTargetWalkSpeed() * mActor->getConst().getTable()->mWalkSpeed)) / pFootStep[2])));
    if (animationSpeed < minimumSpeed) {
        animationSpeed = animationSpeed * blend + minimumSpeed * (1.0f - blend);
    }
#else  // SMGPC_RETAIL_SOURCE
    if (f5 < 1.0f) {
        f5 = 1.0f;
    }

    if (getTargetWalkSpeed() < ::sWalkTargetTable[6]) {
        f5 = 1.0f;
    }

    if (getFloorCode() == 32 && mTargetWalkSpeedIndex != 0) {
        f5 = 1.2f;
    }

    f32 f6 = f4 * (0.5f * (60.0f * ((getTargetWalkSpeed() * mActor->getConst().getTable()->mWalkSpeed * 0.01f) / footStep[2])));
    f32 factor2 = 2.0f - f5;

    if (animationSpeed < f6) {
        animationSpeed = animationSpeed * factor2 + f6 * (1.0f - factor2);
    }

#endif  // SMGPC_PC_DIVERGENCE
    if (mSinkTimer != 0 && mTargetWalkSpeedIndex != 0 && animationSpeed < 1.0f) {
        animationSpeed = 1.0f;
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    _3F4 = speedBlend - 1.0f;
    getAnimator()->mXanimePlayer->changeSpeed(animationSpeed * (1.0f - _2D0));

    if (!mActor->_EA4 && mTargetWalkSpeedIndex == 0 && mActor->mHealth == 1 && mActor->mMaxHealth > 2 && (_970 == nullptr || strcmp(_970, "DamageWait") != 0)) {
        getAnimator()->mXanimePlayer->changeTrackAnimation(3, "ダメージウエイト");
#else  // SMGPC_RETAIL_SOURCE
    animationSpeed *= (1.0f - _2D0);
    _3F4 = f5 - 1.0f;
    getAnimator()->getXanimePlayer()->changeSpeed(animationSpeed);

    if (!mActor->_EA4 && mTargetWalkSpeedIndex == 0 && mActor->mHealth == 1 && mActor->mMaxHealth > 2 &&
        (_970 == nullptr || strcmp(_970, "DamageWait"))) {
        getAnimator()->getXanimePlayer()->changeTrackAnimation(3, "ダメージウエイト");
#endif  // SMGPC_PC_DIVERGENCE
        startBas("DamageWait", false, 0.0f, 0.0f);
        mActor->setBlink("DamageWait");
    }

    checkWallPush();
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    f32 brakeSpeed = 0.9f;
    if (lbl_806B6288) {
        brakeSpeed = 0.3f;
    }
    if (mTargetWalkSpeedIndex > 5 && mWalkSpeed > brakeSpeed && !mDrawStates._4 && !mMovementStates._35) {
#else  // SMGPC_RETAIL_SOURCE

    f32 f7 = 0.9f;

    if (gIsLuigi) {
        f7 = 0.3f;
    }

    if (mTargetWalkSpeedIndex > 5 && mWalkSpeed > f7 && !mDrawStates._4 && !mMovementStates._35) {
#endif  // SMGPC_PC_DIVERGENCE
        _71E = mActor->getConst().getTable()->mBrakeFirstTimer;
    }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#else  // SMGPC_RETAIL_SOURCE

#endif  // SMGPC_PC_DIVERGENCE
    if (_71E != 0) {
        _71E--;
    }

    if (mTargetWalkSpeedIndex == 0) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        if (_71E != 0) {
            if (!isSlipPolygon(mGroundPolygon) && !mDrawStates._5) {
                doBrakingAnimation();
                _71F = mActor->getConst().getTable()->mBrakeSecondTimer;
            }
#else  // SMGPC_RETAIL_SOURCE
        if (_71E != 0 && !isSlipPolygon(mGroundPolygon) && !mDrawStates._5) {
            doBrakingAnimation();
            _71F = mActor->getConst().getTable()->mBrakeSecondTimer;
#endif  // SMGPC_PC_DIVERGENCE
        }
        _71E = 0;

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        const s32 clingNum = MR::getKarikariClingNum();
#else  // SMGPC_RETAIL_SOURCE
        s32 clingNum = MR::getKarikariClingNum();
#endif  // SMGPC_PC_DIVERGENCE
        if (clingNum >= 1) {
            changeAnimationUpper("カリカリ限界", nullptr);
            stopAnimation("歩行制動ブレーキ", 1);
        }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
#else  // SMGPC_RETAIL_SOURCE

#endif  // SMGPC_PC_DIVERGENCE
        if (clingNum < 1 && isAnimationRun("カリカリ限界")) {
            stopAnimationUpper("カリカリ限界", nullptr);
        }
    }
}

void Mario::doBrakingAnimation() {
    changeAnimation("歩行制動ブレーキ", 1);
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    getAnimator()->mXanimePlayer->_20->setAttribute(1);
    if (lbl_806B6288) {
        getAnimator()->mXanimePlayer->changeSpeed(0.5f);
#else  // SMGPC_RETAIL_SOURCE
    getAnimator()->getXanimePlayer()->_20->mAttribute = 1;
    if (gIsLuigi) {
        getAnimator()->getXanimePlayer()->changeSpeed(0.5f);
#endif  // SMGPC_PC_DIVERGENCE
    }
    playEffect("共通ブレーキ");
    _71F = 0;
}

void Mario::checkWallPush() {
    if (mTargetWalkSpeedIndex != 0 && (mMovementStates._8 || mMovementStates._32) && checkWallJumpCode()) {
        return;
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    const TVec3f wallDir = -getWallNorm();
    const f32 angle = MR::diffAngleAbsHorizontal(mFrontVec, wallDir, *getGravityVec());
    const MarioConst& marioConst = mActor->getConst();
    bool pushWall = false;
    bool isPushing = false;
    const f32 wallPushAngleRange = marioConst.getTable()->mWallPushAngleRange;

    if (mTargetWalkSpeedIndex != 0 && mMovementStates._8) {
        isPushing = true;
    }
    if (isPushing && angle < PI_180 * wallPushAngleRange) {
        pushWall = true;
#else  // SMGPC_RETAIL_SOURCE
    f32 angle = MR::diffAngleAbsHorizontal(mFrontVec, -getWallNorm(), *getGravityVec());
    bool sideStep = false;
    f32 wallPushAngleRange = mActor->getConst().getTable()->mWallPushAngleRange;

    bool checkAngle = mTargetWalkSpeedIndex != 0 && mMovementStates._8;

    if (checkAngle && angle < MR::toRadian(wallPushAngleRange)) {
        sideStep = true;
#endif  // SMGPC_PC_DIVERGENCE
    }

    if (mDrawStates._A) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        pushWall = false;
    }
    if (mDrawStates._C) {
        pushWall = false;
#else  // SMGPC_RETAIL_SOURCE
        sideStep = false;
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (calcAngleD(getWallNorm()) < marioConst.getTable()->mForceWallAngle) {
        pushWall = false;
#else  // SMGPC_RETAIL_SOURCE
    if (mDrawStates._C) {
        sideStep = false;
    }

    if (calcAngleD(getWallNorm()) < mActor->getConst().getTable()->mForceWallAngle) {
        sideStep = false;
#endif  // SMGPC_PC_DIVERGENCE
        if (mMovementStates._8 && mTargetWalkSpeedIndex != 0) {
            mTargetWalkSpeedIndex = 1;
            mWalkSpeed = 0.0f;
        }
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (!isAnimationRun("壁押し", 0) && pushWall) {
#else  // SMGPC_RETAIL_SOURCE
    if (!isAnimationRun("壁押し", 0) && sideStep) {
#endif  // SMGPC_PC_DIVERGENCE
        doSideStep();
    }
}

void Mario::updateBrakeAnimation() {
    if (_71F != 0) {
        if (!isAnimationRun("歩行制動ブレーキ", 1)) {
            _71F = 0;
        } else {
            _71F--;
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            if (!MR::isNearZero(mStickPos.z, 0.001f)) {
#else  // SMGPC_RETAIL_SOURCE
            if (!MR::isNearZero(mStickPos.z)) {
#endif  // SMGPC_PC_DIVERGENCE
                _71F = 0;
            }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE

            if (_71F == 0) {
                stopAnimation(nullptr, static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
            if (_71F == 0) {
                stopAnimation(nullptr);
#endif  // SMGPC_PC_DIVERGENCE
                stopWalk();
            }
        }
    } else if (isAnimationRun("歩行制動ブレーキ", 1) && (isAnimationTerminate(nullptr) || mTargetWalkSpeedIndex != 0)) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
        stopAnimation(nullptr);
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (!lbl_806B6288 || (!isAnimationRun("歩行制動ブレーキ", 1) && !isAnimationRun("ブレーキ"))) {
#else  // SMGPC_RETAIL_SOURCE
    if (!gIsLuigi) {
        return;
    }

    if (!isAnimationRun("歩行制動ブレーキ", 1) && !isAnimationRun("ブレーキ")) {
#endif  // SMGPC_PC_DIVERGENCE
        return;
    }

    if (mMovementStates._8 || mMovementStates._32) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
        stopAnimation(nullptr);
#endif  // SMGPC_PC_DIVERGENCE
        _71F = 0;
        _71E = 0;
        _3D0 = 0;
        _3D2 = 0;
    } else if (!MR::isDemoActive() && mMovementStates._1) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        playSound("ルイージ滑り", -1);
#else  // SMGPC_RETAIL_SOURCE
        playSound("ルイージ滑り");
#endif  // SMGPC_PC_DIVERGENCE
    }
}

void Mario::updateWalkSpeed() {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (mMovementStates._F || mMovementStates._A || mSinkTimer != 0 || getPlayerMode() != 0) {
        throw std::logic_error("special Mario walk-speed mode is unavailable in the PC walk slice");
#else  // SMGPC_RETAIL_SOURCE
    f32 targetWalkSpeed = getTargetWalkSpeed();
    f32 f2 = 1.0f;

    if (targetWalkSpeed == 0.0f) {
        _404 = mActor->getConst().getTable()->mSlowStartTime;
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
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
#else  // SMGPC_RETAIL_SOURCE
    if (_404 != 0) {
        f2 = mActor->getConst().getTable()->mSlowStartTime;
        f2 /= (mActor->getConst().getTable()->mSlowStartTime - _404);
        _404--;
#endif  // SMGPC_PC_DIVERGENCE
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    targetSpeed *= startRatio * startRatio;
    const f32 inertia = decideInertia(targetSpeed);
    mWalkSpeed = (mWalkSpeed * inertia) + (targetSpeed * (1.0f - inertia));
#else  // SMGPC_RETAIL_SOURCE
    targetWalkSpeed *= f2 * f2;
    if (mMovementStates._F || isStatusActive(17)) {
        targetWalkSpeed *= mActor->getConst().getTable()->mTornadoMultiply;
    }

    bool press = mMovementStates._A;
    if (mMovementStates._A && _1C._F) {
        press = false;
        if (_95C->getCode(_4C8) == 29) {
            press = true;
        } else if (MR::isSensorPressObj(_730)) {
            TVec3f result;
            if (MR::vecKillElement(_184, *getGravityVec(), &result) < -0.5f) {
                press = true;
            } else {
                TVec3f collisionTrans;
                TVec3f collisionPrevTrans;
                MR::extractMtxTrans(_730->mHost->mCollisionParts->mBaseMatrix, &collisionTrans);
                MR::extractMtxTrans(_730->mHost->mCollisionParts->mPrevBaseMatrix, &collisionPrevTrans);
                if (MR::vecKillElement(collisionTrans - collisionPrevTrans, *getGravityVec(), &result) < -0.5f) {
                    press = true;
                }
            }
        }

        if (mMovementStates._1 &&
            (strstr(getGroundPolygon()->mSensor->mHost->mName, "TriPod") || strstr(getGroundPolygon()->mSensor->mHost->mName, "Tripod"))) {
            press = false;
        }

        if (mMovementStates._1 && _730 == getGroundPolygon()->mSensor) {
            press = false;
        }

        if (_730 != nullptr && press) {
            mActor->_3B4 = _368;
            mActor->setPress(0, 0);
        }
    } else {
        mMovementStates._A = false;

        if (_436 == 0 && _434 == 0 && checkSquat(false) && mSinkTimer <= 32 && !isStatusActive(31)) {
            if (!checkLockOnHoming()) {
                mMovementStates._A = true;
            }
            if (!press && mMovementStates._A && (mMovementStates._8 || mMovementStates._32)) {
                mTargetWalkSpeedIndex = 0;
                mWalkSpeed = 0.0f;
            }
        }
        if (_1C._F && !mMovementStates._A && isAnimationRun("しゃがみ終了")) {
            mMovementStates._A = true;
        }

        if (!mMovementStates._A && press) {
            mMovementStates._A = true;
            cancelSquatMode();
            _71E = 0;
        }
    }

    if (_3D0 != 0) {
        targetWalkSpeed = 0.0f;
    }

    if (mMovementStates._10) {
        targetWalkSpeed = 0.0f;
    }

    f32 inertia = decideInertia(targetWalkSpeed);

    if (!mMovementStates._A && getPlayerMode() == 1) {
        if (mWalkSpeed >= 0.9999f) {
            targetWalkSpeed *= mActor->getConst().getTable()->mDashMultiply;
            if (targetWalkSpeed > mWalkSpeed) {
                inertia = 0.99f;
            }
            if (getPlayer()->mWalkSpeed >= 1.5f) {
                getAnimator()->getXanimePlayer()->changeTrackAnimation(2, "メタルダッシュ");
            }
        } else {
            getAnimator()->stopWaitAnimation();
        }
    }

    mWalkSpeed = (mWalkSpeed * inertia) + (targetWalkSpeed * static_cast< f32 >(256 - mSinkTimer) * (1.0f / 256.0f)) * (1.0f - inertia);
#endif  // SMGPC_PC_DIVERGENCE
}

void Mario::decideOnIceAnimation() {
    if (mTargetWalkSpeedIndex == 0) {
        if (mWalkSpeed > 0.2f && !isAnimationRun("氷上慣性走行")) {
            changeAnimationWithAttr("氷上慣性走行", 1);
            mIceAnimFoot = 1 - mIceAnimFoot;
        }
    } else {
        decideWalkAnimation();
        if (mWalkSpeed > 0.7f) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            if (mIceAnimFoot != 0) {
                getAnimator()->mXanimePlayer->changeTrackAnimation(2, "氷上力行右");
            } else {
                getAnimator()->mXanimePlayer->changeTrackAnimation(2, "氷上力行左");
#else  // SMGPC_RETAIL_SOURCE
            switch (mIceAnimFoot) {
            case 0:
                getAnimator()->getXanimePlayer()->changeTrackAnimation(2, "氷上力行左");
                break;
            default:
                getAnimator()->getXanimePlayer()->changeTrackAnimation(2, "氷上力行右");
#endif  // SMGPC_PC_DIVERGENCE
            }
        }
    }

    if (mTargetWalkSpeedIndex != 0 || mWalkSpeed <= 0.2f) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
        stopAnimation("氷上慣性走行", static_cast< const char* >(nullptr));
#else  // SMGPC_RETAIL_SOURCE
        stopAnimation("氷上慣性走行");
#endif  // SMGPC_PC_DIVERGENCE
    }
}

void Mario::updateOnSand() {
    if (mMovementStates._1F) {
        return;
    }

    if (mMovementStates._1) {
        if (_960 == 27 || _960 == 28) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            if (strcmp(MR::getSoundCodeString(_45C), "Sand") == 0 && mSinkTimer < 64) {
#else  // SMGPC_RETAIL_SOURCE
            if (!strcmp(MR::getSoundCodeString(_45C), "Sand") && mSinkTimer < 64) {
#endif  // SMGPC_PC_DIVERGENCE
                mSinkTimer++;
            }
        } else if (isCurrentFloorSink()) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
            if (mSinkTimer == 255) {
#else  // SMGPC_RETAIL_SOURCE
            if (mSinkTimer < 255) {
                mSinkTimer++;
                if (_960 == 25 || _960 == 31) {
                    if (_960 == 31) {
                        if (mSinkTimer == 1) {
                            playSound("声沼沈み");
                        }
                        playSound("沼強制沈み");
                    } else {
                        if (mSinkTimer == 1) {
                            playSound("声砂沈み");
                        }
                        playSound("砂強制沈み");
                    }
                    stopWalk();
                    mSinkTimer = MR::clamp(static_cast< s32 >(mSinkTimer) + 3, 0, 255);

                    if (getAirGravityVec().dot(_368) > -0.99f) {
                        TVec3f vec1;
                        MR::vecKillElement(_368, getAirGravityVec(), &vec1);
                        TVec3f vec2;
                        vec2.cross(vec1, _368);
                        MR::normalize(&vec2);
                        vec1.cross(_368, vec2);
                        addVelocity(vec1 * 6.0f);
                    }
                } else {
                    playSound("砂沈み");
                }
            } else {
#endif  // SMGPC_PC_DIVERGENCE
                mActor->forceGameOverSink();
                return;
            }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE

            mSinkTimer++;
            if (_960 == 25 || _960 == 31) {
                if (_960 == 31) {
                    if (mSinkTimer == 1) {
                        playSound("声沼沈み", -1);
                    }
                    playSound("沼強制沈み", -1);
                } else {
                    if (mSinkTimer == 1) {
                        playSound("声砂沈み", -1);
                    }
                    playSound("砂強制沈み", -1);
                }

                stopWalk();
                mSinkTimer = static_cast< u8 >(MR::clamp(static_cast< s32 >(mSinkTimer) + 3, 0, 255));

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
#else  // SMGPC_RETAIL_SOURCE
            if (!isAnimationRun(nullptr)) {
                getAnimator()->getXanimePlayer()->changeTrackAnimation(1, "埋まり歩行");
#endif  // SMGPC_PC_DIVERGENCE
            }
        } else {
            if (mSinkTimer != 0 && !isAnimationRun(nullptr)) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                getAnimator()->mXanimePlayer->changeTrackAnimation(1, "歩行");
#else  // SMGPC_RETAIL_SOURCE
                getAnimator()->getXanimePlayer()->changeTrackAnimation(1, "歩行");
#endif  // SMGPC_PC_DIVERGENCE
            }
            mSinkTimer = 0;
        }
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    if (mMovementStates.jumping || isStatusActive(6)) {
#else  // SMGPC_RETAIL_SOURCE
    if (mMovementStates.jumping) {
        mSinkTimer = 0;
    }

    if (isStatusActive(6)) {
#endif  // SMGPC_PC_DIVERGENCE
        mSinkTimer = 0;
    }
}

void Mario::updateOnPoison() {
    if (mMovementStates._1) {
        if (checkCurrentFloorCodeSevere(18)) {
            if (mPoisonTimer == 0) {
                mActor->decLife(0);
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
                playSound("毒沼ダメージ", -1);
                playSound("ダメージ", -1);
                playSound("声小ダメージ", -1);
#else  // SMGPC_RETAIL_SOURCE
                playSound("毒沼ダメージ");
                playSound("ダメージ");
                playSound("声小ダメージ");
#endif  // SMGPC_PC_DIVERGENCE
                if (mActor->mHealth == 0) {
                    mActor->forceGameOver();
                }
                startCamVib(0);
                mActor->_BC4 = 1;
            }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE

#else  // SMGPC_RETAIL_SOURCE
#endif  // SMGPC_PC_DIVERGENCE
            if (mPoisonTimer < 255) {
                mPoisonTimer++;
            } else {
                mPoisonTimer = 0;
            }
        } else {
            mPoisonTimer = 0;
        }
    } else if (mMovementStates.jumping && _3BC > 10) {
        mPoisonTimer = 0;
    }

    if (isStatusActive(6)) {
        mPoisonTimer = 0;
    }
}

void Mario::updateOnWater() {
    if (mMovementStates._1) {
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
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
#else  // SMGPC_RETAIL_SOURCE
        switch (_960) {
        case 20:
        case 21:
        case 22:
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
#endif  // SMGPC_PC_DIVERGENCE
            }
        }
#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE

#else  // SMGPC_RETAIL_SOURCE
#endif  // SMGPC_PC_DIVERGENCE
        if (_960 == 23 && _962 == 23) {
            touchWater();
            mDrawStates._13 = true;
            _738 = 3.0f;
            _73C = _368;
        }
    }

#if defined(TARGET_PC)  // SMGPC_PC_DIVERGENCE
    const s32 previousFloorCode = _962;
    if (previousFloorCode < 24) {
        if (previousFloorCode >= 20) {
            mDrawStates._1D = true;
        }
#else  // SMGPC_RETAIL_SOURCE
    switch (_962) {
    case 20:
    case 21:
    case 22:
    case 23:
        mDrawStates._1D = true;
#endif  // SMGPC_PC_DIVERGENCE
    }
}
