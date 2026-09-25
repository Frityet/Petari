#include "resource/TextEncoding.hpp"
#include "Game/Player/MarioClimb.hpp"
#include "Game/Player/MarioActor.hpp"

void Mario::connectToClimb() {
    _790 = -getShadowNorm();
    changeStatus(mClimb);
}

MarioClimb::MarioClimb(MarioActor* pActor) : MarioState(pActor, MarioStatus_Climb), mTimer() {
}

bool MarioClimb::update() {
    if (mTimer < 15 && checkTrgA()) {
        getPlayer()->tryJump();
        return false;
    }

    if (mTimer != 0) {
        mTimer--;
    }

    addVelocity(getFrontVec(), 6.0f);

    if (mTimer == 0) {
        return false;
    }

    return true;
}

bool MarioClimb::start() {
    changeAnimation(CP932("匍匐前進"), CP932("匍匐前進"));

    if (mActor->_468 != 0) {
        changeAnimationUpper(CP932("ひろいウエイト"), nullptr);
    }

    mTimer = 15;
    return true;
}

bool MarioClimb::close() {
    getPlayer()->mWalkSpeed = getStickP();

    if (getPlayer()->getMovementStates()._1) {
        stopAnimation(CP932("匍匐前進"), CP932("基本"));
    } else {
        stopAnimation(CP932("匍匐前進"), CP932("落下"));
        getPlayer()->set3BC(8);
    }

    return true;
}
