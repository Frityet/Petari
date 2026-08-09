#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "revolution/mtx.h"

static const f32 sZero = 0.0f;
static const f32 sOne = 1.0f;

void Mario::set25Dmode(const AreaObj* pArea) {
    TVec3f rotate;
    MR::calcCubeRotate(pArea, &rotate);

    TVec3f axisZ(sZero, sZero, sOne);
    TVec3f axisY(sZero, sOne, sZero);
    Mtx mtx;
    MR::makeMtxTR(mtx, sZero, sZero, sZero, rotate.x, rotate.y, rotate.z);
    PSMTXMultVec(mtx, &axisZ, &axisZ);
    PSMTXMultVec(mtx, &axisY, &axisY);

    _6D0 = axisZ;
    _6DC = axisY;
    _6E8.cross(_6D0, _6DC);
    MR::normalize(&_6E8);
    mMovementStates._3A = true;
}

void Mario::update25Dmode() {
    TVec3f gravityAxis;
    MR::vecKillElement(-*getGravityVec(), _6D0, &gravityAxis);
    MR::normalizeOrZero(&gravityAxis);

    if (MR::isNearZero(gravityAxis)) {
        _6AC = -getGravityVec()->dot(_6D0) > sZero ? 4 : 5;
        return;
    }

    const f32 axisSide = gravityAxis.dot(_6E8);
    const f32 axisUp = gravityAxis.dot(_6DC);
    const f32 axisFront = gravityAxis.dot(_6D0);
    const f32 absSide = __fabsf(axisSide);
    const f32 absUp = __fabsf(axisUp);
    const f32 absFront = __fabsf(axisFront);

    u8 mode;
    if (absSide > absUp && absSide > absFront) {
        mode = axisSide > sZero ? 2 : 3;
    } else if (absUp > absSide && absUp > absFront) {
        if (axisUp > sZero) {
            mode = 0;
        } else {
            mode = 1;
        }
    } else {
        mode = axisFront > sZero ? 4 : 5;
    }

    _6AD = mode;

    if (mode == 2 || mode == 3) {
        f32 threshold = 0.965f;
        if (_6AC == 2 || _6AC == 3) {
            threshold = 0.9397f;
        }

        if (absSide < threshold) {
            if (absUp > absFront) {
                if (axisUp > sZero) {
                    mode = 0;
                } else {
                    mode = 1;
                }
            } else {
                return;
            }
        }
    }

    if (!isStickOn()) {
        _6AC = mode;
        mMovementStates._3B = false;
        return;
    }

    if (!mMovementStates._3B && mStickPos.z > 0.7f) {
        _6C8.set(mStickPos.x, mStickPos.y);
        _6AC = mode;
        mMovementStates._3B = true;
        return;
    }

    TVec2f stick(mStickPos.x, mStickPos.y);
    if (MR::diffAngleAbs(_6C8, stick) >= 0.7853982f) {
        _6AC = mode;
        _6C8.set(mStickPos.x, mStickPos.y);
    }
}

void Mario::updateAxisFromMode(u8 mode) {
    if (mMovementStates.jumping) {
        return;
    }

    TVec3f moveAxis;
    TVec3f upAxis;

    if (mode < 4) {
        TVec3f gravityAxis;
        TVec3f pipeUp;
        MR::vecKillElement(-*getGravityVec(), _6D0, &gravityAxis);
        MR::vecKillElement(_6DC, _6D0, &pipeUp);
        MR::normalizeOrZero(&gravityAxis);
        MR::normalizeOrZero(&pipeUp);

        const f32 parallel = gravityAxis.dot(pipeUp);
        if (__fabsf(parallel) > 0.99f) {
            switch (mode) {
            case 0:
                moveAxis = _6E8;
                upAxis = _6D0;
                if (parallel < sZero) {
                    moveAxis = -moveAxis;
                }
                break;
            case 1:
                moveAxis = _6E8;
                upAxis = _6D0;
                if (parallel > sZero) {
                    moveAxis = -moveAxis;
                }
                break;
            case 2:
                moveAxis = _6D0;
                upAxis = -_6E8;
                if (parallel < sZero) {
                    moveAxis = -moveAxis;
                    upAxis = -upAxis;
                }
                break;
            case 3:
                moveAxis = -_6D0;
                upAxis = _6E8;
                if (parallel < sZero) {
                    moveAxis = -moveAxis;
                    upAxis = -upAxis;
                }
                break;
            }
        } else {
            TVec3f sideAxis;
            sideAxis.cross(gravityAxis, pipeUp);
            MR::normalize(&sideAxis);
            if (sideAxis.dot(_6D0) < sZero) {
                sideAxis = -sideAxis;
            }

            TVec3f frontAxis;
            frontAxis.cross(sideAxis, gravityAxis);
            MR::normalize(&frontAxis);

            switch (mode) {
            case 0:
                moveAxis = frontAxis;
                upAxis = sideAxis;
                break;
            case 1:
                moveAxis = -frontAxis;
                upAxis = sideAxis;
                break;
            case 2:
                moveAxis = sideAxis;
                upAxis = -frontAxis;
                break;
            case 3:
                moveAxis = -sideAxis;
                upAxis = frontAxis;
                break;
            }
        }
    }

    _6B0 = moveAxis;
    _6BC = -upAxis;
}

void Mario::calcMoveDir25D(f32 stickX, f32 stickY, TVec3f* pOut) {
    *pOut = _6B0 * stickX - _6BC * stickY;
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
