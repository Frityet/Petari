#include "Game/Player/MarioFaint.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/MathUtil.hpp"

extern const char lbl_805C64D8[];

bool Mario::doFlipWeak(const TVec3f& rVec) {
    if (mMovementStates._1C) {
        return false;
    }

    mFaint->_24 = true;
    if (faint(rVec)) {
        if (isStatusActive(MarioStatus_Swim)) {
            mSwim->mDamageType = 1;
            mFaint->_24 = false;
        }

        return true;
    }

    mFaint->_24 = false;
    forceStopTornado();
    return false;
}

bool Mario::faint(const TVec3f& rVec) {
    _7C4 = rVec;

    if (!isEnableAddDamage()) {
        return false;
    }

    if (getCurrentStatus() == MarioStatus_Damage) {
        return false;
    }

    mFaint->setVec(rVec);
    stopWalk();
    forceStopTornado();
    mActor->damageDropThrowMemoSensor();

    if (getCurrentStatus() != MarioStatus_Faint) {
        mMovementStates._27 = true;
        return true;
    }

    return mMovementStates._27;
}

MarioFaint::MarioFaint(MarioActor* pActor) : MarioState(pActor, MarioStatus_Faint) {
    _12 = 0;
    _14 = 0;
    _16 = 0;
    _18.zero();
    _24 = 0;
    _25 = 0;
}

void MarioFaint::setVec(const TVec3f& rVec) {
    MR::vecKillElement(rVec, mActor->_240, &_18);
    _18.setLength(mActor->mConst->getTable()->mSlideDistFaint);
    _12 = 0;
    _16 = 0;

    if (getPlayer()->isStatusActive(MarioStatus_Hang)) {
        _18.zero();
    }
}

bool MarioFaint::update() {
    ++_12;

    switch (_16) {
    case 0:
        addVelocity(_18);
        _18.scale(mActor->mConst->getTable()->mFaintFriction1);

        if (_12 == mActor->mConst->getTable()->mFaintTimer1) {
            ++_16;
        }
        break;

    case 1:
        if (!getPlayer()->mMovementStates._1) {
            return false;
        }

        addVelocity(_18);
        _18.scale(mActor->mConst->getTable()->mFaintFriction2);

        if (checkTrgA()) {
            getPlayer()->tryJump();
            return false;
        }

        if (_12 == mActor->mConst->getTable()->mFaintTimer1 + mActor->mConst->getTable()->mFaintTimer2) {
            return false;
        }
        break;
    }

    return true;
}

bool MarioFaint::start() {
    const char* pString = lbl_805C64D8;
    _12 = 0;
    _16 = 0;
    getPlayer()->mMovementStates._B = false;
    getPlayer()->mMovementStates.jumping = false;

    if (_18.dot(getPlayer()->mFrontVec) > 0.0f) {
        getPlayer()->setFrontVecKeepUp(_18);
        changeAnimation(pString, static_cast<const char*>(nullptr));
    }
    else {
        getPlayer()->setFrontVecKeepUp(-_18);
        changeAnimation(pString + 0xF, static_cast<const char*>(nullptr));
    }

    if (_24) {
        changeAnimation(pString + 0x1E, static_cast<const char*>(nullptr));
    }

    playSound(pString + 0x2B, -1);
    playSound(pString + 0x38, -1);
    playEffect(pString + 0x38);
    startPadVib(2);
    addVelocity(_18);

    _25 = !_24;
    if (!_24) {
        mActor->decLife(0);
        mActor->resetPlayerModeOnDamage();

        if (mActor->mHealth == 0) {
            if (!getPlayer()->mMovementStates._1) {
                mActor->forceGameOverNonStop();
            }
            else {
                mActor->forceGameOver();
            }
        }
    }
    else {
        _24 = false;
        mActor->resetPlayerModeOnNoDamage();
    }

    return true;
}

bool MarioFaint::close() {
    const char* pString = lbl_805C64D8;

    if (getPlayer()->mMovementStates._1) {
        stopAnimation(pString, static_cast<const char*>(nullptr));
        stopAnimation(pString + 0xF, pString + 0x41);
    }

    if (_25) {
        _14 = 120;
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
