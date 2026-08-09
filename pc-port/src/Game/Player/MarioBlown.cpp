#include "Game/Player/MarioBlown.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/MathUtil.hpp"

bool Mario::blown(const TVec3f& rVec) {
    if (getCurrentStatus() == MarioStatus_Blown) {
        return false;
    }

    if (getCurrentStatus() == MarioStatus_Damage) {
        return false;
    }

    if (getCurrentStatus() == MarioStatus_Recovery) {
        return false;
    }

    stopWalk();
    MR::vecKillElement(rVec, mBlown->mActor->_240, &mBlown->_18);
    mMovementStates._2C = true;
    _430 = 3;
    mMovementStates._A = false;
    return true;
}

MarioBlown::MarioBlown(MarioActor* pActor) : MarioState(pActor, MarioStatus_Blown) {
    _12 = 0;
    _14 = 0;
    _18.set(0.0f, 0.0f, 0.0f);
    _24 = 0;
    _25 = 0;
}

bool MarioBlown::close() {
    if (!_24) {
        getPlayer()->stopJump();
    }

    stopAnimation("壁ヒット", static_cast<const char*>(nullptr));

    if (_25) {
        stopAnimation("壁ヒット着地", "基本");
    }

    mActor->setBlendMtxTimer(6);
    getPlayer()->unlockGroundCheck(this);

    if (!_24) {
        getPlayer()->mMovementStates._36 = false;
    }

    return true;
}

bool MarioBlown::start() {
    _12 = 0;
    _14 = 0;
    _24 = false;
    _25 = false;

    changeAnimation("壁ヒット", "基本");
    playSound("声壁体当たり", -1);
    playSound("壁衝突", -1);
    playEffectTrans("壁ヒット", getPlayer()->getWallPos());

    getPlayer()->mMovementStates._1 = false;
    getPlayer()->mMovementStates.jumping = true;
    getPlayer()->mMovementStates._B = false;

    TVec3f jump = -mActor->_240 * mActor->mConst->getTable()->mJumpHeightBlown;
    _18 += jump;
    getPlayer()->setJumpVec(_18);
    addVelocity(_18);
    getPlayer()->lockGroundCheck(this, true);
    return true;
}

bool MarioBlown::update() {
    ++_12;

    if (getPlayerMode() == 6) {
        return false;
    }

    switch (_14) {
    case 0:
    case 2: {
        addVelocity(_18);
        _18 += mActor->_240 * mActor->mConst->getTable()->mGravityBlown;

        if (_12 > 120) {
            changeAnimation("中ダメージ空中", static_cast<const char*>(nullptr));
        }

        if (_12 > 60) {
            TVec3f lastMove;
            f32 gravityMove = MR::vecKillElement(mActor->getLastMove(), mActor->_240, &lastMove);
            if (MR::isNearZero(gravityMove, 0.001f)) {
                return false;
            }
        }

        if (!getPlayer()->mMovementStates._1 && !MR::isNearZero(getPlayer()->mVerticalSpeed, 0.001f)) {
            break;
        }

        getPlayer()->mMovementStates.jumping = false;
        getPlayer()->mMovementStates._D = true;

        if (getPlayer()->damagePolygonCheck(getPlayer()->getGroundPolygon())) {
            getPlayer()->_1C._16 = true;
            _24 = true;
            return false;
        }

        if (_14 != 2 || _12 >= 3) {
            playSound("吹っ飛び倒れ", -1);
            changeAnimation("壁ヒット着地", static_cast<const char*>(nullptr));
            playEffect("共通壁ヒット着地");
            MR::vecKillElement(_18, mActor->_240, &_18);
        }

        _14 = 1;
        _12 = 0;
        break;
    }

    case 1:
        if (!getPlayer()->mMovementStates._1 && !MR::isNearZero(getPlayer()->mVerticalSpeed, 0.001f)) {
            _25 = true;
            return false;
        }

        addVelocity(_18);
        _18.x *= 0.95f;
        _18.y *= 0.95f;
        _18.z *= 0.95f;

        if (!isAnimationRun("壁ヒット着地")) {
            return false;
        }

        if (_12 > 15 && checkTrgA()) {
            getPlayer()->stopJump();
            getPlayer()->tryJump();
            _24 = true;
            return false;
        }
        break;
    }

    f32 gravitySpeed = MR::vecKillElement(_18, mActor->_240, &_18);
    if (_18.length() > 10.0f) {
        _18.x *= 0.5f;
        _18.y *= 0.5f;
        _18.z *= 0.5f;
    }

    f32 gravityAccel;
    if (gravitySpeed < 0.0f) {
        gravityAccel = 0.0f;
    }
    else if (gravitySpeed > 40.0f) {
        gravityAccel = 40.0f;
    }
    else {
        gravityAccel = gravitySpeed;
    }

    _18 += mActor->_240 * gravityAccel;
    getPlayer()->setJumpVec(_18);
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
