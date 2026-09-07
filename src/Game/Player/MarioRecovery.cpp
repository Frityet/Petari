#include "Game/Player/MarioRecovery.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MathUtil.hpp"

bool Mario::doRecovery() {
    if (isStatusActive(MarioStatus_Foo)) {
        return false;
    }

    if (isStatusActive(MarioStatus_Recovery)) {
        return false;
    }

    if (isStatusActive(MarioStatus_Warp)) {
        return false;
    }

    if (mActor->_934) {
        return false;
    }

    if (mActor->_EA4) {
        return false;
    }

    cancelSquatMode();
    stopWalk();

    if (mRecovery->_12) {
        changeStatus(mRecovery);
        stopJump();
    } else {
        TVec3f normal;
        TVec3f recoveryPos(*getLastSafetyTrans(&normal));
        recoveryPos += normal * 160.0f;

        TVec3f recoveryDir = mPosition - recoveryPos;
        MR::normalizeOrZero(&recoveryDir);
        doPointWarpRecovery(recoveryPos, recoveryDir);
    }

    return true;
}

MarioRecovery::MarioRecovery(MarioActor* pActor) : MarioState(pActor, MarioStatus_Recovery) {
    _16 = 0;
    _1A = -1;
    _58.zero();
    _12 = false;
    _11 = false;
    _14 = 0;
    _18 = 0;
    _1C.zero();
    _28.zero();
    _34.zero();
    _40.zero();
    _4C.zero();
    _64.zero();
    _70.zero();
    _88 = 0;
    _7C = 0.0f;
    _80 = 0.0f;
    _84 = 0.0f;
}

bool MarioRecovery::calcFirstVector() {
    _40 = getTrans();

    MarioActor* actor = mActor;
    const TVec3f& gravity = getGravityVec();
    f32 gravitySpeed = MR::vecKillElement(actor->getLastMove(), gravity, &_1C);
    _28 = getGravityVec() * gravitySpeed;

    TVec3f direction = getTrans() - _34;
    direction.setLength(10.0f);
    _1C += direction;

    _14 = 60;
    _11 = false;

    _70 = getTrans() - _34;
    f32 distance = _70.length();
    _84 = _70.length();

    if (MR::normalizeOrZero(&_70)) {
        return false;
    }

    TVec3f horizontal;
    MR::vecKillElement(_70, getGravityVec(), &horizontal);

    if (_12) {
        const TVec3f& specialGravity = getGravityVec();
        TVec3f previous = _4C - _34;
        TVec3f specialHorizontal;
        MR::vecKillElement(previous, specialGravity, &specialHorizontal);
        horizontal = specialHorizontal;
    }

    if (MR::normalizeOrZero(&horizontal)) {
        return false;
    }

    PSVECCrossProduct(&getGravityVec(), &horizontal, &_64);
    if (MR::normalizeOrZero(&_64)) {
        return false;
    }

    if (_12) {
        TVec3f reverse = -_70;
        _7C = MR::diffAngleAbs(horizontal, reverse) + 3.1415927f;
    } else {
        _7C = 6.2831855f - MR::diffAngleAbs(horizontal, _70);
    }

    _18 = 200;
    f32 extraTime = (distance - 500.0f) / 20.0f;
    if (extraTime > 0.0f) {
        _18 += static_cast< s32 >(extraTime);
    }

    _88 = _18;
    _80 = _7C / _18;
    _7C = 0.0f;
    mActor->_F44 = false;

    TVec3f* actorPosition = &mActor->mPosition;
    TVec3f actorOffset = *actorPosition - getTrans();
    addTrans(actorOffset, "Module");
    update();

    _12 = false;
    return true;
}

void MarioRecovery::updateJump() {
    TMtx34f rotation;
    PSMTXRotAxisRad(rotation.toMtxPtr(), &_64, _7C);

    TVec3f rotated;
    PSMTXMultVec(rotation.toMtxPtr(), &_70, &rotated);

    if (_88 > 60 && _88 < 120) {
        if (_7C < 4.712389f) {
            _84 += -_84 / static_cast< s32 >(_88 - 60);
        }

        _7C += 0.5f * _80;
    } else {
        _7C += _80;
    }

    if (_88 != 0) {
        _88--;
    }

    f32 pathLength = _84;
    if (pathLength < 300.0f) {
        pathLength = 300.0f;
    }
    rotated.setLength(pathLength);

    TVec3f movement = _34 + rotated - getTrans();
    f32 movementRate = 25.0f / (25.0f + _88);
    movement = movement * movementRate;

    u32 elapsed = _18 - _88;
    if (elapsed < 60) {
        f32 inertiaRate = (1.0f - static_cast< f32 >(elapsed) / 60.0f) * 0.8f;
        movement += _1C * inertiaRate;
        movement += _28 * inertiaRate;
    }

    f32 gravityMovement = MR::vecKillElement(movement, getGravityVec(), &movement);
    if (movement.length() > 30.0f) {
        movement.setLength(30.0f);
    }

    movement += getGravityVec() * gravityMovement;
    addTrans(movement, "Module");
}

bool MarioRecovery::start() {
    _16 = 0;
    _1A = 0;
    _34 = *getPlayer()->getLastSafetyTrans(nullptr);

    changeAnimationNonStop("引き戻し");
    playSound("声慌て", -1);
    playEffect("引き戻し泡");

    if (_12) {
        _1A = 3;
        _16 = 180;
        mActor->_F44 = false;

        if (MR::diffAngleAbs(_34 - _4C, _58) <= 1.5707964f) {
            TVec3f recoveryDirection = _34 - _4C;
            if (MR::normalizeOrZero(&recoveryDirection)) {
                recoveryDirection.set(1.0f, 0.0f, 0.0f);
            }

            TVec3f side;
            PSVECCrossProduct(&_58, &recoveryDirection, &side);
            if (MR::normalizeOrZero(&side)) {
                side.set(0.0f, 0.0f, 1.0f);
            }

            PSVECCrossProduct(&side, &_58, &recoveryDirection);
            MR::normalizeOrZero(&recoveryDirection);
            _34 = _4C + _58 * 600.0f + recoveryDirection * 600.0f;
        }
    } else if (!calcFirstVector()) {
        return false;
    }

    MR::startGlobalEventCameraNoTarget("引き戻し", -1);
    return true;
}

bool MarioRecovery::update() {
    clearVelocity();

    switch (_1A) {
    case 0:
        _1A++;
        _16 = _18 + 60;
    case 1: {
        s32 soundLevel;
        if (_16 == 0) {
            soundLevel = 0;
        } else {
            soundLevel = _16 / 2;
            if (_16 > _18) {
                soundLevel = 100 - (_16 - _18);
            }
        }

        if (soundLevel < 0) {
            soundLevel = 0;
        }

        if (soundLevel > 100) {
            soundLevel = 100;
        }
        playSound("引き戻し基本", -1);
        playSound("引き戻し浮遊", soundLevel);
        getPlayer()->setFrontVecKeepUp(_1C, 0.1f);

        if (_14 != 0 && --_14 == 0) {
            mActor->_F44 = true;
        }

        if (_16 != 0) {
            _16--;
        }

        if (_16 == 0 || (_34 - getTrans()).length() < 20.0f) {
            addTrans(_34 - getTrans(), "Module");
            return false;
        }

        updateJump();

        bool landed = false;
        bool shadowIsClose = true;
        TVec3f shadowPos(getShadowPos());
        TVec3f shadowOffset = shadowPos - _34;
        TVec3f startOffset = _40 - _34;

        if (MR::isNearZero(shadowOffset, 0.001f)) {
            landed = true;
        }

        if (shadowOffset.length() >= 50.0f) {
            shadowIsClose = false;
        }

        MR::normalizeOrZero(&shadowOffset);
        MR::normalizeOrZero(&startOffset);

        if (shadowOffset.dot(startOffset) < 0.0f) {
            landed = true;
        }

        if (landed) {
            if (shadowIsClose && (shadowPos - getTrans()).length() > 300.0f) {
                break;
            }

            stopEffectForce("引き戻し泡");
            playEffect("引き戻し泡破裂");
            playSound("引き戻し泡破裂", -1);
            _1A++;
        }
        break;
    }
    case 2:
        getPlayer()->_10._2 = true;
        return false;
    case 3:
        if (_16 != 0) {
            _16--;
        }

        if (_16 == 0) {
            _1A = 0;
            calcFirstVector();
        } else {
            TVec3f horizontal;
            f32 planeDistance = MR::vecKillElement(_34 - getTrans(), _58, &horizontal);

            if (__fabsf(planeDistance) < 10.0f) {
                addTrans(_58 * planeDistance, "Module");
                _1A = 0;
                calcFirstVector();
            } else {
                f32 movement = MR::clamp(planeDistance, -10.0f, 10.0f);
                addTrans(_58 * movement, "Module");
            }

            playSound("引き戻し基本", -1);
        }
        break;
    }

    return true;
}

bool MarioRecovery::close() {
    MR::endGlobalEventCamera("引き戻し", -1, true);

    getPlayer()->mMovementStates._1 = true;
    getPlayer()->mMovementStates.jumping = false;
    getPlayer()->_420 = 16;

    stopEffectForce("引き戻し泡");
    playEffect("引き戻し泡破裂");
    stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
    changeAnimation(static_cast< const char* >(nullptr), "基本");

    mActor->_F44 = true;
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
