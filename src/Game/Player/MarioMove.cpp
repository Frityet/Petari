#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/MathUtil.hpp"

namespace {
    static f32 clampStick(f32 value) {
        if (value > 1.0f) {
            return 1.0f;
        }

        if (value < -1.0f) {
            return -1.0f;
        }

        return value;
    }

    static f32 absf(f32 value) {
        return value < 0.0f ? -value : value;
    }
};

void Mario::mainMove() {
    if (mMovementStates._37) {
        stick2Dadjust(mStickPos.x, mStickPos.y);
    }

    TVec3f moveDir;
    calcMoveDir(mStickPos.x, mStickPos.y, &moveDir, true);

    if (MR::isNearZero(moveDir)) {
        _40C = 10;
        moveDir = mFrontVec;
        MR::vecKillElement(moveDir, *getGravityVec(), &moveDir);
        MR::normalizeOrZero(&moveDir);
    } else {
        _328 = moveDir;
        calcShadowDir(moveDir, &_22C);

        if (!mDrawStates._5 && isEnableTurn()) {
            setFrontVecKeepUp(_22C);
        }
    }

    calcShadowDir(mFrontVec, &_214);

    const MarioConstTable* table = mActor->getConst().getTable();
    f32 walkSpeed = table->mWalkSpeed;

    if (mMovementStates.debugMode) {
        walkSpeed = table->mDebugMoveSpeed;
    } else if (_3D0 != 0) {
        walkSpeed = table->mFastTurnSpeed;
    }

    if (!MR::isNearZero(moveDir)) {
        addVelocity(moveDir, walkSpeed * mStickPos.z);
    }

    _278 = decideInertia(mStickPos.z);

    checkLockOnHoming();
    fixPositionInTower();
}

bool Mario::isEnableTurn() {
    if (!mMovementStates._1 || mMovementStates._4 || mDrawStates._5 || mMovementStates._23
        || mMovementStates._34 || mDrawStates._A || isStatusActive(0x11)) {
        return false;
    }

    if (isAnimationRun("スピンアタック", 2) || isAnimationRun("スピンジャンプ", 3) || isAnimationRun("壁押し")
        || isAnimationRun("壁押し上") || isAnimationRun("壁押し下") || isAnimationRun("泳ぎ")
        || mActor->_480 || mActor->isPunching() || mActor->isItemSwinging() || mActor->mAlphaEnable
        || mActor->_3C0 || mActor->_EA4 || mMovementStates._37 || _10._2B) {
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
    const MarioConstTable* table = mActor->getConst().getTable();

    if (isStatusActive(0x1F) || mMovementStates._34) {
        return decideInertiaOnIce(stickPower);
    }

    if (mMovementStates._35) {
        return decideInertiaOnSlip(stickPower);
    }

    if (_278 > 1.0f) {
        return table->mInertiaOverSpeed;
    }

    f32 inertia = ((1.0f - _278) * table->mInertiaStandardStop + _278 * table->mInertiaStandardMax) * (1.0f - _3F4)
        + _3F4 * table->mInertiaStartSpin;

    if (stickPower == 0.0f) {
        inertia = table->mInertiaStop;

        if (mMovementStates._10) {
            inertia = table->mInertiaTurnSlip;
        }

        if (mMovementStates._4) {
            inertia = table->mInertiaTurning;
        }

        if (_3CE < 30) {
            inertia = table->mInertiaJumpFinish;
        }

        if (mMovementStates._A) {
            inertia = table->mInertiaSquat;
        }
    }

    if (mMovementStates._11) {
        if (_278 >= stickPower) {
            inertia = table->mInertiaBrake;
        } else {
            inertia = table->mInertiaBdashAfter;
        }
    }

    if (_3F8 != 0) {
        _3F8--;
        inertia = table->mInertiaReflectSlip;
    }

    if (_278 < 0.1f && _3CE > 10 && stickPower > 0.5f && (mMovementStates._A || mMovementStates._C)) {
        _3FA = table->mStartSpinTime;
        _278 = 0.1f;
    }

    if (_3FA != 0) {
        _3FA--;
        inertia = table->mInertiaStartSpin;

        if (_3FA == 0) {
            _3FC = 60;
        }
    }

    if (_3FC != 0) {
        _3FC--;
    }

    if (getFloorCode() == 0x20 && _278 > 0.7f) {
        inertia *= 0.5f;
    }

    return inertia;
}

f32 Mario::decideInertiaOnIce(f32 stickPower) {
    const MarioConstTable* table = mActor->getConst().getTable();

    if (stickPower > 1.0f) {
        stickPower = 1.0f;
    }

    f32 inertia = ((1.0f - stickPower) * table->mInertiaIceStandardStop + stickPower * table->mInertiaIceStandardMax)
        * (1.0f - _3F4) + _3F4 * table->mInertiaIceStartSpin;

    if (stickPower == 0.0f) {
        inertia = table->mInertiaIceStop;
    }

    return inertia;
}

f32 Mario::decideInertiaOnSlip(f32 stickPower) {
    const MarioConstTable* table = mActor->getConst().getTable();
    f32 inertia = ((1.0f - _278) * table->mInertiaSlipStandardStop + _278 * table->mInertiaSlipStandardMax)
        * (1.0f - _3F4) + _3F4 * table->mInertiaSlipStartSpin;

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

    if (mMovementStates._37 || _10._2B) {
        calcShadowDir2D(moveDir, pOut);
        return;
    }

    TVec3f base(-*getGravityVec());
    const f32 gravityDot = moveDir.dot(base);
    moveDir.dot(mHeadVec);
    const f32 airDot = moveDir.dot(_368);

    if (absf(gravityDot) > absf(airDot)) {
        base = _368;
    }

    TVec3f side;
    side.cross(base, moveDir);
    pOut->cross(side, base);
    MR::normalizeOrZero(pOut);
}

void Mario::retainMoveDir(f32 stickX, f32 stickY, TVec3f* pOut) {
    TVec3f moveDir;
    calcMoveDir(stickX, stickY, &moveDir, false);

    if (!MR::isNearZero(moveDir)) {
        _328 = moveDir;
    }

    if (pOut != nullptr) {
        *pOut = _328;
    }
}

void Mario::calcMoveDir(f32 stickX, f32 stickY, TVec3f* pOut, bool doRetain) {
    if (mMovementStates._37) {
        calcMoveDir2D(stickX, stickY, pOut);
        return;
    }

    if (mMovementStates._3A) {
        calcMoveDir25D(stickX, stickY, pOut);
        return;
    }

    stickX = clampStick(stickX);
    stickY = clampStick(stickY);

    if (MR::isNearZero(stickX) && MR::isNearZero(stickY)) {
        pOut->zero();

        if (doRetain && !MR::isNearZero(_328)) {
            *pOut = _328;
        }

        return;
    }

    TVec3f camX(getCamDirX());
    TVec3f camZ(getCamDirZ());
    const TVec3f& gravity = *getGravityVec();

    MR::vecKillElement(camX, gravity, &camX);
    MR::vecKillElement(camZ, gravity, &camZ);

    if (MR::normalizeOrZero(&camX)) {
        camX = mSideVec;
    }

    if (MR::normalizeOrZero(&camZ)) {
        camZ = mFrontVec;
    }

    TVec3f xMove(camX * stickX);
    TVec3f zMove(camZ * stickY);
    *pOut = xMove + zMove;

    if (MR::normalizeOrZero(pOut)) {
        pOut->zero();
    }
}

bool Mario::checkLockOnHoming() {
    if (mStickPos.z != 0.0f || !checkPreLvlZ() || isSwimming() || mActor->_470 == nullptr) {
        return false;
    }

    mMovementStates._15 = false;
    mDrawStates._6 = true;
    doLockOnHoming();
    return true;
}

void Mario::doLockOnHoming() {
    TVec3f front(_22C);

    if (MR::isNearZero(front)) {
        front = _328;
    }

    if (!MR::isNearZero(front)) {
        setFrontVecKeepUp(front);
    }
}

void Mario::fixPositionInTower() {
    if (!mMovementStates._3A) {
        return;
    }

    MR::vecKillElement(mPosition - _694, _6A0, &_688);
}
