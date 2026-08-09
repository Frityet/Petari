#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include <revolution/mtx.h>

void Mario::stick2DadjustGround(f32& rStickX, f32& rStickY) {
    if (isStageCameraRotate2D()) {
        return;
    }

    if (_10.turning && mMovementStates._1 && !mMovementStates.jumping) {
        _10.turning = false;
        clear2DStick();
        _66C = 0;
    }

    _63C.set(1.0f, 0.0f, 0.0f);
    _648.set(0.0f, 1.0f, 0.0f);
    _654.set(0.0f, 0.0f, -1.0f);
    PSMTXMultVecSR(_F4, &_63C, &_63C);
    PSMTXMultVecSR(_F4, &_654, &_654);
    PSMTXMultVecSR(_F4, &_648, &_648);

    _6A0 = _654;
    MR::vecKillElement(_368, _6A0, &_660);
    if (MR::normalizeOrZero(&_660)) {
        _660 = _368;
    }

    const f32 upDot = _660.dot(_648);
    if (!MR::isInRange(upDot, -0.707f, 0.707f)) {
        set2Dmode(upDot >= 0.0f);
    }

    TVec3f stick;
    stick.set(rStickX, rStickY, 0.0f);
    MR::normalizeOrZero(&stick);

    if (_66C) {
        if (MR::isNearZero(mStickPos.z, 0.001f)) {
            _66C = 0;
            return;
        }

        if (MR::diffAngleAbs(stick, _670) > 0.3f) {
            _66C = 0;
            return;
        }

        const f32 angle = -MR::diffAngleSignedHorizontal(_67C, _660, _6A0);
        PSMTXMultVecSR(MR::tmpMtxRotZRad(angle), &stick, &stick);
        rStickX = stick.x;
        rStickY = stick.y;
        return;
    }

    if (MR::isNearZero(mStickPos.z, 0.001f)) {
        return;
    }

    TVec3f projected;
    projected.x = _660.dot(getCamDirX());
    projected.y = _660.dot(getCamDirY());
    projected.z = 0.0f;
    const f32 angle = MR::diffAngleAbs(stick, projected);
    if (angle < 0.5235988f || angle > 2.617994f) {
        rStickX = 0.0f;
        rStickY = 0.0f;
        mStickPos.z = 0.0f;
        return;
    }

    _670 = stick;
    _67C = _660;
    _66C = 1;
}

void Mario::calcDir2D(f32 stickX, f32 stickY, TVec3f* pOut) {
    TVec3f cameraX;
    TVec3f cameraY;
    MR::vecKillElement(getCamDirX(), _6A0, &cameraX);
    MR::vecKillElement(getCamDirY(), _6A0, &cameraY);
    MR::normalizeOrZero(&cameraX);
    MR::normalizeOrZero(&cameraY);

    TVec3f horizontal(cameraX * stickX);
    pOut->set(horizontal);

    TVec3f vertical(cameraY * stickY);
    pOut->add(vertical);
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
