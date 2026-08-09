#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioFreeze.hpp"

bool Mario::doFreeze() {
    if (mMovementStates._1F) {
        return false;
    }

    if (isInvincible()) {
        return false;
    }

    if (getCurrentStatus() == MarioStatus_Freeze) {
        return false;
    }

    if (mFreeze->_1C != 0) {
        return false;
    }

    mActor->resetPlayerModeOnDamage();
    stopJump();
    stopWalk();
    mActor->damageDropThrowMemoSensor();
    cancelSquatMode();
    stopAnimationUpper(nullptr, nullptr);
    changeStatus(mFreeze);
    return true;
}

MarioFreeze::MarioFreeze(MarioActor* pActor)
    : MarioState(pActor, MarioStatus_Freeze), _11(0), _14(0.0f), _18(0), _1A(0), _1C(0) {
}

bool MarioFreeze::close() {
    _1C = 120;

    if (_11) {
        playSound("氷ダメージ終了", -1);
        mActor->hideFreezeModel();
    }

    return true;
}

bool MarioFreeze::notice() {
    if (mActor->mHealth == 0) {
        if (getNoticedStatus() == MarioStatus_Swim) {
            mActor->setB90(true);
            mActor->forceGameOver();
        }

        return true;
    }

    return false;
}

bool MarioFreeze::start() {
    changeAnimationNonStop("氷結");
    playSound("声氷ダメージ", -1);
    playSound("氷ダメージ", -1);
    playSound("ダメージ", -1);
    startPadVib(3);
    mActor->decLife(0);

    _1A = 180;
    _18 = 0;
    _14 = 0.0f;

    Mario* player = getPlayer();
    player->_74C = 0.0f;
    player->_750 = 0;
    player->_754 = 0;

    mActor->showFreezeModel();
    _11 = 1;
    return true;
}

bool MarioFreeze::update() {
    if (_1A != 0) {
        _1A--;

        if (_18 != 0) {
            if (getPlayer()->mMovementStates._1) {
                addVelocity(getFrontVec(), -1.0f);
            }
        } else if (_1A < 120 && mActor->mHealth != 0 && mActor->isRequestSpin()) {
            addVelocity(getFrontVec(), -10.0f);
            changeAnimation("地上ひねり", static_cast< const char* >(nullptr));
            playSound("声スピン", -1);
            playSound("スピンジャンプ", -1);
            playSound("氷ダメージ終了", -1);
            mActor->hideFreezeModel();
            _11 = 0;
            return false;
        }
    }

    if (_1A == 0) {
        if (_18 != 0) {
            if (mActor->mHealth != 0) {
                playSound("声氷ダメージ終了", -1);
                return false;
            }
        } else {
            _18 = 1;

            if (!getPlayer()->mMovementStates._1) {
                _1A = 10;
            } else {
                _1A = 20;
            }

            if (mActor->mHealth == 0) {
                if (!getPlayer()->mMovementStates._1) {
                    mActor->forceGameOverNonStop();
                } else {
                    mActor->forceGameOver();
                }
            } else if (getPlayer()->mMovementStates._1) {
                changeAnimation("氷結解除", static_cast< const char* >(nullptr));
                playSound("氷ダメージ終了", -1);
                mActor->hideFreezeModel();
                _11 = 0;
            }
        }
    }

    if (_1A < 150) {
        if (!getPlayer()->mMovementStates._1 || _14 < 0.0f) {
            TVec3f jumpVec = getGravityVec() * _14;
            getPlayer()->setJumpVec(jumpVec);

            TVec3f velocity = getGravityVec() * _14;
            addVelocity(velocity);

            _14 += 1.5f;
            if (_14 < 0.0f) {
                getPlayer()->mMovementStates._1 = false;
            }
        } else if (_14 > 0.0f) {
            if (_14 < 8.0f) {
                _14 = 0.0f;
            } else {
                _14 = -_14 * 0.4f;
                getPlayer()->mMovementStates._1 = false;

                TVec3f jumpVec = getGravityVec() * _14;
                getPlayer()->setJumpVec(jumpVec);
            }
        }
    }

    return true;
}

void Mario::setJumpVec(const TVec3f& rJumpVec) {
    mJumpVec = rJumpVec;
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
