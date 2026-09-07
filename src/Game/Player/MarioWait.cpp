#include "Game/Player/MarioWait.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/MathUtil.hpp"

void MarioAnimator::controlWaitAnimation() {
    if (_78 != 0) {
        --_78;
        return;
    }

    Mario* pPlayer = getPlayer();
    TVec3f side;
    side.cross(getFrontVec(), pPlayer->_368);
    MR::normalizeOrZero(&side);
    if (MR::isNearZero(side, 0.001f)) {
        return;
    }

    TVec3f front;
    front.cross(getPlayer()->_368, side);
    MR::normalizeOrZero(&front);

    f32 sideAngle = side.dot(getPlayer()->getAirGravityVec());
    f32 frontAngle = front.dot(getPlayer()->getAirGravityVec());
    f32 blendWeight[4];
    blendWeight[0] = 0.0f;
    blendWeight[1] = 0.0f;
    blendWeight[2] = 0.0f;
    blendWeight[3] = 0.0f;
    f32 sideOffset = 0.0f;
    f32 frontOffset = 0.0f;

    if (_16 == 1) {
        sideOffset = 0.1f;
    } else if (_16 == 2) {
        frontOffset = 0.1f;
    }

    if (sideOffset + __fabsf(sideAngle) > frontOffset + __fabsf(frontAngle)) {
        f32 blend = 1.5707964f - marioAcos(__fabsf(sideAngle));
        if (blend >= 0.7853982f) {
            blend = 0.7853982f;
        }

        blend /= 0.7853982f;
        blendWeight[2] = blend;
        blendWeight[3] = 1.0f - blend;

        if (sideAngle < 0.0f) {
            mXanimePlayer->changeTrackAnimation(2, "坂右ウエイト");
        } else {
            mXanimePlayer->changeTrackAnimation(2, "坂左ウエイト");
        }

        if (getPlayer()->_10._F) {
            forceSetBlendWeight(blendWeight);
            getPlayer()->_10._F = false;
        } else {
            setBlendWeight(blendWeight, mActor->mConst->getTable()->mSlopeAnimBlendRatio);
        }

        _16 = 1;
    } else {
        f32 blend = 1.5707964f - marioAcos(__fabsf(frontAngle));
        if (blend >= 0.7853982f) {
            blend = 0.7853982f;
        }

        blend /= 0.7853982f;
        blendWeight[2] = blend;
        blendWeight[3] = 1.0f - blend;

        if (frontAngle < 0.0f) {
            mXanimePlayer->changeTrackAnimation(2, "坂前ウエイト");
        } else {
            mXanimePlayer->changeTrackAnimation(2, "坂後ウエイト");
        }

        if (getPlayer()->_10._F) {
            forceSetBlendWeight(blendWeight);
            getPlayer()->_10._F = false;
        } else {
            setBlendWeight(blendWeight, mActor->mConst->getTable()->mSlopeAnimBlendRatio);
        }

        _16 = 2;
    }

    _78 = 4;
}

void MarioAnimator::stopWaitAnimation() {
    if (getPlayerMode() == 1 && getPlayer()->mWalkSpeed >= 1.5f) {
        return;
    }

    if (_78 != 0) {
        --_78;
        return;
    }

    if (isAnimationRun("基本")) {
        mXanimePlayer->changeTrackAnimation(2, "ラン");
    }
}

bool MarioWait::checkStart() {
    if (mActor->_EA4) {
        _16 = 0;
        return false;
    }

    if (getPlayer()->isStatusActive(MarioStatus_Wait)) {
        return false;
    }

    bool isActorBusy = false;
    if (mActor->_482 || mActor->_481) {
        isActorBusy = true;
    }

    if (isActorBusy) {
        _16 = 0;
        return false;
    }

    bool isInvalid = false;
    switch (getPlayer()->_960) {
    case 27:
    case 28:
        isInvalid = true;
        break;
    }

    if (getStickP() != 0.0f || checkLvlA() || checkLvlZ() || mActor->isRequestRush()) {
        isInvalid = true;
    } else if (getPlayer()->mMovementStates.jumping && getPlayerMode() != 6) {
        isInvalid = true;
    } else if (getPlayer()->isStatusActive(MarioStatus_Swim)) {
        isInvalid = true;
    } else if (getPlayer()->isStatusActive(MarioStatus_Hang)) {
        isInvalid = true;
    } else if (getPlayer()->isStatusActive(MarioStatus_Sukekiyo)) {
        isInvalid = true;
    } else if (getPlayer()->isStatusActive(MarioStatus_Bury)) {
        isInvalid = true;
    } else if (getPlayerMode() == 5 || isStatusActiveID(MarioStatus_Foo)) {
        isInvalid = true;
    } else if (getPlayer()->isStatusActive(MarioStatus_FpView)) {
        isInvalid = true;
    } else if (getPlayer()->isStatusActive(MarioStatus_Magic)) {
        isInvalid = true;
    } else if (getPlayer()->isStatusActive(MarioStatus_Talk)) {
        isInvalid = true;
    }

    if (mActor->mBeeWallWalk) {
        isInvalid = true;
    }

    if (getPlayer()->_1C._5) {
        isInvalid = true;
    }

    if (getPlayer()->mMovementStates._23) {
        isInvalid = true;
    }

    if (getPlayer()->mMovementStates._A) {
        isInvalid = true;
    }

    if (getPlayer()->mDrawStates.mIsUnderwater) {
        isInvalid = true;
    }

    if (!MR::isNearZero(getPlayer()->_184, 0.001f)) {
        isInvalid = true;
    }

    if (MR::isNearZero(getPlayer()->mWalkSpeed, 0.001f)) {
        ++_16;
    } else {
        isInvalid = true;
    }

    if (isInvalid) {
        _16 = 0;
        return false;
    }

    if (_16 == 1800) {
        if (MR::getAreaObj("NonSleepCube", getTrans()) != nullptr) {
            _12 = 1;
            return true;
        }

        _12 = 0;
        return true;
    }

    return false;
}

bool MarioWait::start() {
    switch (_12) {
    case 0:
        changeAnimation("特殊ウエイト1A", static_cast< const char* >(nullptr));
        break;
    case 1:
        changeAnimation("特殊ウエイト1B", static_cast< const char* >(nullptr));
        break;
    }

    _16 = 0;
    _14 = 0;
    return true;
}

bool MarioWait::update() {
    if (getStickP() != 0.0f || checkLvlA() || checkLvlZ() || mActor->isRequestJump2P()) {
        stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
        getPlayer()->mainMove();
        return false;
    }

    bool isInvalid = false;
    if (mActor->isRequestRush()) {
        isInvalid = true;
    }
    if (getPlayer()->isStatusActive(MarioStatus_FpView)) {
        isInvalid = true;
    }

    if (getPlayer()->_1C._5) {
        isInvalid = true;
    }

    if (getPlayerMode() != 6 && !getPlayer()->mMovementStates._1) {
        isInvalid = true;
    }

    if (!MR::isNearZero(getPlayer()->_184, 0.001f)) {
        isInvalid = true;
    }

    if (isInvalid) {
        stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
        return false;
    }

    ++_16;

    switch (_12) {
    case 0:
        switch (_14) {
        case 0:
            if (mActor->_468 == 0) {
                u16 waitTimer = 1800;
                if (isPlayerModeTeresa()) {
                    waitTimer = 210;
                }

                if (_16 == waitTimer) {
                    _16 = 0;
                    ++_14;
                    changeAnimation("戦闘ウエイト", static_cast< const char* >(nullptr));
                }
            }
            break;
        case 1:
            break;
        }
        break;
    }

    return true;
}

bool MarioWait::close() {
    stopSound("声あくび", 0);
    stopSound("声いびき１", 0);
    stopSound("声いびき２", 0);
    _16 = 0;
    return true;
}

bool Mario::isBlendWaitGround() const {
    return calcAngleD(_368) > 8.0f;
}

void Mario::checkSpecialWaitAnimation() {
    if (mWait->checkStart()) {
        changeStatus(mWait);
    }
}

void Mario::resetSleepTimer() {
    mWait->_16 = 0;

    if (isStatusActive(MarioStatus_Wait)) {
        closeStatus(mWait);
    }
}

MarioWait::MarioWait(MarioActor* pActor) : MarioState(pActor, MarioStatus_Wait), _12(0), _14(0), _16(0) {
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
