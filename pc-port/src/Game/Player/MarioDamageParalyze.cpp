#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioParalyze.hpp"

bool Mario::doParalyze() {
    if (mMovementStates._1F) {
        return false;
    }

    if (isInvincible()) {
        return false;
    }

    if (getCurrentStatus() == MarioStatus_Paralyze) {
        return false;
    }

    if (getCurrentStatus() == MarioStatus_FireDamage) {
        return false;
    }

    if (mMovementStates._1B) {
        return false;
    }

    if (getDamageAfterTimer() != 0) {
        return false;
    }

    if (isDamaging()) {
        return false;
    }

    mActor->damageDropThrowMemoSensor();
    mActor->resetPlayerModeOnDamage();
    stopJump();
    stopWalk();
    changeStatus(mParalyze);
    return true;
}

MarioParalyze::MarioParalyze(MarioActor* pActor) : MarioState(pActor, MarioStatus_Paralyze), _12(0), _14(0), _16(0), _18(false) {
}

bool MarioParalyze::close() {
    if (mActor->mHealth == 0) {
        if (!getPlayer()->mMovementStates._1) {
            mActor->forceGameOverNonStop();
        } else {
            mActor->forceGameOver();
        }

        mActor->changeGameOverAnimation();
    }

    stopAnimation("電気ダメージ", static_cast< const char* >(nullptr));
    stopEffect("ビリビリ");
    _16 = 120;

    if (!getPlayer()->isStatusActive(MarioStatus_Swim) && !getPlayer()->mMovementStates._1) {
        TVec3f jumpVec = getFrontVec() * -10.0f;
        getPlayer()->tryFreeJump(jumpVec, true);
    }

    return true;
}

bool MarioParalyze::start() {
    changeAnimationNonStop("電気ダメージ");
    playSound("ダメージ", -1);
    playSound("電気ダメージ", -1);
    playSound("声電気ダメージ", -1);
    playEffect("ビリビリ");
    startPadVib(3);

    if (!_18) {
        mActor->decLifeLarge();
    }

    _18 = false;
    _14 = 60;
    _12 = 0;
    return true;
}

bool MarioParalyze::update() {
    if (_14 != 0) {
        _14--;

        if (_12 != 0 && getPlayer()->mMovementStates._1) {
            addVelocity(getFrontVec(), -3.0f);
        }
    }

    if (_14 == 0) {
        if (_12 != 0) {
            return false;
        }

        _12 = 1;

        if (!getPlayer()->mMovementStates._1) {
            _14 = 10;
        } else {
            _14 = 30;
        }

        if (getPlayer()->mMovementStates._1) {
            changeAnimation("電気ダメージ終了", static_cast< const char* >(nullptr));
            playSound("声電気ダメージ終了", -1);
        }

        if (mActor->mHealth == 0) {
            if (!getPlayer()->mMovementStates._1) {
                mActor->forceGameOverNonStop();
            } else {
                mActor->forceGameOver();
            }

            mActor->changeGameOverAnimation();
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
