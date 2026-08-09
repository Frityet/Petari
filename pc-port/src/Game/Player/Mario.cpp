#include "Game/Player/Mario.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "Game/Enemy/KarikariDirector.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/MarioAbyssDamage.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioBlown.hpp"
#include "Game/Player/MarioBump.hpp"
#include "Game/Player/MarioClimb.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioCrush.hpp"
#include "Game/Player/MarioDamage.hpp"
#include "Game/Player/MarioDarkDamage.hpp"
#include "Game/Player/MarioFaint.hpp"
#include "Game/Player/MarioFireDamage.hpp"
#include "Game/Player/MarioFireDance.hpp"
#include "Game/Player/MarioFireRun.hpp"
#include "Game/Player/MarioFlip.hpp"
#include "Game/Player/MarioFlow.hpp"
#include "Game/Player/MarioFoo.hpp"
#include "Game/Player/MarioFpView.hpp"
#include "Game/Player/MarioFreeze.hpp"
#include "Game/Player/MarioFrontStep.hpp"
#include "Game/Player/MarioHang.hpp"
#include "Game/Player/MarioMagic.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Player/MarioMove.hpp"
#include "Game/Player/MarioParalyze.hpp"
#include "Game/Player/MarioRabbit.hpp"
#include "Game/Player/MarioRecovery.hpp"
#include "Game/Player/MarioSideStep.hpp"
#include "Game/Player/MarioSkate.hpp"
#include "Game/Player/MarioSlider.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Player/MarioStep.hpp"
#include "Game/Player/MarioStick.hpp"
#include "Game/Player/MarioStun.hpp"
#include "Game/Player/MarioSukekiyo.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Player/MarioTalk.hpp"
#include "Game/Player/MarioTeresa.hpp"
#include "Game/Player/MarioWait.hpp"
#include "Game/Player/MarioWall.hpp"
#include "Game/Player/MarioWarp.hpp"
#include "Game/Util.hpp"
#include "revolution/mtx.h"

void FORCE_OPERATOR() {
    TVec3f vec;
    vec.scale(1.0f);
}

Mario::Mario(MarioActor* actor) : MarioModule(actor) {
    initMember();
    mVelocity.zero();
    _13C.zero();
    _148.zero();
    _154.zero();
    mFrontVec.set(1.0f, 0.0f, 0.0f);
    _220 = mFrontVec;
    _43C = mFrontVec;
    _214.set(1.0f, 0.0f, 0.0f);
    _22C.set(1.0f, 0.0f, 0.0f);
    _3E4.set(1.0f, 0.0f, 0.0f);
    _2B4 = 0.0f;
    mSideVec.set(0.0f, 0.0f, 1.0f);
    _290 = mSideVec;
    mAirGravityVec.set(0.0f, -1.0f, 0.0f);
    _1E4.zero();
    bool nearZero = MR::isNearZero(mAirGravityVec);
    mHeadVec = -mAirGravityVec;
    _1FC = mHeadVec;
    _938 = mHeadVec;
    _944 = mFrontVec;
    _950 = mSideVec;
    _60D = 0;
    _71C = 0;
    _278 = 0.0f;
    _2D0 = 0.0f;
    mJumpVec.zero();
    _2E0.zero();
    _2EC.zero();
    _42C = 0;
    _2F8.zero();
    _328.zero();
    _334.zero();
    mVerticalSpeed = 0.0f;
    _430 = 0;
    _3BC = 0;
    _3BE = 0;
    _3C0 = 0;
    _3CE = 0;
    _3CC = 0;
    _3CA = 0;
    _3D0 = 0;
    _3D4 = 0;
    _424 = 0;
    _434 = 0;
    _436 = 0;
    _428 = 0;
    _3D2 = 0;
    _404 = 0;
    _406 = 0;
    _408 = 0;
    _40A = 0;
    _426 = 0;
    _418 = 0;
    _41A = 0;
    _774 = 0;
    _3C2 = 0;
    _3C4 = 0;
    _3C6 = 0;
    _3C8 = 0;
    _7D0 = 0x78;
    _524 = 0.0f;
    _528 = 0.0f;
    _52C = 0.0f;
    _530 = 0.0f;
    _534 = 0;
    _538 = 0.0f;
    _53C = 0.0f;
    _544 = 0;
    mYAngleOffset = 0.0f;
    _3F8 = 0;
    _3FA = 0;
    _3FC = 0;
    _3FE = 0;
    _400 = 0;
    _402 = 0;
    _27C = 0.0f;
    _734 = 0;
    _735 = 0;
    _748 = 0;
    _71E = 0;
    _414 = 0;
    mMorphResetTimer = 0;
    _41C = 0;
    _41E = 0;
    _410 = 0;
    _420 = 0;
    _43A = 0;
    _3F4 = 0.0f;
    _3F0 = 1.0f;
    _58.zero();
    _12C = 0;
    _76C = 0;
    _770 = 0.0f;
    _738 = 0.0f;
    _73C = TVec3f(0.0f, 0.0f, 0.0f);
    _368.set(0.0f, 1.0f, 0.0f);
    _374 = -_368;
    _380 = _368;
    _38C = -_368;
    _398.set(0.0f, 1.0f, 0.0f);
    _344.set(1.0f, 0.0f, 0.0f);
    _74C = 0.0f;
    _750 = 0;
    _754 = 0;
    _40C = 0;
    _40E = 0;
    _412 = 0;
    _558 = 0;
    _2A8 = -_368;
    _29C = _368;
    _548 = _128 = _124 = 0.0f;
    _54C = mHeadVec;
    PSMTXIdentity(_C4);
    PSMTXIdentity(_64);
    PSMTXIdentity(_94);
    clearSlope();
    _910.zero();
    _91C.zero();
    _928.zero();
    _934 = 0.8f;
    _3C = -0.515f;
    mMovementStates_LOW_WORD = 0;
    mMovementStates_HIGH_WORD = 0;
    _10_LOW_WORD = 0;
    _10_HIGH_WORD = 0;
    mDrawStates_WORD = 0;
    _1C_WORD = 0;
    _20 = 0;
    _24 = 0;
    _28 = 0;

    _458 = TriangleFilterDelegator< Mario >::allocateDelegator(this, &Mario::isIgnoreTriangle);

    _45C = new Triangle();
    _460 = new Triangle();
    mGroundPolygon = new Triangle();
    _468 = new Triangle();
    _46C = new Triangle();

    _4C8 = new Triangle();
    mFrontWallTriangle = new Triangle();
    mBackWallTriangle = new Triangle();
    mSideWallTriangle = new Triangle();
    _4D8 = new Triangle();
    _4DC = new Triangle();

    _470 = new Triangle();
    _474 = new Triangle();
    mTmpPolygon = new Triangle();

    _7E0 = new Triangle();

    _820 = new Triangle();

    _47C = new Triangle();
    _480 = new Triangle();
    _484 = new Triangle();

    for (u32 i = 0; i < ARRAY_SIZE(_57C); i++) {
        _57C[i] = new Triangle();
    }

    _578 = 0;

    _4E8.zero();
    _4F4.zero();

    _7D4.zero();
    _8A4.zero();

    _8C8 = new Triangle();

    _8E8 = new Triangle();

    _8CC[0] = new Triangle();
    _8CC[1] = new Triangle();

    _564 = -1;
    _568 = 0;
    _56C = 0;
    _570 = 0;

    for (u32 i = 0; i < ARRAY_SIZE(_A8C); i++) {
        _A8C[i].zero();
    }

    _4A4.zero();
    _4B0.zero();

    _25C.zero();
    _268.zero();

    _350.zero();
    _35C.zero();

    _184.zero();

    _1A8.zero();

    _190.zero();
    _19C.zero();

    _4E4 = 0.0f;
    _4E0 = 0.0f;

    _72C = 200.0f;

    _730 = 0;

    _8D8 = nullptr;
    _8D4 = nullptr;

    _A38 = 0;
    _A68 = 0;

    _5FC = 0;

    mWorldPadDir.zero();

    _38 = 0.0f;
    mStickPos.z = 0.0f;
    mStickPos.y = 0.0f;
    mStickPos.x = 0.0f;

    _3D8.zero();

    _244.zero();
    _238.zero();
    _250.zero();

    clear2DStick();

    _6AC = 0;
    _6AD = 0;
    _6B0.zero();
    _6BC.zero();

    _63C.set(1.0f, 0.0f, 0.0f);
    _648.set(0.0f, 1.0f, 0.0f);
    _654.set(0.0f, 0.0f, 1.0f);
    _6A0 = _654;

    initSound();

    _97C = 0;

    mFlow = new MarioFlow(mActor);
    mWall = new MarioWall(mActor);
    mDamage = new MarioDamage(mActor);
    mFaint = new MarioFaint(mActor);
    mBlown = new MarioBlown(mActor);
    mHang = new MarioHang(mActor);
    mSwim = new MarioSwim(mActor);
    mSlider = new MarioSlider(mActor);
    mFireDamage = new MarioFireDamage(mActor);
    mFireRun = new MarioFireRun(mActor);
    mFireDance = new MarioFireDance(mActor);
    mAbyssDamage = new MarioAbyssDamage(mActor);
    mDarkDamage = new MarioDarkDamage(mActor);
    mStep = new MarioStep(mActor);
    mBump = new MarioBump(mActor);
    mParalyze = new MarioParalyze(mActor);
    mStun = new MarioStun(mActor);
    mCrush = new MarioCrush(mActor);
    mFreeze = new MarioFreeze(mActor);
    mMagic = new MarioMagic(mActor);
    mFpView = new MarioFpView(mActor);
    mRecovery = new MarioRecovery(mActor);
    mFlip = new MarioFlip(mActor);
    mSideStep = new MarioSideStep(mActor);
    mFrontStep = new MarioFrontStep(mActor);
    mStick = new MarioStick(mActor);
    mRabbit = new MarioRabbit(mActor);
    mSukekiyo = new MarioSukekiyo(mActor);
    mBury = new MarioBury(mActor);
    mWait = new MarioWait(mActor);
    mClimb = new MarioClimb(mActor);
    mSkate = new MarioSkate(mActor);
    mFoo = new MarioFoo(mActor);
    mWarp = new MarioWarp(mActor);
    mTeresa = new MarioTeresa(mActor);
    mTalk = new MarioTalk(mActor);
    mMove = new MarioMove(mActor);

    initTask();
    _95C = new FloorCode();
    _960 = 0;
    _962 = 0;
    for (int i = 0; i < ARRAY_SIZE(_964); i++) {
        _964[i] = _960;
    }
    _96A = 0;
    _574 = 0;
    _898 = 0;
}

void Mario::updateAndClearStrideParameter() {
    _43C = mFrontVec;
    _16C = mVelocity;
    mPosition = mActor->mPosition;
    mVelocityAfter.zero();
    mVelocity.zero();
    _20 = mMovementStates_LOW_WORD;
    _24 = mMovementStates_HIGH_WORD;
    _28 = _10_LOW_WORD;
    _2C = _10_HIGH_WORD;
    mPrevDrawStates = mDrawStates;
    _34 = _1C_WORD;
    mDrawStates_WORD = 0;
    _1C_WORD = 0;
    mMovementStates._28 = 0;
    if (mMovementStates._1) {
        _10._1B = 0;
        mWall->_1C = 0;
    }
    if (_10._25) {
        _1C._1 = 1;
    }
    if (_10._26) {
        _1C._2 = 1;
    }
    if (_10._24) {
        _1C._0 = 1;
    }
    _10._25 = 0;
    _10._26 = 0;
}

void Mario::checkKeyLock() {
    if (!mMovementStates.jumping) {
        mMovementStates._22 = 0;
    }
    if (mActor->_37C < 0x4b) {
        mMovementStates._22 = 1;
    }
    if (mActor->isEnableNerveChange() == false) {
        mMovementStates._22 = 1;
    }
    if (isSwimming()) {
        mMovementStates._22 = 0;
    }
    if (MR::testSubPadButtonC(WPAD_CHAN0)) {
        mDrawStates._7 = 1;
    }
}

// Regswap
void Mario::updateTimers() {
    if (mMovementStates._1) {
        _3CE++;
    }
    if (_414 != 0) {
        _414--;
    }
    if (_434 != 0) {
        _434--;
    }
    MarioHang* hang = mHang;
    u16 hangTimer = hang->mHangTimer;
    if (hangTimer == 0) {
        return;
    } else {
        hang->mHangTimer = hangTimer + -1;
    }
}

/*
void Mario::setFrontVecKeepUp(const TVec3f &v)
{
    TVec3f stack_38(mFrontVec);
    if (!MR::isNearZero(v)) {
        setFrontVec(v);
    }
    TVec3f stack_2C = mHeadVec.cross(mFrontVec);

    if (MR::isNearZero(stack_2C)) {
        TVec3f stack_20;
        MR::vecBlendSphere(stack_38, v, &stack_20, 0.5f);
        stack_2C.cross(mHeadVec, stack_20);
        setFrontVec(stack_20);
        if (MR::isNearZero(stack_2C)) {
            setFrontVecKeepSide(v);
            return;
        }
    }
    MR::normalize(&stack_2C);
    mSideVec = stack_2C;
    MR::normalize(&mSideVec);
    setFrontVec(mSideVec.cross(mHeadVec));
    _22C = mFrontVec;
    _328 = mFrontVec % _328.length();
    _344 = mSideVec;
}
*/

void Mario::updateMorphResetTimer() {
    u16 timer = mMorphResetTimer;
    if (timer == 0) {
        return;
    }
    timer--;
    mMorphResetTimer = timer;
    if (timer != 0) {
        return;
    }
    mActor->setPlayerMode(0, true);
    return;
}

void Mario::doExtraServices() {
    TVec3f killed;
    TVec3f horizontal;
    MarioConst* marioConst = mActor->mConst;
    MarioConstTable* table = marioConst->getTable();

    if (MR::diffAngleAbs(_43C, mFrontVec) < table->mFrontAngleFixMargin) {
        setFrontVec(_43C);
    }

    if (!isPlayerModeBee() && !isPlayerModeTeresa() && mMovementStates.jumping && _3BC > 600 && !MR::isNearZero(*getGravityVec())) {
        f32 vertical = MR::vecKillElement(mJumpVec, *getGravityVec(), &killed);
        TVec3f gravity(*getGravityVec());
        gravity.scale(vertical);
        mJumpVec = gravity;
    }

    if (!isPlayerModeFoo() && !isPlayerModeTeresa() && mMovementStates.jumping && _3BC > 300) {
        const TVec3f* safetyTrans = getLastSafetyTrans(nullptr);
        TVec3f displacement(mPosition);
        displacement -= *safetyTrans;
        MR::isNearZero(mAirGravityVec);

        if (MR::vecKillElement(displacement, mAirGravityVec, &horizontal) > 28000.0f) {
            if (mVerticalSpeed > 2499.0f || mActor->isInZeroGravitySpot()) {
                mActor->forceKill(0);
            }
        }
    }

    if (getCurrentStatus() != MarioStatus_None || mActor->_3C0 || mActor->_EA4) {
        _10._C = 0;
    }
}

bool Mario::isEnableCheckGround() {
    return !isStatusActive(MarioStatus_Swim);
}

void Mario::setGroundNorm(const TVec3f& rVec) {
    if (MR::isNearZero(mAirGravityVec)) {
    }

    TVec3f negRVec = -rVec;

    if (MR::diffAngleAbs(negRVec, mAirGravityVec) > 1.4959966f) {
    } else {
        _368 = rVec;
        _374 = -rVec;
    }
}

void Mario::checkForceGrounding() {
    if ((mMovementStates._37 || _10._15) && mMovementStates._1) {
        if (MR::diffAngleAbs(_368, *_45C->getNormal(0)) < 0.7853982f) {
            mDrawStates._0 = 1;
        }
    }

    if (mMovementStates._8 || mMovementStates._19 || mMovementStates._1A || mMovementStates._15 || mMovementStates._23 ||
        mMovementStates.debugMode || !mMovementStates._1) {
        return;
    }

    if (!mDrawStates._0) {
        TVec3f negGround = -_368;
        f32 gravityDot = getGravityVec()->dot(negGround);

        TVec3f killAxis;
        if (gravityDot > 0.99f) {
            killAxis = *getGravityVec();
        } else {
            killAxis = -_368;
        }

        f32 vertical = MR::vecKillElement(mVelocity, killAxis, &mVelocity);
        TVec3f shadowDelta(mShadowPos);
        shadowDelta -= mPosition;
        gravityDot *= shadowDelta.dot(killAxis);

        if (MR::isNearZero(gravityDot)) {
            return;
        }
        if (__fabsf(gravityDot) >= 30.0f) {
            return;
        }

        if (mMovementStates._8) {
            vertical = gravityDot;
        }
        if (vertical > gravityDot) {
            vertical = gravityDot;
        }

        TVec3f adjustment(killAxis);
        adjustment.scale(vertical);
        mVelocity += adjustment;
        return;
    }

    TVec3f horizontalVelocity;
    f32 verticalVelocity = MR::vecKillElement(mVelocity, *getGravityVec(), &horizontalVelocity);
    f32 horizontalSpeed = horizontalVelocity.length();

    if (mVerticalSpeed != 0.0f && verticalVelocity > mVerticalSpeed) {
        verticalVelocity = mVerticalSpeed;
    }

    if (!mDrawStates._4) {
        if (__fabsf(verticalVelocity) > horizontalSpeed) {
            horizontalSpeed = 0.0f;
        } else {
            horizontalSpeed = MR::sqrt< f32 >((horizontalSpeed * horizontalSpeed) - (verticalVelocity * verticalVelocity));
        }
    }

    MR::normalizeOrZero(&horizontalVelocity);

    TVec3f gravityComponent(*getGravityVec());
    gravityComponent.scale(verticalVelocity);
    TVec3f horizontalComponent(horizontalVelocity);
    horizontalComponent.scale(horizontalSpeed);
    TVec3f newVelocity(horizontalComponent);
    newVelocity += gravityComponent;
    mVelocity = newVelocity;
}

void Mario::fixHeadFrontVecByGravity() {
    TVec3f oldUp(_368);

    if (MR::diffAngleAbs(mActor->mCamDirZ, mActor->_FA8) <= 0.7853982f && isStickOn()) {
        mDrawStates.mIsUnderwater = 1;
        _10._15 = 1;
        _60D = 1;
        _40E = 0;
    }

    if (_3D0 != 0 || _3D2 != 0) {
        mDrawStates._D = 0;
    }

    if (!_60D && mActor->_370) {
        mDrawStates._D = 0;
    }

    if (isStatusActive(MarioStatus_Bump) || isStatusActive(MarioStatus_Climb)) {
        _60D = 1;
    }

    TVec3f oldHead(mHeadVec);
    TVec3f upTarget = -*getGravityVec();
    MR::normalize(&upTarget);

    MarioConstTable* pTable = mActor->mConst->getTable();
    f32 rotateSpeed;
    if (upTarget.dot(mHeadVec) < 0.0f) {
        if (mActor->_334 != 0) {
            f32 blend = static_cast< f32 >(mActor->_334) / 15.0f;
            rotateSpeed = blend * pTable->mRotateHeadVecSpeedByGravityM + (1.0f - blend) * pTable->mRotateHeadVecSpeedByGravityL;
        } else {
            rotateSpeed = pTable->mRotateHeadVecSpeedByGravityL;
        }
    } else if (upTarget.dot(mHeadVec) < 0.99f) {
        rotateSpeed = pTable->mRotateHeadVecSpeedByGravityM;
    } else {
        rotateSpeed = pTable->mRotateHeadVecSpeedByGravityS;
    }

    if (_60D && _71C) {
        if ((mMovementStates._1 || !mMovementStates.jumping) && mDrawStates._D) {
            MR::isNearZero(mAirGravityVec);

            TVec3f airGravity(mAirGravityVec);
            if (MR::normalizeOrZero(&airGravity)) {
                return;
            }
            airGravity = -airGravity;

            TVec3f side;
            PSVECCrossProduct(&_290, &airGravity, &side);
            if (MR::normalizeOrZero(&side)) {
                return;
            }

            TVec3f correctedSide;
            PSVECCrossProduct(&airGravity, &side, &correctedSide);
            if (MR::normalizeOrZero(&correctedSide)) {
                return;
            }

            _290 = correctedSide;
            mSideVec = _290;
            MR::normalize(&mSideVec);

            if (_410 < 30) {
                _410++;
            }
        }

        TVec3f blendedHead;
        if (!MR::vecBlendSphere(mHeadVec, upTarget, &blendedHead, rotateSpeed)) {
            MR::vecRotAxis(mHeadVec, upTarget, mSideVec, &blendedHead, rotateSpeed);
        }

        mHeadVec = blendedHead;
        MR::normalize(&mHeadVec);
        fixFrontVecFromUpSide();
        _29C = oldUp;
        _290 = mSideVec;
        return;
    }

    TVec3f axis;
    PSVECCrossProduct(&mHeadVec, &upTarget, &axis);

    bool useFrontAxis = true;
    if (MR::isNearZero(axis)) {
        f32 frontDot = __fabsf(upTarget.dot(mFrontVec));
        f32 sideDot = __fabsf(upTarget.dot(mSideVec));
        if (sideDot < frontDot) {
            useFrontAxis = false;
        }
    } else {
        f32 sideDot = __fabsf(axis.dot(mSideVec));
        f32 frontDot = __fabsf(axis.dot(mFrontVec));
        if (frontDot < sideDot) {
            useFrontAxis = false;
        }
    }

    if (useFrontAxis) {
        TVec3f blendedHead;
        if (!MR::vecBlendSphere(mHeadVec, upTarget, &blendedHead, rotateSpeed)) {
            MR::vecRotAxis(mHeadVec, upTarget, mFrontVec, &blendedHead, rotateSpeed);
        }

        mHeadVec = blendedHead;
        MR::normalize(&mHeadVec);
        setFrontVecKeepUp(mFrontVec);
    } else {
        TVec3f blendedHead;
        if (!MR::vecBlendSphere(mHeadVec, upTarget, &blendedHead, rotateSpeed)) {
            MR::vecRotAxis(mHeadVec, upTarget, mSideVec, &blendedHead, rotateSpeed);
        }

        mHeadVec = blendedHead;
        MR::normalize(&mHeadVec);
        fixFrontVecFromUpSide();
    }

    pTable = mActor->mConst->getTable();
    if (MR::diffAngleAbs(_43C, mFrontVec) < pTable->mFrontAngleFixMargin) {
        setFrontVec(_43C);
    }

    if (MR::diffAngleAbs(mHeadVec, oldHead) < pTable->mHeadAngleFixMargin) {
        mHeadVec = oldHead;
        MR::normalize(&mHeadVec);
    }

    bool keepOldSide = _60D || !mActor->_370;
    if (keepOldSide && mDrawStates._D) {
        TVec3f diffA(mHeadVec);
        diffA -= _290;
        if (!MR::isNearZero(diffA)) {
            TVec3f diffB(mHeadVec);
            diffB += _290;
            if (!MR::isNearZero(diffB)) {
                mSideVec = _290;
                MR::normalize(&mSideVec);

                f32 angle = marioAcos(oldUp.dot(_29C));

                TVec3f rotationAxis;
                PSVECCrossProduct(&_29C, &oldUp, &rotationAxis);
                MR::normalizeOrZero(&rotationAxis);

                if (!MR::isNearZero(rotationAxis)) {
                    TVec3f sum(_29C);
                    sum += oldUp;
                    if (!MR::isNearZero(sum)) {
                        TVec3f diff(_29C);
                        diff -= oldUp;
                        if (!MR::isNearZero(diff)) {
                            TMtx34f rotation;
                            PSMTXRotAxisRad(rotation.toMtxPtr(), &rotationAxis, angle);
                            PSMTXMultVec(rotation.toMtxPtr(), &mSideVec, &mSideVec);
                            _29C = oldUp;
                            _290 = mSideVec;
                            _60D = 0;
                            fixFrontVecFromUpSide();
                            return;
                        }
                    }
                }

                _60D = 1;
                fixFrontVecFromUpSide();
                return;
            }
        }
    }

    TVec3f fallbackSide;
    PSVECCrossProduct(&oldUp, &_22C, &fallbackSide);
    MR::normalizeOrZero(&fallbackSide);
    if (!MR::isNearZero(fallbackSide)) {
        _290 = fallbackSide;
    } else {
        _290 = mSideVec;
    }

    _60D = 0;
}

// Nearly matches
void Mario::createMtxDir(MtxPtr mtx, const TVec3f& rFront, const TVec3f& rUp, const TVec3f& rSide) {
    TVec3f side;
    TVec3f front;
    TVec3f up;
    side = rSide;
    front = rFront;
    up = rUp;
    if (!isPlayerModeTeresa()) {
        if (mMovementStates._37 || _10._15) {
            f32 sideAngleDiff = MR::diffAngleAbs(side, _950);
            f32 frontAngleDiff = MR::diffAngleAbs(front, _944);
            f32 upAngleDiff = MR::diffAngleAbs(up, _938);
            if (sideAngleDiff < mActor->mConst->getTable()->mVibrationAbsorbAngleS) {
                side = _950;
            } else {
                _950 = side;
            }
            if (frontAngleDiff < mActor->mConst->getTable()->mVibrationAbsorbAngleF) {
                front = _944;
            } else {
                _944 = front;
            }
            if (upAngleDiff < mActor->mConst->getTable()->mVibrationAbsorbAngleH) {
                up = _938;
            } else {
                _938 = up;
            }
        }
    }
    mtx[0][0] = side.x;
    mtx[1][0] = side.y;
    mtx[2][0] = side.z;
    mtx[0][1] = up.x;
    mtx[1][1] = up.y;
    mtx[2][1] = up.z;
    mtx[0][2] = front.x;
    mtx[1][2] = front.y;
    mtx[2][2] = front.z;
}

bool Mario::isNonFixHeadVec() const {
    if (isStatusActive(MarioStatus_Swim)) {
        return true;
    }
    if (isStatusActive(MarioStatus_Wall)) {
        return true;
    }
    if (isStatusActive(MarioStatus_SideStep)) {
        return true;
    }
    if (isStatusActive(MarioStatus_Foo)) {
        return true;
    }
    if (isStatusActive(MarioStatus_Stick)) {
        return true;
    }
    if (mMovementStates._3E) {
        return true;
    }
    if (mActor->_EA5) {
        return true;
    }
    if (isStatusActive(MarioStatus_Talk)) {
        return false;
    }
    return mActor->_EA4;
}

void Mario::createDirectionMtx(MtxPtr mtx) {
    TVec3f stack_20;
    TVec3f stack_14;
    TVec3f stack_8;
    stack_14 = _1FC;
    stack_20 = mSideVec;
    stack_8 = mFrontVec;
    if (mDrawStates._11) {
        MR::normalize(&stack_14);
        PSVECCrossProduct(&stack_20, &stack_14, &stack_8);
        MR::normalizeOrZero(&stack_8);
        if (MR::isNearZero(stack_8)) {
            stack_8 = mFrontVec;
        }
        PSVECCrossProduct(&stack_14, &stack_8, &stack_20);
        MR::normalizeOrZero(&stack_20);
        if (MR::isNearZero(stack_20)) {
            stack_20 = mSideVec;
        }
    } else {
        MR::normalizeOrZero(&stack_14);
        PSVECCrossProduct(&stack_14, &mFrontVec, &stack_20);
        MR::normalizeOrZero(&stack_20);
        if (MR::isNearZero(stack_20)) {
            stack_20 = mSideVec;
        }
        PSVECCrossProduct(&stack_20, &stack_14, &stack_8);
        MR::normalizeOrZero(&stack_8);
        if (MR::isNearZero(stack_8)) {
            stack_8 = mFrontVec;
        }
    }
    createMtxDir(mtx, stack_8, stack_14, stack_20);
    MR::setMtxTrans(mtx, 0.0f, 0.0f, 0.0f);
}

bool Mario::createCorrectionMtx(MtxPtr mtx, TVec3f* pOut) {
    pOut->zero();

    if (isNonFixHeadVec()) {
        PSMTXIdentity(mtx);
        return false;
    }

    if (MR::isNearZero(_74C) && _754 == 0) {
        PSMTXIdentity(mtx);
        _750 = 0;
        _74C = 0.0f;
        return true;
    }

    if (_750 == 0 && _754 == 0) {
        TMtx34f rotMtx;
        TVec3f frontVec;
        PSMTXRotAxisRad(rotMtx.toMtxPtr(), &mHeadVec, _74C);
        PSMTXMultVec(rotMtx.toMtxPtr(), &mFrontVec, &frontVec);
        setFrontVecKeepUp(frontVec);
        _74C = 0.0f;
    }

    TVec3f yAxis(0.0f, 1.0f, 0.0f);
    if (mMovementStates._37 && !isStatusActive(MarioStatus_Talk)) {
        bool flip = false;
        f32 frontDot = mFrontVec.dot(getCamDirX());
        if (_1FC.dot(getCamDirY()) < 0.0f) {
            frontDot = -frontDot;
        }

        if (frontDot < 0.0f) {
            if (_74C < 0.0f) {
                flip = true;
            }
        } else if (frontDot > 0.0f && _74C > 0.0f) {
            flip = true;
        }

        PSMTXRotAxisRad(mtx, &yAxis, flip ? -_74C : _74C);
    } else {
        PSMTXRotAxisRad(mtx, &yAxis, _74C);
    }

    if (_754 != 0) {
        f32 step = (3.1415927f - _74C) / static_cast< f32 >(_754);
        _754--;
        _750 = 0;
        _74C += step;
    }

    if (_750 != 0) {
        _74C = _74C * static_cast< f32 >(_750 - 1) / static_cast< f32 >(_750);
        _750--;
        if (_750 == 0) {
            setFrontVecKeepUp(mFrontVec);
        }
    }

    return true;
}

void Mario::slopeTiltHead(TVec3f* pVec) {
    if (mMovementStates._1) {
        if (_8F0 > 0.0f && !mMovementStates._23) {
            f32 blendFactor = _8F0 / 5.0f;
            if (blendFactor > 1.0f) {
                blendFactor = 1.0f;
            }
            MR::vecBlend(_368, *pVec, pVec, blendFactor);
            MR::normalize(pVec);
        } else if (mDrawStates._4) {
            MR::vecBlend(_368, *pVec, pVec, 0.2);
            MR::normalize(pVec);
        }
    }
}

void Mario::fixFrontVecByGravity() {
    if (isNonFixHeadVec()) {
        return;
    }

    if (MR::isNearZero(mAirGravityVec)) {
    }

    TVec3f up = -mAirGravityVec;
    MR::normalize(&up);
    if (_10._13) {
        TVec3f side(mPosition);
        side -= _6F4;
        MR::vecKillElement(side, _700, &side);
        MR::normalizeOrZero(&side);
        if (_344.dot(side) >= 0.0f) {
            _344 = side;
        } else {
            _344 = -side;
        }
        if (mSideVec.dot(_344) >= 0.0f) {
            mSideVec = _344;
            MR::normalize(&mSideVec);
        } else {
            // TVec3f stack_14 = -_344;
            mSideVec = -_344;
            MR::normalize(&mSideVec);
        }
    } else if (_1C._2) {
        TVec3f side2 = up.cross(mFrontVec);
        MR::normalizeOrZero(&side2);
        if (MR::isNearZero(side2) == false) {
            _344 = side2;
        }
    }
    TVec3f front = _344.cross(up);
    if (MR::normalizeOrZero(&front) == nullptr) {
        setFrontVec(front);
        _22C = mFrontVec;
        f32 _328mag = _328.length();
        _328 = mFrontVec * _328mag;
    }
}

void Mario::fixFrontVecFromUpSide() {
    TVec3f frontVec = mSideVec.cross(mHeadVec);
    MR::normalizeOrZero(&frontVec);
    if (MR::isNearZero(frontVec) == false) {
        setFrontVec(frontVec);
        _22C = mFrontVec;
        f32 scaleFactor = _328.length();
        _328 = mFrontVec * scaleFactor;
    }
}

void Mario::fixSideVecFromFrontUp() {
    TVec3f side = mHeadVec.cross(mFrontVec);
    MR::normalizeOrZero(&side);
    if (MR::isNearZero(side) == false) {
        mSideVec = side;
    }
}

void Mario::setSideVec(const TVec3f& rSide) {
    mSideVec = rSide;
    MR::normalize(&mSideVec);
}

void Mario::setHeadVec(const TVec3f& rHead) {
    mHeadVec = rHead;
    MR::normalize(&mHeadVec);
}

void Mario::setFrontVec(const TVec3f& rFront) {
    TVec3f plusMinus6A0;
    TVec3f frontWithout6A0;
    if (mMovementStates._37 && !isStatusActive(MarioStatus_Talk)) {
        if (getCamDirZ().dot(_6A0) > 0.0f) {
            plusMinus6A0 = _6A0;
        } else {
            TVec3f neg6A0 = -_6A0;
            plusMinus6A0 = neg6A0;
        }
        if (plusMinus6A0.dot(rFront) > 0.0f) {
            // maybe this reflects rFront around _6A0?? (very unsure)
            f32 f1 = MR::vecKillElement(rFront, plusMinus6A0, &frontWithout6A0);
            frontWithout6A0 -= plusMinus6A0 * f1;
            MR::normalizeOrZero(&frontWithout6A0);
            mFrontVec = frontWithout6A0;
        } else {
            mFrontVec = rFront;
        }
    } else {
        mFrontVec = rFront;
    }
    MR::normalize(&mFrontVec);
}

void Mario::setFrontVecKeepUp(const TVec3f& rFront, f32 blendFactor) {
    TVec3f blendedFront;
    if (MR::isNearZero(rFront) == false) {
        TVec3f normalizedFront(rFront);
        MR::normalize(&normalizedFront);
        s32 blend = 0;
        f32 angleDiff = MR::diffAngleAbs(mFrontVec, normalizedFront);
        if (angleDiff < HALF_PI) {
            blend = MR::vecBlendSphere(mFrontVec, normalizedFront, &blendedFront, blendFactor);
        }
        if (blend == 0) {
            MR::vecRotAxis(mFrontVec, normalizedFront, mHeadVec, &blendedFront, angleDiff * blendFactor);
        }
        setFrontVecKeepUp(blendedFront);
    }
}

void Mario::setFrontVecKeepUpAngle(const TVec3f& frontVec, f32 angle) {
    if (MR::isNearZero(frontVec) == false) {
        f32 angleDiff = MR::diffAngleAbs(mFrontVec, frontVec);
        if (angleDiff != 0.0f) {
            if (angleDiff < angle) {
                setFrontVecKeepUp(frontVec);
            } else {
                setFrontVecKeepUp(frontVec, angle / angleDiff);
            }
        }
    }
}

void Mario::setFrontVecKeepUp(const TVec3f& rFront, u32 _750_set) {
    Mtx rotMtx;
    TVec3f rotFrontVec;
    TVec3f rotVec2;
    PSMTXRotAxisRad(rotMtx, &mHeadVec, _74C);
    PSMTXMultVec(rotMtx, &mFrontVec, &rotFrontVec);
    f32 angleDiff = MR::diffAngleAbs(rFront, rotFrontVec);
    if (angleDiff > 0.024543693f) {
        PSMTXRotAxisRad(rotMtx, &mHeadVec, PI_180);
        PSMTXMultVec(rotMtx, &rotFrontVec, &rotVec2);
        f32 angleDiff2 = MR::diffAngleAbs(rFront, rotVec2);
        if (angleDiff2 < angleDiff) {
            _74C = -angleDiff;
        } else {
            _74C = angleDiff;
        }
    }
    setFrontVecKeepUp(rFront);
    if (_750_set != 0) {
        _750 = _750_set;
    }
}

void Mario::setFrontVecKeepUp(const TVec3f& rFront) {
    TVec3f front = mFrontVec;
    TVec3f side;
    TVec3f blendedFront;
    if (MR::isNearZero(rFront) == false) {
        setFrontVec(rFront);
    }
    side.cross(mHeadVec, mFrontVec);
    if (MR::isNearZero(side)) {
        MR::vecBlendSphere(front, rFront, &blendedFront, 0.5f);
        side.cross(mHeadVec, blendedFront);
        setFrontVec(blendedFront);
        if (MR::isNearZero(side)) {
            setFrontVecKeepSide(rFront);
            return;
        }
    }
    MR::normalize(&side);
    mSideVec = side;
    MR::normalize(&mSideVec);
    TVec3f stack_14 = mSideVec.cross(mHeadVec);
    setFrontVec(stack_14);
    _22C = mFrontVec;
    f32 _328mag = _328.length();
    _328 = mFrontVec * _328mag;
    _344 = mSideVec;
}

void Mario::setFrontVecKeepSide(const TVec3f& rFront) {
    TVec3f headVec;
    TVec3f front;
    if (MR::isNearZero(rFront) == false) {
        setFrontVec(rFront);
    }

    headVec.cross(mFrontVec, mSideVec);
    if (MR::normalizeOrZero(&headVec) != nullptr) {
        const TVec3f* gravity = getGravityVec();
        TVec3f up = -(*gravity);
        mHeadVec = up;
        MR::normalize(&mHeadVec);
    } else {
        mHeadVec = headVec;
        MR::normalize(&mHeadVec);
    }
    front.cross(mSideVec, mHeadVec);
    setFrontVec(front);
    _22C = mFrontVec;
    f32 _328mag = _328.length();
    _328 = mFrontVec * _328mag;
}

void Mario::setHeadAndFrontVecFromRotate(const TVec3f& rRotate) {
    TVec3f frontVec = TVec3f(0.0f, 0.0f, 1.0f);
    TVec3f headVec = TVec3f(0.0f, 1.0f, 0.0f);
    Mtx mtx;
    MR::makeMtxTR(mtx, 0.0f, 0.0f, 0.0f, rRotate.x, rRotate.y, rRotate.z);
    PSMTXMultVec(mtx, (Vec*)&frontVec, (Vec*)&frontVec);
    PSMTXMultVecSR(mtx, (Vec*)&headVec, (Vec*)&headVec);
    mHeadVec = headVec;
    MR::normalize(&mHeadVec);
    setFrontVecKeepUp(frontVec);
}

void Mario::forceSetHeadVecKeepSide(const TVec3f& rVec) {
    mHeadVec = rVec;
    _1FC = rVec;
    if (MR::isSameDirection(rVec, mSideVec, 0.01f)) {
        fixSideVecFromFrontUp();
    }
    fixFrontVecFromUpSide();
    _43C = mFrontVec;
    _750 = 0;
}

// float regswap + bad stack ordering
void Mario::draw() const {
    if (mDrawStates._1) {
        TDDraw::cameraInit3D();
        TDDraw::setup(0, 1, 0);
        TVec3f* p_2A0 = &mActor->_2A0;
        TVec3f diff31C_2A0(mShadowPos);
        diff31C_2A0 -= *p_2A0;
        TDDraw::drawCylinder(mPosition, diff31C_2A0, 32.0f, 0xffff0010, 0xff000004, 8);
        TVec3f diff2A0_31C(mActor->_2A0);
        diff2A0_31C -= mShadowPos;
        f32 diffMag = diff2A0_31C.length();
        MR::normalize(&diff2A0_31C);
        TVec3f _31CPlusStuff(mShadowPos);
        f32 _37CMod10 = mActor->_37C % 0xa;
        _31CPlusStuff += diff2A0_31C * diffMag * 0.125f * _37CMod10 * 0.1f;
        f32 one_eighth = 0.125f;
        for (u32 i = 0; i < 8; i++) {
            TDDraw::drawCircle(_31CPlusStuff, diff2A0_31C, 100.0f, 0xffffff70, 0x10);
            _31CPlusStuff += diff2A0_31C * one_eighth * diffMag;
        }
    }
    if (_97C != nullptr) {
        _97C->draw3D();
    }
    drawTask();
}
/*
    r3 = mActor
    r0 = 0x4330
    r5 = 0x43300000
    r6 = 0xa
    r7 = mActor->_37C;
    r3 = stack_20
    stack_78 = 0x4330
    r4 = stack_68
    r0 = mActor->_37C / 0xa
    f1 = 0x43300000
    r0 = mActor->_37C / 0xa * 0xa
    r0 = mActor->_37C & 0xa

*/

void Mario::addTrans(const TVec3f& rShift, const char* a2) {
    mPosition += rShift;
    TVec3f _148shift(_148);
    _148shift -= rShift;
    if (MR::abs(_148.x) <= MR::abs(_148shift.x)) {
        _148.x = 0.0f;
    }
    if (MR::abs(_148.y) <= MR::abs(_148shift.y)) {
        _148.y = 0.0f;
    }
    if (MR::abs(_148.z) <= MR::abs(_148shift.z)) {
        _148.z = 0.0f;
    }
}

void Mario::setTrans(const TVec3f& rShift, const char* a2) {
    TVec3f reqShift(rShift);
    reqShift -= mPosition;
    mPosition = rShift;
    TVec3f _148shift(_148);
    _148shift -= reqShift;
    if (MR::abs(_148.x) <= MR::abs(_148shift.x)) {
        _148.x = 0.0f;
    }
    if (MR::abs(_148.y) <= MR::abs(_148shift.y)) {
        _148.y = 0.0f;
    }
    if (MR::abs(_148.z) <= MR::abs(_148shift.z)) {
        _148.z = 0.0f;
    }
}

bool Mario::isEnableRush() const {
    if (isHanging()) {
        return false;
    }
    if (isDamaging()) {
        return false;
    }
    if (isStatusActive(MarioStatus_Flow)) {
        return false;
    }
    if (mMovementStates._B && mMovementStates.jumping) {
        return false;
    }
    return true;
}

bool Mario::isInvincible() const {
    if (isPlayerModeInvincible()) {
        return true;
    }
    if (mMovementStates._1F) {
        return true;
    } else {
        return mActor->_3D8 != false;
    }
}

void Mario::inputStick() {
    mActor->getStickValue(&mStickPos.x, &mStickPos.y);

    if (_10._28) {
        _10._28 = 0;
        mStickPos.x = 0.0f;
        mStickPos.y = 0.0f;
    }

    mStickPos.x *= 1.5f;
    mStickPos.y *= 1.5f;
    mStickPos.x = MR::clamp(mStickPos.x, -1.0f, 1.0f);
    mStickPos.y = MR::clamp(mStickPos.y, -1.0f, 1.0f);

    _38 = mStickPos.z;
    const f32 stickX = mStickPos.x;
    const f32 stickY = mStickPos.y;
    mStickPos.z = MR::sqrt< f32 >((stickX * stickX) + (stickY * stickY));
    mStickPos.z = MR::clamp(mStickPos.z, 0.0f, 1.0f);

    if (MR::isNearZero(mStickPos.z, 0.01f)) {
        mStickPos.z = 0.0f;
    }

    f32 angle = JMath::sAtanTable.atan2_(mStickPos.x, mStickPos.y);
    angle = MR::normalizeAngleAbs(angle);

    MarioConstTable* table = mActor->mConst->getTable();
    const f32 halfPi = 1.5707964f;
    const f32 margin = table->mStickAngleMargin;

    u32 quarter = 0;
    while (angle > halfPi) {
        angle -= halfPi;
        quarter++;
    }

    if (angle >= halfPi - margin) {
        angle = halfPi;
    } else if (angle <= margin) {
        angle = 0.0f;
    } else {
        angle = (angle - margin) * (halfPi / (halfPi - (2.0f * margin)));
    }

    const f32 quantizedAngle = angle + (halfPi * static_cast< f32 >(quarter));

    if (!getPlayer()->_10._11) {
        f32 absAngle = quantizedAngle;
        if (absAngle < 0.0f) {
            absAngle = -absAngle;
        }

        f32 cosScaled = absAngle * 2607.5945f;
        u16 cosIndex = static_cast< u16 >(cosScaled);
        f32 cosValue = JMath::sSinCosTable.table[cosIndex & (JMath::TSinCosTable< 14, f32 >::LEN - 1)].b1;
        mStickPos.x = mStickPos.z * cosValue;

        f32 sinValue;
        if (quantizedAngle < 0.0f) {
            f32 sinScaled = quantizedAngle * -2607.5945f;
            u16 sinIndex = static_cast< u16 >(sinScaled);
            sinValue = -JMath::sSinCosTable.table[sinIndex & (JMath::TSinCosTable< 14, f32 >::LEN - 1)].a1;
        } else {
            f32 sinScaled = quantizedAngle * 2607.5945f;
            u16 sinIndex = static_cast< u16 >(sinScaled);
            sinValue = JMath::sSinCosTable.table[sinIndex & (JMath::TSinCosTable< 14, f32 >::LEN - 1)].a1;
        }

        mStickPos.y = mStickPos.z * sinValue;
    }

    _524 = _528;
    _528 = quantizedAngle;

    if (mMovementStates._37) {
        stick2DadjustGround(mStickPos.x, mStickPos.y);
    }
    if (_10._15) {
        stick2Dadjust(mStickPos.x, mStickPos.y);
    }
    if (mMovementStates._3A) {
        update25Dmode();
        updateAxisFromMode(_6AC);
    }
    if (_10._13) {
        mStickPos.y = 0.0f;
    }
    if (mStickPos.z <= 0.01f) {
        mMovementStates._1D = 0;
    }

    calcWorldPadDir(&mWorldPadDir, mStickPos.x, mStickPos.y, false);
    MR::normalizeOrZero(&mWorldPadDir);
}

bool Mario::isCeiling() const {
    if (_4C8->isValid() == false) {
        return false;
    } else {
        return !MR::isNearZero(_3A4);
    }
}

void Mario::setGravityVec(const TVec3f& rGravity) {
    if (MR::isSameDirection(mAirGravityVec, rGravity, 0.01)) {
        if (MR::isOppositeDirection(mAirGravityVec, rGravity, 0.01)) {
            _10._24 = 1;
            _10._25 = 1;
        }
        _1E4.zero();
    } else {
        _1E4.cross(mAirGravityVec, rGravity);
        MR::normalizeOrZero(&_1E4);
        f32 frontDot = MR::abs(mFrontVec.dot(_1E4));
        f32 sideDot = MR::abs(mSideVec.dot(_1E4));
        f32 headDot = MR::abs(mHeadVec.dot(_1E4));
        if (frontDot > sideDot && frontDot > headDot) {
            _10._A = 0;
            _10._26 = 1;
        }
        _10._24 = 1;
    }
    TVec3f stack_8 = -_368;
    _374 = stack_8;
    mAirGravityVec = rGravity;
}

void Mario::updateSoundCode() {
    s32 soundCode = -1;
    Mario* pPlayer = getPlayer();
    if (pPlayer->mMovementStates._1) {
        if (mGroundPolygon->isValid()) {
            soundCode = MR::getSoundCodeIndex(mGroundPolygon->getAttributes());
        }
    } else if (_45C->isValid()) {
        soundCode = MR::getSoundCodeIndex(_45C->getAttributes());
    }
    if (isStatusActive(MarioStatus_Wall)) {
        if (mMovementStates._8) {
            soundCode = MR::getSoundCodeIndex(mFrontWallTriangle->getAttributes());
        } else if (mMovementStates._19) {
            soundCode = MR::getSoundCodeIndex(mBackWallTriangle->getAttributes());
        } else if (mMovementStates._1A) {
            soundCode = MR::getSoundCodeIndex(mSideWallTriangle->getAttributes());
        }
    }
    MR::setMapSondCodeGravity(mActor, soundCode);
}

// nearly, last conditional doesn't want to cooperate
bool Mario::isForceStopRush() const {
    if (mVerticalSpeed != 0.0f) {
        goto exit_false;
    }
    u16 temp = _960;
    if (temp < (s16)0xf) {
        if (temp == (s16)4) {
            goto exit_true;
        }
        if (temp < (s16)4) {
            if (temp == (s16)1) {
                goto exit_true;
            }
            goto exit_false;
        }
        if (temp == (s16)0xa) {
            goto exit_true;
        }
        goto exit_false;
    }
    if (temp == (s16)0x18) {
        goto exit_true;
    }
    if (temp < (s16)0x18) {
        if (temp < (s16)0x11 == false) {
            goto exit_false;
        }
        goto exit_true;
    }
    if (temp == (s16)0x81 != false) {
        goto exit_true;
    }
    goto exit_false;

exit_true:
    return true;
exit_false:
    return false;
}

const TVec3f& Mario::getShadowNorm() const {
    return *_45C->getNormal(0);
}

const TVec3f& Mario::getAirGravityVec() const {
    if (MR::isNearZero(this->mAirGravityVec)) {
    }

    return mAirGravityVec;
}

const TVec3f Mario::getAirFrontVec() const {
    TVec3f airFront;
    MR::vecKillElement(mFrontVec, mAirGravityVec, &airFront);

    if (MR::normalizeOrZero(&airFront)) {
        return mFrontVec;
    }

    return airFront;
}

void Mario::initAfterConst() {
    MarioConst* pConst = mActor->mConst;
    _402 = pConst->getTable()->mAirWalkTime;
    mPosition = mActor->mPosition;
    mShadowPos = mPosition;
    mGroundPos = mPosition;
    mMove->initAfter();
    mFoo->init();
    mSwim->init();
    _7D4 = mPosition;
    _814 = mPosition;
    PSMTXIdentity(_7E4);
    PSMTXIdentity(_824);
    _720 = getAnimationStringPointer("ヒップドロップ開始");
    _724 = getAnimationStringPointer("ヒップドロップ");
    _728 = getAnimationStringPointer("ヒップドロップ着地");
}

void Mario::writeBackPhyisicalVector() {
    TVec3f stack_ec;
    TVec3f stack_e0;
    TVec3f stack_d4;
    TVec3f stack_c8;
    TVec3f stack_bc;
    TVec3f stack_b0;
    TVec3f stack_98;
    TVec3f stack_8c;
    if (_1C._10) {
        TVec3f stack_104(mPosition);
        addTrans(mVelocity, nullptr);
        if (calcDistToCeil(false) < 200.0f) {
            mVelocity.zero();
            stopWalk();
        }
        mPosition = stack_104;
    }

    if (mMovementStates._37 || _10._15) {
        TVec3f stack_f8(mPosition);
        stack_f8 -= _688;
        MR::vecKillElement(stack_f8, _6A0, &stack_f8);
        TVec3f stack_80(_688);
        stack_80 += stack_f8;
        mPosition = stack_80;
        MR::vecKillElement(mVelocity, _6A0, &mVelocity);
        TVec3f stack_74(_694);
        stack_74 -= _688;
        f32 f29 = MR::vecKillElement(stack_74, _6A0, &stack_ec);
        if (f29 > 1.0f) {
            f29 = 1.0f;
        } else if (f29 < -1.0f) {
            f29 = -1.0f;
        }
        stack_ec = _6A0;
        stack_ec.setLength(f29);
        _688 += stack_ec;
    }

    fixPositionInTower();

    if (mMovementStates.jumping) {
        bool rising = false;
        if (isRising()) {
            rising = true;
        }
        if (mActor->_334 != 0) {
            _578 = 0;
        }

        if (!mMovementStates._22) {
            for (u32 i = 0; i < _578; i++) {
                if (MR::vecKillElement(mVelocity, *_57C[i]->getNormal(0), &stack_e0) < 0.0f) {
                    mVelocity = stack_e0;

                    if (!_1C._D) {
                        f32 f28 = MR::vecKillElement(mJumpVec, *_57C[i]->getNormal(0), &stack_d4);
                        if (f28 <= 0.0f) {
                            TVec3f stack_50(*_57C[i]->getNormal(0));
                            stack_50.scale(f28);
                            TVec3f stack_5c(stack_50);
                            stack_5c.scale(0.5f);
                            TVec3f stack_68(stack_d4);
                            stack_68 += stack_5c;
                            mJumpVec = stack_68;
                        } else if (MR::diffAngleAbs(*getGravityVec(), mJumpVec) < 0.10471976f) {
                            MR::vecKillElement(*_57C[i]->getNormal(0), *getGravityVec(), &stack_c8);
                            f32 jumpMagnitude = PSVECMag(&mJumpVec);
                            MR::normalizeOrZero(&stack_c8);
                            mJumpVec += stack_c8;
                            mJumpVec.setLength(jumpMagnitude);
                        }
                    }

                    if (rising && _57C[i]->getNormal(0)->dot(*getGravityVec()) > 0.707f) {
                        mActor->sendMsgUpperPunch(_57C[i]->mSensor);
                    }

                    if (isPlayerModeBee() && !mActor->mAlphaEnable) {
                        const char* pWallCode = MR::getWallCodeString(_57C[i]);
                        if (pWallCode != nullptr && strcmp(pWallCode, "Fur") == 0) {
                            Triangle stack_110;
                            MarioActor* pActor = mActor;
                            TVec3f stack_38 = -*_57C[i]->getNormal(0);
                            TVec3f stack_44(stack_38);
                            stack_44.scale(300.0f);

                            if (MR::getFirstPolyOnLineToMap(&stack_bc, &stack_110, pActor->_2A0, stack_44)) {
                                const char* pHitWallCode = MR::getWallCodeString(&stack_110);
                                if (pHitWallCode != nullptr && strcmp(pHitWallCode, "Fur") == 0) {
                                    MarioActor* pActor2 = mActor;
                                    pActor2->entryWallWalkMode(stack_bc, *stack_110.getNormal(0));
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (isSwimming() && !mMovementStates._22) {
        for (u32 i = 0; i < _578; i++) {
            if (MR::vecKillElement(mVelocity, *_57C[i]->getNormal(0), &stack_b0) < 0.0f) {
                mVelocity = stack_b0;
                f32 f28 = MR::vecKillElement(mSwim->mKnockbackVel, *_57C[i]->getNormal(0), &mSwim->mKnockbackVel);
                TVec3f stack_20(*_57C[i]->getNormal(0));
                stack_20.scale(f28);
                TVec3f stack_2c(stack_20);
                stack_2c.scale(0.5f);
                mSwim->mKnockbackVel += stack_2c;
            }
        }
    } else if (!_1C._C && mActor->_288.dot(_368) < 0.707f && !MR::isNearZero(mActor->_288, 1.0f)) {
        TVec3f stack_a4(mActor->_288);
        MR::normalizeOrZero(&stack_a4);
        f32 push = MR::vecKillElement(mVelocity, stack_a4, &stack_98);
        if (push < 0.0f) {
            mVelocity = stack_98;
            MR::normalizeOrZero(&stack_a4);
            f32 pushScale = 1.0f - MR::clamp((-stack_a4.dot(mFrontVec) - 0.3f) * 1.4285715f, 0.0f, 1.0f);

            if (!mMovementStates._8 && pushScale < 0.1f) {
                if (mMovementStates._1A) {
                    TVec3f stack_14(getSideWallNorm());
                    stack_14.scale(-push);
                    mVelocity += stack_14;
                } else {
                    u32 planeCount = mActor->mBinder->mPlaneNum;
                    if (planeCount != 0) {
                        f32 mostFacingDot = 0.0f;
                        u32 index = 0;
                        for (u32 i = 0; i < planeCount; i++) {
                            f32 dot = mActor->mBinder->getPlane(i)->getNormal(0)->dot(mFrontVec);
                            if (dot < mostFacingDot) {
                                index = i;
                                mostFacingDot = dot;
                            }
                        }

                        TVec3f stack_8(mPosition);
                        const HitInfo* pInfo = reinterpret_cast< const HitInfo* >(mActor->mBinder->getPlane(index));
                        stack_8 -= pInfo->mHitPos;
                        MR::vecKillElement(stack_8, *getGravityVec(), &stack_8c);
                        MR::vecKillElement(stack_8c, stack_a4, &stack_8c);
                        if (!MR::isNearZero(stack_8c)) {
                            stack_8c.setLength(-push);
                            mVelocity += stack_8c;
                        }
                    }
                }
            }

            _71C = 0;
            _278 *= pushScale;
        }
    }

    mActor->mVelocity = mVelocity;
    mActor->mPosition = mPosition;
}

#pragma dont_inline on
void Mario::update() {
    OSGetTime();
    updateAndClearStrideParameter();
    checkKeyLock();

    if (!(mMovementStates.jumping && mMovementStates._B) && _728 != nullptr && isAnimationRun(_728)) {
        mMovementStates.jumping = 1;
        mMovementStates._B = 1;
    }

    updateCubeCode();
    mMovementStates._7 = updateBinderInfo();
    checkEnforceMove();
    _A8C[0] = mVelocity;

    if (!isStatusActive(MarioStatus_Stick) && !isStatusActive(MarioStatus_Hang)) {
        checkBaseTransPoint();
        checkHeadPoint();
    }

    if (mMovementStates._F && _544 > 1) {
        createAtField(true, 150.0f);
    } else {
        createAtField(false, 40.0f);
    }

    _72C = calcDistToCeil(true);
    _1C._F = _72C < 160.0f;
    if (_1C._F && _4C8->isValid()) {
        damagePolygonCheck(_4C8);
    }

    if (MR::getKarikariClingNum() != 0) {
        _1C_WORD |= 0x04000000;
    }

    OSGetTime();
    f32 wallDistance = 80.0f;
    if (isSwimming()) {
        wallDistance = 90.0f;
    }
    checkAllWall(mActor->_2A0, wallDistance);

    OSGetTime();
    updateGroundInfo();
    OSGetTime();

    if (mMovementStates._1 && (!mMovementStates.jumping || mMovementStates._B)) {
        if (damageFloorCheck()) {
            writeBackPhyisicalVector();
            return;
        }

        saveLastSafetyTrans();
        if (isCurrentShadowFloorDangerAction()) {
            TVec3f floorDelta(mShadowPos);
            floorDelta -= mGroundPos;
            MR::isNearZero(mAirGravityVec);

            TVec3f horizontal;
            f32 vertical = MR::vecKillElement(floorDelta, mAirGravityVec, &horizontal);
            if (vertical <= 5.0f && floorDelta.dot(mFrontVec) > 0.0f) {
                mDrawStates_WORD |= 0x00200000;
                floorDelta.setLength(5.0f);
                addVelocity(floorDelta);
            }
        }
    }

    mDrawStates_WORD |= 0x00008000;
    if (damageWallCheck()) {
        return;
    }
    mDrawStates_WORD &= ~0x00008000;

    OSGetTime();
    if (checkPressDamage()) {
        return;
    }

    OSGetTime();
    if (checkSliderMode()) {
        startSlider();
    }

    checkAndTryForceJump();
    if (mMovementStates._2E) {
        return;
    }

    if (checkStartSwim()) {
        writeBackPhyisicalVector();
        return;
    }

    if (!mMovementStates._22) {
        check2DMode();
    }

    _898 = 0;
    if (!mMovementStates.debugMode && MR::testFpViewStartTrigger()) {
        if (MR::isPossibleToShiftToFirstPersonCamera()) {
            tryFpViewMode();
        } else if (!MR::isDemoActive()) {
            if (!MR::isEqualStageName("EpilogueDemoStage")) {
                MR::startSystemSE("SE_SY_CAMERA_NG", -1, -1);
            }
            _898 = 1;
        }
    }

    inputStick();
    checkLockOnHoming();

    _A8C[1] = mVelocity;
    actionMain();
    _A8C[3] = mVelocity;

    calcFrontFloor();

    if (mMovementStates._23 && mMovementStates._1 && mMovementStates._24 && isSlipPolygon(_460)) {
        f32 removed = MR::vecKillElement(mVelocity, *_460->getNormal(0), &mVelocity);
        TVec3f normalDelta(_368);
        normalDelta -= *_460->getNormal(0);
        TVec3f halfDelta(normalDelta);
        halfDelta.scale(0.5f);
        removed = __fabsf(removed);
        TVec3f adjustment(halfDelta);
        adjustment.scale(removed);
        mVelocity += adjustment;
    }

    _A8C[4] = mVelocity;
    tryPushToVelocity();
    powerAreaMove();
    powerRailMove();
    _A8C[5] = mVelocity;

    addVelocity(mVelocityAfter);
    _A8C[6] = mVelocity;

    checkForceGrounding();
    _A8C[7] = mVelocity;

    checkStep();
    checkBump();
    doCubeWarp();

    if (isSwimming()) {
        checkBaseTransBall();
    }

    doSpinPunchAroundPolygons();
    _A8C[8] = mVelocity;

    writeBackPhyisicalVector();
    updateTimers();
    doExtraServices();
}
#pragma dont_inline reset

void Mario::actionMain() {
    if (mMovementStates.debugMode) {
        return;
    }

    if (!checkDamage()) {
        if (_97C != nullptr) {
            sendStateMsg(MarioStateMsg_Update);
        } else if (mMovementStates.jumping) {
            procJump(false);
            checkTornado();
            checkHang();
            checkWallStick();
        } else {
            mainMove();
            updateWalkSpeed();
        }

        if (mMovementStates._1) {
            mMovementStates._30 = 1;
            mMovementStates._31 = 1;
            doFrontStep();
        }
    }

    if (!mMovementStates._23) {
        _1A8 = mVelocity;
        _3C2 = 0;
        _280 = 0.0f;
        _284.zero();
    } else {
        _3C2++;
    }

    if (_3C4 != 0) {
        _3C4--;
    }

    _A8C[2] = mVelocity;
    if (mMovementStates._1 && !mMovementStates.jumping) {
        if (isEnableSlopeMove()) {
            slopeMove();
        } else {
            clearSlope();
        }
    }

    if (isStickOn() && isAnimationRun("水泳陸うちあげ")) {
        stopAnimation(nullptr, static_cast< const char* >(nullptr));
    }

    if (!mDrawStates._4) {
        _3FE = 0;
    }

    callExtraTasks(0);
    beeMarioOnGround();
    checkSpecialWaitAnimation();

    if (!mDrawStates._16) {
        retainMoveDir(mStickPos.x, mStickPos.y, nullptr);
    }
}

void Mario::updateGroundInfo() {
    checkMap();

    if (isEnableCheckGround()) {
        bool wasAirborne = !mMovementStates.jumping && !mMovementStates._1;
        mMovementStates._1 = checkGround();

        if (mDrawStates._18) {
            _3C6++;
        } else {
            _3C6 = 0;
        }

        if (wasAirborne && mMovementStates._1 && isDefaultAnimationRun("落下")) {
            changeAnimation(nullptr, "基本");
        }

        if (!isStatusActive(MarioStatus_13)) {
            if (!isStatusActive(MarioStatus_13) && !mMovementStates._1 && mMovementStates._3E == 1 &&
                mJumpVec.dot(*getGravityVec()) > 0.0f && mJumpVec.dot(*_45C->getNormal(0)) < 0.0f &&
                _1FC.dot(*getGravityVec()) > 0.0f && mVerticalSpeed < 170.0f) {
                mMovementStates._1 = 1;
                setGroundNorm(*_45C->getNormal(0));
                setTrans(mShadowPos, nullptr);
            }
        }
    }

    updateCameraPolygon();
    if (mMovementStates._1) {
        mMovementStates._29 = 0;
        if (_43A == 0 && isSlipPolygon(mGroundPolygon)) {
            mDrawStates._C = 1;
        }
    }

    if (_43A != 0) {
        _43A--;
    }

    updateFloorCode();
    updateSoundCode();
    updateOnSand();
    updateOnWater();
    updateOnPoison();
}

bool Mario::postureCtrl(MtxPtr mtx) {
    TVec3f oldHeadUp(_1FC);
    TVec3f targetHeadUp(mHeadVec);

    MarioConstTable* pTable = mActor->mConst->getTable();
    f32 blendRatio = pTable->mHeadRotateRatio;

    if (mMovementStates._1 && !isSlipFloorCode(_960)) {
        if (_71C < 3) {
            f32 angle = calcAngleD(*mGroundPolygon->getNormal(0));
            if (angle < 45.0f) {
                MR::isNearZero(mAirGravityVec);
                targetHeadUp = -mAirGravityVec;
            } else {
                f32 blend = (calcAngleD(*mGroundPolygon->getNormal(0)) - 45.0f) / 60.0f;
                MR::isNearZero(mAirGravityVec);
                TVec3f negGravity = -mAirGravityVec;
                MR::vecBlendSphere(negGravity, targetHeadUp, &targetHeadUp, blend);
            }
        } else {
            MR::isNearZero(mAirGravityVec);
            TVec3f gravity = -*getGravityVec();
            gravity -= mAirGravityVec;
            targetHeadUp = gravity;
            MR::normalizeOrZero(&targetHeadUp);
        }
    }

    if (isAnimationRun("坂すべり上向きうつぶせ", 2)) {
        targetHeadUp = _368;
    }

    if (isStatusActive(MarioStatus_Wait)) {
        if (isAnimationRun("特殊ウエイト1A")) {
            f32 blend;
            if (getAnimator()->getFrame() > 290.0f) {
                blend = 1.0f;
            } else if (getAnimator()->getFrame() > 260.0f) {
                blend = 0.5f + 0.5f * ((getAnimator()->getFrame() - 260.0f) / 30.0f);
            } else {
                blend = 0.5f * (getAnimator()->getFrame() / 260.0f);
            }

            TVec3f gravity = -_374;
            MR::vecBlendSphere(gravity, targetHeadUp, &targetHeadUp, blend);
        } else if (_1C._13 && _1C._14) {
            targetHeadUp = -_374;
        }
    }

    if (!isSwimming()) {
        if (isAnimationRun("水泳ジェット") || isAnimationRun("水泳スピン移動") || isAnimationRun("リングダッシュ") ||
            isAnimationRun("水泳スピンジャンプ")) {
            MR::vecBlendSphere(mHeadVec, mJumpVec, &targetHeadUp, 0.1f);
            if (MR::normalizeOrZero(&targetHeadUp)) {
                targetHeadUp = mHeadVec;
            }
            mDrawStates_WORD |= 0x00004000;
        }
    }

    calcTornadoTilt();
    slopeTiltHead(&targetHeadUp);

    isStatusActive(MarioStatus_Climb);

    pTable = mActor->mConst->getTable();
    f32 angle = MR::diffAngleAbs(oldHeadUp, targetHeadUp);
    if (angle > pTable->mHeadAngleFixMargin) {
        if (!MR::vecBlendSphere(oldHeadUp, targetHeadUp, &oldHeadUp, blendRatio)) {
            TMtx34f rotMtx;
            PSMTXRotAxisRad(rotMtx.toMtxPtr(), &mSideVec, 0.1f);
            PSMTXMultVecSR(rotMtx.toMtxPtr(), &oldHeadUp, &oldHeadUp);
        }
    }

    if (!MR::normalizeOrZero(&oldHeadUp)) {
        _1FC = oldHeadUp;
    }

    return false;
}

void Mario::createAngleMtx(MtxPtr mtx, bool forceNoFix) {
    if (!isNonFixHeadVec() && !forceNoFix && !mMovementStates.debugMode) {
        fixHeadFrontVecByGravity();
    }

    if (updatePosture(mtx)) {
        return;
    }

    createDirectionMtx(mtx);

    TVec3f sideVec;
    TVec3f headUp;
    TVec3f frontVec;
    sideVec = mSideVec;
    headUp = _1FC;
    frontVec = mFrontVec;

    PSMTXConcat(_C4, mtx, mtx);

    HitSensor* pSensor = mActor->_470;
    if (isAnimationRun("投げ")) {
        pSensor = mActor->_474;
    }
    if (mActor->isPunching()) {
        pSensor = mActor->_924;
    }

    TVec3f sensorPos;
    if (pSensor != nullptr) {
        sensorPos = pSensor->mPosition;
    }

    if (pSensor != nullptr) {
        TVec3f toSensor(sensorPos);
        toSensor -= mPosition;

        TVec3f sensorDir(toSensor);
        MR::normalizeOrZero(&sensorDir);
        if (MR::isNearZero(sensorDir)) {
            return;
        }

        TVec3f up(headUp);
        TVec3f noUpDir;
        MR::vecKillElement(sensorDir, up, &noUpDir);
        if (MR::normalizeOrZero(&noUpDir)) {
            return;
        }

        f32 angle = JMAAcosRadian(noUpDir.dot(frontVec));

        TVec3f cross;
        PSVECCrossProduct(&frontVec, &noUpDir, &cross);
        if (cross.dot(headUp) < 0.0f) {
            angle = -angle;
        }

        JMAAcosRadian(noUpDir.dot(sensorDir));
        bool sensorAbove = sensorDir.dot(up) >= 0.0f;
        if (__fabsf(angle) < 1.5707964f && sensorAbove) {
        } else if (__fabsf(angle) >= 1.5707964f && !sensorAbove) {
        }

        f32 maxAngle = 0.1f * static_cast< f32 >(_412);
        if (maxAngle > 1.0471976f) {
            maxAngle = 1.0471976f;
        }
        angle = MR::clamp(angle, -maxAngle, maxAngle);

        _58.x = angle;
        _58.y = 0.0f;
        _58.z = 0.0f;
        MR::orderRotateMtx(0, _58, _64);

        TVec3f invAngle = -_58;
        MR::orderRotateMtx(0, invAngle, _94);
        _12C = 0x10;
    } else {
        if (_12C != 0) {
            _12C--;
        }

        _58.x *= 0.8f;
        _58.y = 0.0f;
        _58.z *= 0.8f;

        MR::orderRotateMtx(0, _58, _64);

        TVec3f invAngle = -_58;
        MR::orderRotateMtx(0, invAngle, _94);
    }
}

void Mario::updateLookOfs() {
    TVec3f lookOffset;
    lookOffset.zero();

    if (isStatusActive(MarioStatus_Blown)) {
        return;
    }

    if (mActor->_934) {
        MR::vecBlend(_13C, lookOffset, &_13C, 0.3f);
    }

    if (_97C == nullptr) {
        if (isAnimationRun("壁押し")) {
            TVec3f wallDelta(_4E8);
            wallDelta -= mPosition;

            TVec3f horizontalWallDelta;
            MR::vecKillElement(wallDelta, mHeadVec, &horizontalWallDelta);

            f32 distance = horizontalWallDelta.length();
            TVec3f wallNormal(*mFrontWallTriangle->getNormal(0));
            wallNormal.scale(distance - 35.0f);
            lookOffset = wallNormal;
        }
    } else if (_97C->mStatusId == MarioStatus_Swim) {
        TVec3f antiGravity = -*getGravityVec();
        antiGravity.scale(mSwim->mSurfaceOffset);
        lookOffset += antiGravity;
    }

    if (mMovementStates._1 && !mMovementStates.jumping && mVerticalSpeed > 5.0f && mVerticalSpeed < 50.0f &&
        !isCurrentShadowFloorDangerAction()) {
        TVec3f groundDelta(mGroundPos);
        groundDelta -= mShadowPos;
        MR::normalizeOrZero(&groundDelta);

        if (groundDelta.dot(_368) < 0.1f) {
            TVec3f gravity(*getGravityVec());
            gravity.scale(mVerticalSpeed);
            lookOffset += gravity;
        } else {
            TVec3f movementDelta(mGroundPos);
            movementDelta -= mPosition;

            TVec3f horizontalMovement;
            f32 gravityDistance = MR::vecKillElement(movementDelta, *getGravityVec(), &horizontalMovement);
            TVec3f gravity(*getGravityVec());
            gravity.scale(gravityDistance);
            lookOffset += gravity;
        }
    }

    TVec3f headOffset;
    headOffset.zero();
    if (_735 != 0) {
        f32 scale = static_cast< f32 >(_735);

        TVec3f gravity(*getGravityVec());
        gravity.scale(scale);
        gravity.scale(0.00390625f);
        gravity.scale(120.0f);
        headOffset = gravity;

        TVec3f headUp = -_1FC;
        headUp.scale(scale);
        headUp.scale(0.00390625f);
        headUp.scale(120.0f);
        headOffset = headUp;
    }

    f32 blend = 0.2f;
    if (!isUseSimpleGroundCheck()) {
        if (getAnimator()->isLandingAnimationRun()) {
            blend = 0.01f;
        } else if (_71C == 0 && _278 < 0.2f && isBlendWaitGround()) {
            f32 targetMagnitude = lookOffset.length();
            f32 currentMagnitude = _148.length();
            if (currentMagnitude > targetMagnitude) {
                blend = 0.5f;
            } else {
                blend = 0.02f * static_cast< f32 >(_3CE);
                if (blend >= 0.2f) {
                    blend = 0.2f;
                }
            }
        }
    }

    MR::vecBlend(_148, lookOffset, &_148, blend);
    MR::vecBlend(_154, headOffset, &_154, 0.2f);

    TVec3f combinedOffset(_148);
    combinedOffset += _154;
    _13C = combinedOffset;
}

// conditionals won't behave
const TVec3f* Mario::getGravityVec() const {
    if (isStatusActive(MarioStatus_Bump) || isStatusActive(MarioStatus_Climb)) {
        return &_790;
    }
    if (mMovementStates._1 && !isSlipFloorCode(_960)) {
        if (mActor->mAlphaEnable) {
            return &mAirGravityVec;
        }
        if (isAnimationRun("ハード着地")) {
            return &mAirGravityVec;
        }
        if (_3CE < 0x10 && _3BC > 3 && !mMovementStates._B) {
            return &mAirGravityVec;
        }
        if (MR::isNearZero(_374) == false && _1C._13 && _1C._14) {
            return &_374;
        }
    }
    if (isSwimming()) {
        if (mSwim->mWaterDepth >= -150.0f && mDrawStates._F && MR::isNearZero(_38C) == false) {
            return &_38C;
        }
        return &mAirGravityVec;
    }
    if (MR::isNearZero(mAirGravityVec)) {
    }
    bool b1 = true;
    if (_430 == 0xc) {
        b1 = false;
    } else if (_430 == 0xd) {
        b1 = false;
    }
    if (!isSlipFloorCode(_960) && b1 && !isPlayerModeHopper() && !isPlayerModeTeresa() && !isDamaging() && !isStatusActive(MarioStatus_Swim) &&
        !isStatusActive(MarioStatus_Blown) && !isStatusActive(MarioStatus_13) && _430 != 5 && mActor->_334 == 0 && mMovementStates.jumping &&
        !mMovementStates._22 && _3BC < 8) {
        return &_374;
    }
    return &mAirGravityVec;
}

void Mario::touchWater() {
    if (mActor->isActionOk("水解除") && mMorphResetTimer == 0) {
        mMorphResetTimer = 10;
    }
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
}  // namespace NrvMarioActor

XjointTransform* XanimeCore::getJointTransform(u32 idx) {
    if (mTransformList == nullptr) {
        return nullptr;
    }

    return &mTransformList[idx];
}

void MarioState::draw3D() const {
}

template <>
bool TriangleFilterDelegator< Mario >::isInvalidTriangle(const Triangle* pTriangle) const {
    return (mParent->*mFunc)(pTriangle);
}
