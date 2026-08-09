#include "Game/Player/MarioClimb.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"

void Mario::connectToClimb() {
    _790 = -getShadowNorm();
    changeStatus(mClimb);
}

MarioClimb::MarioClimb(MarioActor* pActor) : MarioState(pActor, MarioStatus_Climb), _12(0) {
}

bool MarioClimb::update() {
    if (_12 < 15 && checkTrgA()) {
        getPlayer()->tryJump();
        return false;
    }

    if (_12 != 0) {
        --_12;
    }

    addVelocity(getFrontVec(), 6.0f);

    if (_12 == 0) {
        return false;
    }

    return true;
}

bool MarioClimb::start() {
    changeAnimation("匍匐前進", "匍匐前進");

    if (mActor->_468 != 0) {
        changeAnimationUpper("ひろいウエイト", static_cast<const char*>(nullptr));
    }

    _12 = 15;
    return true;
}

bool MarioClimb::close() {
    getPlayer()->_278 = getStickP();

    if (getPlayer()->mMovementStates._1) {
        stopAnimation("匍匐前進", "基本");
    }
    else {
        stopAnimation("匍匐前進", "落下");
        Mario* pPlayer = getPlayer();
        pPlayer->_3BC = 8;
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
