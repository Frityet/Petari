#include "Game/Player/MarioRabbit.hpp"

#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

#define HOPPER_HIGH_JUMP_A "ホッパーハイジャンプA"
#define HOPPER_HIGH_JUMP_B "ホッパーハイジャンプB"
#define HOPPER_MOVE_A "ホッパー移動A"
#define HOPPER_MOVE_B "ホッパー移動B"
#define HOPPER_JUMP_A "ホッパージャンプA"
#define HOPPER_JUMP_B "ホッパージャンプB"
#define HOPPER_BOUND "ホッパー跳ね返り"
#define HOPPER_WALL_JUMP "ホッパー壁ジャンプ"
#define HOPPER_HIP_DROP "ホッパーヒップドロップ"
#define COMMON_LAND "共通着地普通"
#define COMMON_HIGH_JUMP "共通ハイジャンプ"
#define HOPPER_CHARGE_VIB "マリオ[ホッパーため]"
#define HOPPER_CHARGE_SOUND "ホッパージャンプ溜め"
#define VOICE_OBJECT_JUMP "声物ジャンプ"
#define HOPPER_JUMP_SOUND "ホッパージャンプ"
#define JOINT_SPINE1 "Spine1"
#define FALL_ANIM "落下"
#define BASIC_ANIM "基本"
#define JOINT_HIP "Hip"

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
    mMoveVelocity.zero();
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

    if (mIsForceJump || player->mMovementStates.jumping) {
        mPlayLandingSound = true;
        return true;
    }

    stopAnimationUpper(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));

    const MarioConstTable* constants = mActor->mConst->getTable();
    mIsForceJump = false;
    mTurnTimer = 0;
    mVerticalSpeed = -constants->mRabbitFirstJump;

    if (player->mDrawStates._10) {
        mVerticalSpeed *= 0.25f;
        mMoveVelocity = player->getWallNorm() * constants->mRabbitFirstJump * 0.5f;
        player->setFrontVecKeepUp(player->getWallNorm());
        mIsWallJump = true;
        mTurnTimer = 60;
    }

    impact();
    return true;
}

void MarioRabbit::hop() {
    const f32 firstJump = -mActor->mConst->getTable()->mRabbitFirstJump;

    if (mVerticalSpeed < 0.0f) {
        mVerticalSpeed = 0.5f * firstJump;
    }
    else {
        mVerticalSpeed = 0.3f * firstJump;
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
    Mario* player = getPlayer();
    const MarioConstTable* constants = mActor->mConst->getTable();

    if (!player->mDrawStates._10) {
        const f32 maxSpeed = 2.0f * constants->mRabbitMoveSpeed;
        if (mMoveVelocity.length() > maxSpeed) {
            mMoveVelocity.setLength(maxSpeed);
        }

        mMoveVelocity.setLength(0.5f * mMoveVelocity.length());
    }

    player->stopJump();
    player->mMovementStates._1 = false;
    player->mMovementStates.jumping = true;
    player->initJumpParam();
    player->_42A = 0;
    player->_430 = 0;
    player->mMovementStates._22 = false;
    player->mMovementStates._2B = false;

    if (mIsHighJump) {
        switch (mJumpAnimationIndex) {
        case 0:
            changeAnimationNonStop(HOPPER_HIGH_JUMP_A);
            break;
        case 1:
            changeAnimationNonStop(HOPPER_HIGH_JUMP_B);
            break;
        }

        mJumpAnimationIndex = 1 - mJumpAnimationIndex;
    }
    else if (!MR::isNearZero(getStickP())) {
        switch (mJumpAnimationIndex) {
        case 0:
            changeAnimationNonStop(HOPPER_JUMP_A);
            break;
        case 1:
            changeAnimationNonStop(HOPPER_JUMP_B);
            break;
        }

        mJumpAnimationIndex = 1 - mJumpAnimationIndex;
    }
    else {
        switch (mJumpAnimationIndex) {
        case 0:
            changeAnimationNonStop(HOPPER_MOVE_A);
            break;
        case 1:
            changeAnimationNonStop(HOPPER_MOVE_B);
            break;
        }
    }

    mDidImpact = true;
}

bool MarioRabbit::update() {
    Mario* player = getPlayer();

    if (player->mMorphResetTimer != 0) {
        return false;
    }

    player->mMovementStates._30 = false;
    player->checkWallStick();

    if (mJumpRequestTimer != 0) {
        mJumpRequestTimer--;
    }

    if (!player->mMovementStates.jumping && !player->mMovementStates._1) {
        player->mMovementStates._1 = true;
    }

    if (player->mMovementStates._1) {
        if (mPlayLandingSound) {
            playSound(HOPPER_BOUND, -1);
            mPlayLandingSound = false;
        }

        if (mDidImpact || isAnimationRun(HOPPER_WALL_JUMP) || isAnimationRun(HOPPER_HIP_DROP)) {
            stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
            switch (mJumpAnimationIndex) {
            case 0:
                changeAnimation(HOPPER_JUMP_A, static_cast< const char* >(nullptr));
                break;
            case 1:
                changeAnimation(HOPPER_JUMP_B, static_cast< const char* >(nullptr));
                break;
            }

            mIsHighJump = false;
            mDidImpact = false;

            if (!player->mMovementStates._B) {
                player->mMovementStates.jumping = false;
            }

            startPadVib(1);
            playEffect(COMMON_LAND);

            if (!mIsHighJump) {
                playSound(HOPPER_BOUND, -1);
            }
        }

        const MarioConstTable* constants = mActor->mConst->getTable();
        if (player->_3CE < constants->mHopperLandingTime) {
            if (mActor->isRequestJump() || mJumpRequestTimer != 0) {
                mIsHighJump = true;
                playEffect(COMMON_HIGH_JUMP);
                startPadVib(HOPPER_CHARGE_VIB);
                switch (mJumpAnimationIndex) {
                case 0:
                    changeAnimationNonStop(HOPPER_HIGH_JUMP_A);
                    break;
                case 1:
                    changeAnimationNonStop(HOPPER_HIGH_JUMP_B);
                    break;
                }
            }

            return true;
        }

        if (mIsHighJump) {
            if (player->_3CE < constants->mRabbitChargeTime2) {
                playSound(HOPPER_CHARGE_SOUND, -1);
                return true;
            }

            mVerticalSpeed = -constants->mRabbitFirstJump2;
            playSound(VOICE_OBJECT_JUMP, -1);
            playSound(HOPPER_JUMP_SOUND, -1);
        }
        else {
            mVerticalSpeed = -constants->mRabbitFirstJump;
        }

        impact();
        mIsForceJump = false;
    }
    else {
        if (mIsForceJump) {
            player->procJump(false);
            return true;
        }

        if (mActor->isRequestHipDrop() && player->jumpToHipDrop()) {
            mJumpAnimationIndex = 0;
        }

        if (mActor->isRequestJump()) {
            mJumpRequestTimer = 3;
        }
    }

    TVec3f jumpVelocity = getAirGravityVec() * mVerticalSpeed;
    player->mJumpVec = jumpVelocity;
    addVelocity(getAirGravityVec(), mVerticalSpeed);

    const MarioConstTable* constants = mActor->mConst->getTable();
    if (mIsHighJump) {
        if (mVerticalSpeed < 0.0f) {
            mVerticalSpeed += constants->mRabbitGravityRise2;
        }
        else {
            mVerticalSpeed += constants->mRabbitGravityDrop2;
        }
    }
    else {
        if (mVerticalSpeed < 0.0f) {
            mVerticalSpeed += constants->mRabbitGravityRise;
        }
        else {
            mVerticalSpeed += constants->mRabbitGravityDrop;
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
                player->setFrontVecKeepUp(worldPadDir, constants->mRabbitTurnRatio2);

                if (mVerticalSpeed < 0.0f) {
                    mMoveVelocity += getFrontVec() * constants->mRabbitMoveAcc2;
                }
                else {
                    mMoveVelocity += getFrontVec() * constants->mRabbitMoveAcc3;
                }
            }
            else {
                player->setFrontVecKeepUp(worldPadDir, constants->mRabbitTurnRatio);
                mMoveVelocity += getFrontVec() * constants->mRabbitMoveAcc;
            }
        }
    }

    if (!mIsWallJump && mMoveVelocity.length() > constants->mRabbitMoveSpeed) {
        mMoveVelocity.setLength(constants->mRabbitMoveSpeed);
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
        setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, JOINT_HIP)), mJointMtx.toMtxPtr());
    case 1:
        setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, JOINT_SPINE1)), mJointMtx.toMtxPtr());
        break;
    }

    return true;
}

bool MarioRabbit::close() {
    stopAnimation(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));

    if (getPlayer()->mMovementStates.jumping) {
        stopAnimation(static_cast< const char* >(nullptr), FALL_ANIM);
    }
    else {
        stopAnimation(static_cast< const char* >(nullptr), BASIC_ANIM);
    }

    setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, JOINT_HIP)), nullptr);
    setJointGlobalMtx(static_cast< u8 >(MR::getJointIndex(mActor, JOINT_SPINE1)), nullptr);
    return true;
}

bool MarioRabbit::notice() {
    return true;
}
