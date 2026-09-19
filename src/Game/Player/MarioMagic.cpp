#include "compat/Cp932Literal.hpp"
#include "Game/Player/MarioMagic.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioModule.hpp"

void Mario::stopPunch() {
    if (isStatusActive(MarioStatus_Magic)) {
        closeStatus(mMagic);
    }

    MarioActor* actor = mActor;

    if (!actor->_944) {
        actor->_945 = 0;
        actor->_974 = 0;
    }

    actor->_944 = 0;
}

void Mario::startMagic() {
    if (!mMovementStates.jumping) {
        if (!mActor->_468) {
            if (!mMovementStates._23) {
                if (!isStatusActive(MarioStatus_Slider)) {
                    if (isSkatableFloor()) {
                        doSkate();
                    } else {
                        clearSlope();
                        changeStatus(mMagic);
                        stopAnimationUpper(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
                        _10._1 = 1;
                    }
                }
            }
        }
    }
}

MarioMagic::MarioMagic(MarioActor* pActor) : MarioState(pActor, MarioStatus_Magic) {
    _12 = 0;
}

bool MarioMagic::close() {
    stopEffect(CP932("スピンライト"));

    if (_12 < 0x1A) {
        playEffect(CP932("スピンライト消去"));
    }

    return true;
}

bool MarioMagic::start() {
    changeAnimation(CP932("地上ひねり"), static_cast< const char* >(nullptr));
    stopEffect(CP932("パンチブラー左"));
    stopEffect(CP932("パンチブラー右"));
    playEffect(CP932("共通地上スピン"));
    playSound(CP932("声スピン"));
    playSound(CP932("スピンジャンプ"));
    startPadVib(2);
    _12 = 0;
    return true;
}

bool MarioMagic::update() {
    if (mActor->isRequestJump()) {
        getPlayer()->tryJump();
        return false;
    } else if (!isAnimationRun(CP932("地上ひねり"))) {
        return false;
    }

    _12++;

    if (_12 == 25) {
        stopEffect(CP932("スピンライト"));
        playEffect(CP932("スピンライト消去"));
    }

    if (getPlayer()->mMovementStates.jumping) {
        getPlayer()->procJump(false);
    } else {
        getPlayer()->mainMove();
    }

    getPlayer()->updateWalkSpeed();
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
