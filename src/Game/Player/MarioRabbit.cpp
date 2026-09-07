#include "Game/Player/MarioRabbit.hpp"

#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

void Mario::startRabbitMode() {
    if (!isStatusActive(0x17)) {
        changeStatus(mRabbit);
    }
}

void Mario::endRabbitMode() {
    if (isStatusActive(0x17)) {
        closeStatus(mRabbit);
    }
}

MarioRabbit::MarioRabbit(MarioActor* actor) : MarioState(actor, 0x17) {
    mVerticalSpeed = 0.0f;
    mMoveVelocity.z = 0.0f;
    mMoveVelocity.y = 0.0f;
    mMoveVelocity.x = 0.0f;
    _24 = false;
    mIsWallJump = false;
    mIsForceJump = false;
    mDidImpact = false;
    mIsHighJump = false;
    mJumpAnimationIndex = 0;
    mJumpRequestTimer = 0;
    mTurnTimer = 0;
    PSMTXIdentity(mJointMtx.toMtxPtr());
    mPrevFrontVec.set(1.0f, 0.0f, 0.0f);
    mPlayLandingSound = false;
}

bool MarioRabbit::start() {
    _24 = false;
    mIsHighJump = false;
    mIsWallJump = false;
    mJumpAnimationIndex = 0;
    mJumpRequestTimer = 0;

    Mario* player = getPlayer();
    MR::vecKillElement(player->mJumpVec, getAirGravityVec(), &mMoveVelocity);
    mPrevFrontVec = getFrontVec();

    if (mIsForceJump || getPlayer()->mMovementStates.jumping) {
        mPlayLandingSound = true;
        return true;
    }

    stopAnimationUpper(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));

    mVerticalSpeed = -mActor->mConst->getTable()->mRabbitFirstJump;
    mIsForceJump = false;
    mTurnTimer = 0;

    if (getPlayer()->mDrawStates._10) {
        mVerticalSpeed *= 0.25f;
        const MarioConstTable* wallConstants = mActor->mConst->getTable();
        mMoveVelocity = getPlayer()->getWallNorm() * wallConstants->mRabbitFirstJump * 0.5f;
        const TVec3f& wallNorm = getPlayer()->getWallNorm();
        getPlayer()->setFrontVecKeepUp(wallNorm);
        mIsWallJump = true;
        mTurnTimer = 60;
    }

    impact();
    return true;
}

void MarioRabbit::hop() {
    if (mVerticalSpeed < 0.0f) {
        mVerticalSpeed = 0.5f * -mActor->mConst->getTable()->mRabbitFirstJump;
    }
    else {
        mVerticalSpeed = 0.3f * -mActor->mConst->getTable()->mRabbitFirstJump;
    }

    mJumpAnimationIndex = 0;
}

void MarioRabbit::forceJump() {
    mIsForceJump = true;

    if (getPlayer()->_430 != 12) {
        mJumpAnimationIndex = 1;
    }
}

void MarioRabbit::impact() {
    if (!getPlayer()->mDrawStates._10) {
        if (mMoveVelocity.length() > 2.0f * mActor->mConst->getTable()->mRabbitMoveSpeed) {
            mMoveVelocity.setLength(2.0f * mActor->mConst->getTable()->mRabbitMoveSpeed);
        }

        mMoveVelocity.setLength(0.5f * mMoveVelocity.length());
    }

    getPlayer()->stopJump();
    getPlayer()->mMovementStates._1 = false;
    getPlayer()->mMovementStates.jumping = true;
    getPlayer()->initJumpParam();
    getPlayer()->_42A = 0;
    getPlayer()->_430 = 0;
    getPlayer()->mMovementStates._22 = false;
    getPlayer()->mMovementStates._2B = false;

    if (mIsHighJump) {
        switch (mJumpAnimationIndex) {
        case 0:
            changeAnimationNonStop("ホッパーハイジャンプA");
            break;
        case 1:
            changeAnimationNonStop("ホッパーハイジャンプB");
            break;
        }

        mJumpAnimationIndex = 1 - mJumpAnimationIndex;
    }
    else if (!MR::isNearZero(getStickP())) {
        switch (mJumpAnimationIndex) {
        case 0:
            changeAnimationNonStop("ホッパージャンプA");
            break;
        case 1:
            changeAnimationNonStop("ホッパージャンプB");
            break;
        }

        mJumpAnimationIndex = 1 - mJumpAnimationIndex;
    }
    else {
        switch (mJumpAnimationIndex) {
        case 0:
            changeAnimationNonStop("ホッパー移動A");
            break;
        case 1:
            changeAnimationNonStop("ホッパー移動B");
            break;
        }
    }

    mDidImpact = true;
}

bool MarioRabbit::update() {
    if (getPlayer()->mMorphResetTimer != 0) {
        return false;
    }

    getPlayer()->mMovementStates._30 = false;
    getPlayer()->checkWallStick();

    if (mJumpRequestTimer != 0) {
        mJumpRequestTimer--;
    }

    if (!getPlayer()->mMovementStates.jumping && !getPlayer()->mMovementStates._1) {
        getPlayer()->mMovementStates._1 = true;
    }

    if (getPlayer()->mMovementStates._1) {
        if (mPlayLandingSound) {
            playSound("ホッパー跳ね返り", -1);
            mPlayLandingSound = false;
        }

        if (mDidImpact || isAnimationRun("ホッパー壁ジャンプ") || isAnimationRun("ホッパーヒップドロップ")) {
            stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
            switch (mJumpAnimationIndex) {
            case 0:
                changeAnimation("ホッパージャンプA", static_cast< const char* >(nullptr));
                break;
            case 1:
                changeAnimation("ホッパージャンプB", static_cast< const char* >(nullptr));
                break;
            }

            mIsHighJump = false;
            mDidImpact = false;

            if (!getPlayer()->mMovementStates._B) {
                getPlayer()->mMovementStates.jumping = false;
            }

            startPadVib(1);
            playEffect("共通着地普通");

            if (!mIsHighJump) {
                playSound("ホッパー跳ね返り", -1);
            }
        }

        if (getPlayer()->_3CE < mActor->mConst->getTable()->mHopperLandingTime) {
            if (mActor->isRequestJump() || mJumpRequestTimer != 0) {
                mIsHighJump = true;
                playEffect("共通ハイジャンプ");
                startPadVib("マリオ[ホッパーため]");
                switch (mJumpAnimationIndex) {
                case 0:
                    changeAnimationNonStop("ホッパーハイジャンプA");
                    break;
                case 1:
                    changeAnimationNonStop("ホッパーハイジャンプB");
                    break;
                }
            }

            return true;
        }

        if (mIsHighJump) {
            if (getPlayer()->_3CE < mActor->mConst->getTable()->mRabbitChargeTime2) {
                playSound("ホッパージャンプ溜め", -1);
                return true;
            }

            mVerticalSpeed = -mActor->mConst->getTable()->mRabbitFirstJump2;
            playSound("声物ジャンプ", -1);
            playSound("ホッパージャンプ", -1);
        }
        else {
            mVerticalSpeed = -mActor->mConst->getTable()->mRabbitFirstJump;
        }

        impact();
        mIsForceJump = false;
    }
    else {
        if (mIsForceJump) {
            getPlayer()->procJump(false);
            return true;
        }

        if (mActor->isRequestHipDrop() && getPlayer()->jumpToHipDrop()) {
            mJumpAnimationIndex = 0;
        }

        if (mActor->isRequestJump()) {
            mJumpRequestTimer = 3;
        }
    }

    TVec3f jumpVelocity = getAirGravityVec() * mVerticalSpeed;
    getPlayer()->mJumpVec = jumpVelocity;
    addVelocity(getAirGravityVec(), mVerticalSpeed);

    if (mIsHighJump) {
        if (mVerticalSpeed < 0.0f) {
            mVerticalSpeed += mActor->mConst->getTable()->mRabbitGravityRise2;
        }
        else {
            mVerticalSpeed += mActor->mConst->getTable()->mRabbitGravityDrop2;
        }
    }
    else {
        if (mVerticalSpeed < 0.0f) {
            mVerticalSpeed += mActor->mConst->getTable()->mRabbitGravityRise;
        }
        else {
            mVerticalSpeed += mActor->mConst->getTable()->mRabbitGravityDrop;
        }
    }

    if (mVerticalSpeed > 50.0f) {
        mVerticalSpeed = 50.0f;
    }

    if (getStickP() != 0.0f) {
        if (mTurnTimer != 0) {
            mTurnTimer--;
        }
        else {
            const TVec3f& worldPadDir = getWorldPadDir();

            if (mIsHighJump) {
                getPlayer()->setFrontVecKeepUp(worldPadDir, mActor->mConst->getTable()->mRabbitTurnRatio2);

                if (mVerticalSpeed < 0.0f) {
                    mMoveVelocity += getFrontVec() * mActor->mConst->getTable()->mRabbitMoveAcc2;
                }
                else {
                    mMoveVelocity += getFrontVec() * mActor->mConst->getTable()->mRabbitMoveAcc3;
                }
            }
            else {
                getPlayer()->setFrontVecKeepUp(worldPadDir, mActor->mConst->getTable()->mRabbitTurnRatio);
                mMoveVelocity += getFrontVec() * mActor->mConst->getTable()->mRabbitMoveAcc;
            }
        }
    }

    if (!mIsWallJump && mMoveVelocity.length() > mActor->mConst->getTable()->mRabbitMoveSpeed) {
        mMoveVelocity.setLength(mActor->mConst->getTable()->mRabbitMoveSpeed);
    }

    addVelocity(mMoveVelocity);

    f32 frontAngle = MR::diffAngleAbsHorizontal(getFrontVec(), mPrevFrontVec, getAirGravityVec());
    TVec3f cross;
    cross.cross(getFrontVec(), mPrevFrontVec);

    if (cross.dot(getAirGravityVec()) < 0.0f) {
        frontAngle = -frontAngle;
    }

    PSMTXCopy(MR::tmpMtxRotXRad(frontAngle), mJointMtx.toMtxPtr());

    if (__fabsf(frontAngle) >= 1.0471976f) {
        MR::vecBlendSphere(mPrevFrontVec, getFrontVec(), &mPrevFrontVec, 0.2f);
    }
    else {
        MR::vecBlendSphere(mPrevFrontVec, getFrontVec(), &mPrevFrontVec, 0.05f);
    }

    MR::normalizeOrZero(&mPrevFrontVec);

    switch (mJumpAnimationIndex) {
    case 0:
        setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, "Hip")), mJointMtx.toMtxPtr());
    case 1:
        setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, "Spine1")), mJointMtx.toMtxPtr());
        break;
    }

    return true;
}

bool MarioRabbit::close() {
    stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));

    if (getPlayer()->mMovementStates.jumping) {
        stopAnimation(static_cast< const char* >(nullptr), "落下");
    }
    else {
        stopAnimation(static_cast< const char* >(nullptr), "基本");
    }

    setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, "Hip")), nullptr);
    setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, "Spine1")), nullptr);
    return true;
}

bool MarioRabbit::notice() {
    return true;
}
