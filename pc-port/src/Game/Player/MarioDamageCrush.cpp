#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioCrush.hpp"

bool Mario::requestCrush() {
    _10._18 = true;
    return true;
}

bool Mario::tryCrush() {
    if (mMovementStates._1F) {
        return false;
    }

    if (isInvincible()) {
        return false;
    }

    if (mMovementStates.jumping || !mMovementStates._1) {
        setTrans(mShadowPos, nullptr);
    }

    if (getCurrentStatus() == MarioStatus_Crush) {
        return false;
    }

    mActor->damageDropThrowMemoSensor();
    mActor->resetPlayerModeOnDamage();
    stopJump();
    stopWalk();
    changeStatus(mCrush);
    return true;
}

MarioCrush::MarioCrush(MarioActor* pActor) : MarioState(pActor, MarioStatus_Crush), _12(0), _14(0) {
}

bool MarioCrush::close() {
    getPlayer()->mMovementStates._3C = true;
    mActor->mScale.set(1.0f, 1.0f, 1.0f);
    stopAnimation("しびれ", static_cast< const char* >(nullptr));
    Mario* pPlayer = getPlayer();
    pPlayer->_41E = 120;
    return true;
}

bool MarioCrush::start() {
    changeAnimationNonStop("しびれ");
    startPadVib("マリオ[しびれ]");
    playSound("声しびれ", -1);
    getPlayer()->mMovementStates._3C = true;
    mActor->mScale.set(1.0f, 0.2f, 1.0f);
    startPadVib(3);
    mActor->decLifeLarge();

    if (mActor->mHealth == 0) {
        mActor->forceGameOver();
    }

    _14 = 180;
    _12 = 0;
    return true;
}

bool MarioCrush::update() {
    if (_14 != 0) {
        _14--;
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
            changeAnimation("しびれ回復", static_cast< const char* >(nullptr));
        }
    }

    if (_12 != 0 && (mActor->isRequestRush() || checkTrgA())) {
        stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));

        if (checkTrgA()) {
            getPlayer()->tryJump();
        }

        return false;
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

extern const char lbl_805C64D8[0x48] = "後方小ダメージ\0"
                                       "前方小ダメージ\0"
                                       "ノーダメージ\0"
                                       "声小ダメージ\0"
                                       "ダメージ\0"
                                       "基本";
