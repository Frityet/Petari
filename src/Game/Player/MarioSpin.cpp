#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/MathUtil.hpp"

static const f32 sOne = 1.0f;
static const f32 sZero = 0.0f;

void Mario::checkTornado() {
    if (mMovementStates._1) {
        mMovementStates._2B = false;
    }
}

void Mario::resetTornado() {
    _530 = sZero;
    _534 = 0;
    _538 = sZero;
    mMovementStates._F = false;
    _544 = 0;
    mYAngleOffset = sZero;
    _3F0 = sOne;
}

void Mario::calcTornadoTilt() {
    u32 tornado = mMovementStates._F;
    u32 special = 0;

    if (getPlayerMode() == 4 && mMovementStates.jumping && mMovementStates._11) {
        special = true;
    }

    special = (tornado | special) != 0;

    if (!isStickOn() || !special) {
        _548 *= mActor->mConst->getTable()->mTornadoTiltCancel;

        bool blended;
        if (special) {
            blended = MR::vecBlendSphere(_54C, mHeadVec, &_54C, mActor->mConst->getTable()->mTornadoTiltSpeed);
        } else {
            blended = MR::vecBlendSphere(_54C, mHeadVec, &_54C, mActor->mConst->getTable()->mTornadoTiltOffSpeed);
        }

        if (!blended) {
            _54C = mHeadVec;
        }

        return;
    }

    TVec3f tilt = getWorldPadDir() * mActor->mConst->getTable()->mTornadoTiltAngle
        + mHeadVec * (sOne - mActor->mConst->getTable()->mTornadoTiltAngle);
    MR::normalize(&tilt);

    const bool blended = MR::vecBlendSphere(_54C, tilt, &_54C, mActor->mConst->getTable()->mTornadoTiltSpeed);
    MR::normalize(&_54C);

    const f32 padDot = __fabsf(getWorldPadDir().dot(mFrontVec));
    const f32 near = mActor->mConst->getTable()->mTornadoTiltNear;
    _548 = _548 * near + padDot * (sOne - near);

    if (!blended) {
        _54C = tilt;
    }
}

void Mario::reflectWallOnSpinning(const TVec3f& rFront, u16 timer) {
    setFrontVecKeepUp(rFront);
    _3F8 = timer;
    _328 = mFrontVec;
    doSpinWallEffect();
}

void Mario::forceStopTornado() {
    if (mMovementStates._F) {
        _40A = mActor->mConst->getTable()->mTornadoRestartTime;
    }

    resetTornado();

    if (mMovementStates.jumping) {
        cancelTornadoJump();
    }

    mDrawStates._8 = true;
}

void Mario::startRotationTask(u32 flags) {
    pushTask(&Mario::taskOnRotation, flags);
}

void Mario::doSpinWallEffect() {
    if (mMovementStates._8 && mFrontWallTriangle->mSensor->isType(0x55)) {
        return;
    }

    if (mMovementStates._19 && mBackWallTriangle->mSensor->isType(0x55)) {
        return;
    }

    if (mMovementStates._1A && mSideWallTriangle->mSensor->isType(0x55)) {
        return;
    }

    playSound("壁反射", -1);
    playSound("声スピンキャンセル", -1);
    playEffect("壁スパーク");
}

bool Mario::taskOnRotation(u32 flags) {
    if (flags & 4) {
        if (!isAnimationRun("ヘリコプタージャンプ")) {
            mYAngleOffset = sZero;
            return false;
        }

        if (isRising()) {
            mYAngleOffset += mActor->mConst->getTable()->mTrampleBegomaRotRise;
        } else {
            mYAngleOffset += mActor->mConst->getTable()->mTrampleBegomaRotFall;
        }
    }

    return true;
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
