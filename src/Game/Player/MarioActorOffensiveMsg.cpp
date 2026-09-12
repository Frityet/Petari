#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioMessenger.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"

static f32 mSensorRadiusAttack = 120.0f;
static f32 mSensorRadiusTornadoAttack = 150.0f;
static f32 mSensorRadiusHipDropAttack = 80.0f;
static f32 mSensorRadiusClapCatch = 600.0f;
static f32 mSensorRadiusClapCatchSwim = 2000.0f;
static f32 mSensorRadiusTrampleAttack = 80.0f;
static f32 mSensorRadiusSpinStorm = 600.0f;
static f32 mSensorRadiusTornadoStorm = 1000.0f;
static f32 mSensorRadiusSpinPull = 450.0f;
static f32 mSensorRadiusSpinPullOnGround = 450.0f;

void MarioActor::attackOrPushSensor(HitSensor* pSensor, f32 distance) {
    f32 sensorRadius = pSensor->mRadius;
    mMario->checkOnimasu(pSensor);

    bool canPull = false;
    if (mMario->getMovementStates()._F) {
        if (distance < sensorRadius + mSensorRadiusTornadoAttack && tryTornadoAttack(pSensor)) {
            return;
        }
        if (distance < sensorRadius + mSensorRadiusTornadoStorm) {
            pSensor->receiveMessage(ACTMES_TORNADO_STORM_RANGE, getSensor("body"));
            if (mMario->isSwimming()) {
                if (distance < mSensorRadiusSpinPull) {
                    canPull = true;
                }
            } else if (distance < mSensorRadiusSpinPullOnGround) {
                canPull = true;
            }
        }
    } else if (_3E5 && _945 < 15) {
        if (distance < sensorRadius + mSensorRadiusSpinStorm) {
            if (selectAction("スピンアタック") < 4) {
                pSensor->receiveMessage(ACTMES_SPIN_STORM_RANGE, getSensor("body"));
            }
            if (mMario->isSwimming()) {
                if (distance < mSensorRadiusSpinPull) {
                    canPull = true;
                }
            } else if (distance < mSensorRadiusSpinPullOnGround) {
                canPull = true;
            }
        }
    } else if (isAnimationRun("ファイアスピン")) {
        if (distance < sensorRadius + mSensorRadiusSpinStorm) {
            pSensor->receiveMessage(ACTMES_SPIN_STORM_RANGE, getSensor("body"));
        }
    }

    if (canPull && _424 == nullptr) {
        tryTornadoPull(pSensor);
    }

    if (mMario->getMovementStates()._B && !mMario->getMovementStates()._1) {
        f32 jumpSpeed = mMario->mJumpVec.dot(getGravityVector());
        if (distance < sensorRadius + mSensorRadiusHipDropAttack + jumpSpeed) {
            if (tryHipDropAttack(pSensor)) {
                return;
            }
            if (tryGetItem(pSensor)) {
                return;
            }
            mMario->_10._27 = false;
        }
    }

    if (distance < sensorRadius + mSensorRadiusAttack) {
        if (_944 && tryPunchAirAfter(pSensor)) {
            return;
        }
        if (checkAndTryTrampleAttack(pSensor, distance, false)) {
            return;
        }

        bool isTouch = false;
        if (mPlayerMode == PlayerMode_Teresa) {
            TVec3f diff = _2A0 - pSensor->mPosition;
            f32 length = diff.length();
            if (length < pSensor->mRadius + mConst->getTable()->mTeresaBodyRadius) {
                if (selectTeresaThru(pSensor)) {
                    return;
                }
                if (tryGetItem(pSensor)) {
                    return;
                }
                diff.setLength(pSensor->mRadius + mConst->getTable()->mTeresaBodyRadius - length);
                if (!MR::isSensorAutoRush(pSensor)) {
                    mMario->doTeresaReflection(diff, false);
                    MR::sendArbitraryMsg(ACTMES_TERESA_PLAYER_TOUCH, pSensor, getSensor("body"));
                    touchSensor(pSensor);
                }
                if (MR::isSensorEnemy(pSensor)) {
                    setPlayerMode(PlayerMode_Normal, true);
                }
            }
        } else {
            f32 height = _4B4;
            f32 radius = 20.0f + _4B0;
            if (!mMario->isActiveTaskID(0x200)) {
                if (isJumping()) {
                    height += 10.0f;
                }
                if (mMario->mTargetWalkSpeedIndex >= 5) {
                    radius += 15.0f;
                }
            } else {
                radius -= 20.0f;
            }
            f32 receiverRadius = pSensor->mRadius;
            if (cylinderPushCheck(pSensor->mPosition - _2A0, receiverRadius, radius, height)) {
                if (sendBodyAttack(pSensor)) {
                    return;
                }
                if (tryGetItem(pSensor)) {
                    return;
                }
                touchSensor(pSensor);
                bool isPushed = MR::sendMsgPush(pSensor, getSensor("dummy"));
                isTouch = true;
                if (!isPushed && !pSensor->isType(ATYPE_POWER_STAR_BIND)) {
                    mMario->_10._27 = false;
                }
            }
        }
        addRushSensor(pSensor, isTouch);
    } else if (mMario->_10._6) {
        f32 height = _4B4;
        f32 radius = 20.0f + _4B0;
        if (isJumping()) {
            height += 10.0f;
        }
        if (mMario->mTargetWalkSpeedIndex >= 5) {
            radius += 15.0f;
        }
        f32 receiverRadius = pSensor->mRadius;
        TVec3f diff = pSensor->mPosition - _2A0;
        f32 vertical = MR::vecKillElement(diff, mCamDirZ, &diff);
        if (vertical < 250.0f && vertical > -250.0f && cylinderPushCheck(diff, receiverRadius, radius, height) && tryGetItem(pSensor)) {
            return;
        }
    }

    f32 clapRadius = mSensorRadiusClapCatch;
    if (mMario->isSwimming()) {
        clapRadius = mSensorRadiusClapCatchSwim;
    }
    if (distance < sensorRadius + clapRadius) {
        tryAddClapCoin(pSensor);
    }
    trySetLockOnTarget(pSensor);
    if (mPlayerMode == PlayerMode_Bee && distance < 100.0f + sensorRadius) {
        mMario->tryBeeStick(pSensor);
    }
    if (isUnderTarget(pSensor) && !selectNotHomingSensor(pSensor)) {
        const TVec3f& gravity = getGravityVector();
        f32 angle = MR::diffAngleAbs(pSensor->mPosition - _2A0, gravity);
        if (_4AC > angle) {
            _4AC = angle;
            _4A8 = pSensor;
        }
    }
}

void MarioActor::attackOrPushSensorInDamage(HitSensor* pReceiver, f32 radius) {
    f32 sensorRadius = pReceiver->mRadius;

    if (!isEnableMoveMario()) {
        return;
    }

    if (radius < sensorRadius + mSensorRadiusAttack) {
        bool b1 = false;
        f32 f1 = _4B4;
        f32 f2 = 20.0f + _4B0;

        if (isJumping()) {
            f1 += 10.0f;
        }

        if (mMario->mTargetWalkSpeedIndex >= 5) {
            f2 += 15.0f;
        }

        f32 f3 = pReceiver->mRadius;

        if (cylinderPushCheck(pReceiver->mPosition - _2A0, f3, f2, f1)) {
            if (tryGetItem(pReceiver)) {
                return;
            }

            touchSensor(pReceiver);
            MR::sendMsgPush(pReceiver, getSensor("dummy"));
            b1 = true;
        }

        addRushSensor(pReceiver, b1);
        return;
    }

    if (!mMario->_10._6) {
        return;
    }

    f32 f1 = _4B4;
    f32 f2 = 20.0f + _4B0;

    if (isJumping()) {
        f1 += 10.0f;
    }

    if (mMario->mTargetWalkSpeedIndex >= 5) {
        f2 += 15.0f;
    }

    f32 f3 = pReceiver->mRadius;
    TVec3f diff = pReceiver->mPosition - _2A0;
    f32 vecKill = MR::vecKillElement(diff, mCamDirZ, &diff);
    if (vecKill < 250.0f && vecKill > -250.0f && cylinderPushCheck(diff, f3, f2, f1) && tryGetItem(pReceiver)) {
        return;
    }
}

void MarioActor::attackOrPushSensorInRush(HitSensor* pSensor, f32 radius) {
    f32 sensorRadius = pSensor->mRadius;
    if (radius < sensorRadius + mSensorRadiusAttack) {
        f32 height = _4B4;
        f32 width = 20.0f + _4B0;
        if (cylinderPushCheck(pSensor->mPosition - _2A0, sensorRadius, width, height)) {
            if (tryGetItem(pSensor)) {
                return;
            }

            sendMsgToSensor(pSensor, ACTMES_RUSH_PLAYER_TOUCH);
        }

        addRushSensor(pSensor, false);
    }

    if (radius < sensorRadius + mSensorRadiusClapCatch) {
        tryAddClapCoin(pSensor);
    }

    trySetLockOnTarget(pSensor);
}

void MarioActor::tryAddClapCoin(HitSensor* pSensor) {
    if (_7DC != 64 && pSensor->isType(ATYPE_STAR_PIECE)) {
        _6DC[_7DC] = pSensor;
        _7DC++;
    }
}

bool MarioActor::tryTornadoAttack(HitSensor* pSensor) {
    bool isMessageReceived = pSensor->receiveMessage(ACTMES_TORNADO_ATTACK, getSensor("body"));
    if (isMessageReceived) {
        mMario->startPadVib(1);
    }

    return isMessageReceived;
}

bool MarioActor::isUnderTarget(HitSensor* pSensor) {
    TVec3f down;
    if (mBeeWallWalk != 0) {
        down = _240;
    } else {
        MR::calcGravityVectorOrZero(this, pSensor->mPosition, &down, mGravityInfo, 0);
    }

    if (down.dot(mPosition - pSensor->mPosition) <= 0.0f && mMario->_424 == 0) {
        return true;
    }

    return false;
}

bool MarioActor::tryHipDropAttack(HitSensor* pSensor) {
    if (isUnderTarget(pSensor) && cylinderHorizontalCheck(pSensor)) {
        if (pSensor->isType(ATYPE_PLAYER_AUTO_JUMP)) {
            return tryTrampleAttack(pSensor);
        }

        return pSensor->receiveMessage(ACTMES_PLAYER_HIP_DROP, getSensor("body"));
    }

    return false;
}

bool MarioActor::checkAndTryTrampleAttack(HitSensor* pSensor, f32 distance, bool isForce) {
    f32 sensorRadius = pSensor->mRadius;
    bool canTrample = !mMario->getMovementStates()._B;
    bool isFalling = false;
    if (isJumping() && !mMario->isRising()) {
        isFalling = true;
    }
    f32 jumpSpeed = 1.5f * mMario->mJumpVec.dot(getGravityVector());
    bool isNear = distance < sensorRadius + mSensorRadiusTrampleAttack + jumpSpeed;
    if (isForce) {
        isFalling = isJumping();
    }
    if (canTrample && isFalling && isNear) {
        bool isTrampled = false;
        if (tryTrampleAttack(pSensor)) {
            isTrampled = true;
        }
        if (mPlayerMode == PlayerMode_Invincible) {
            if (MR::sendArbitraryMsg(ACTMES_INVINCIBLE_ATTACK, pSensor, getSensor("body"))) {
                mMario->startPadVib(2);
                printHitMark(pSensor);
            }
        }
        return isTrampled;
    }
    return false;
}

bool MarioActor::tryTrampleAttack(HitSensor* pSensor) {
    if (_FCD) {
        return false;
    }

    if (mMario->isStatusActive(MarioStatus_Blown)) {
        return false;
    }

    bool isUnder = isUnderTarget(pSensor);
    bool isHorizontalCheck = cylinderHorizontalCheck(pSensor);
    if (isUnder && isHorizontalCheck) {
        _FCD = true;

        bool isReceiveMessage = pSensor->receiveMessage(ACTMES_PLAYER_TRAMPLE, getSensor("body"));

        _FCD = false;

        if (isReceiveMessage) {
            doTrampleJump(pSensor);
            tryGetItem(pSensor);
        }

        return isReceiveMessage;
    }

    return false;
}

bool MarioActor::cylinderHorizontalCheck(HitSensor* pSensor) {
    f32 radius = pSensor->mRadius;
    TVec3f diff = pSensor->mPosition - (_2A0 + mMario->_350 + mMario->_35C + mVelocity);
    TVec3f vec;
    MR::vecKillElement(diff, getGravityVector(), &vec);

    if (radius + _4B0 - vec.length() > 0.0f) {
        return true;
    }

    MR::vecKillElement(diff, _4C4, &vec);

    return radius + _4B0 - vec.length() > 0.0f;
}

bool MarioActor::tryJetAttack(HitSensor* pReceiver) {
    return MR::sendArbitraryMsg(ACTMES_JET_TURTLE_ATTACK, pReceiver, getSensor("dummy"));
}

void MarioActor::tryCounterJetAttack(HitSensor* pReceiver) {
    _1BC->addRequest(pReceiver, MR::MovementType_MsgSharedGroup);
}

bool MarioActor::tryGetItem(HitSensor* pSensor) {
    if (_934) {
        switch (pSensor->mType) {
        case ATYPE_JET_TURTLE:
        case ATYPE_JET_TURTLE_SLOW:
        case ATYPE_BOMBHEI:
        case ATYPE_NOKONOKO:
            return false;
        }
    }

    if (!isEnableNerveChange()) {
        return false;
    }

    if (mHealth == 0) {
        switch (pSensor->mType) {
        case ATYPE_JET_TURTLE:
        case ATYPE_JET_TURTLE_SLOW:
        case ATYPE_BOMBHEI:
        case ATYPE_NOKONOKO:
        case ATYPE_MORPH_ITEM:
            return false;
        }
    }

    switch (pSensor->mType) {
    case ATYPE_NOKONOKO:
        if (isDamaging()) {
            return false;
        }
        if (!isActionOk("カメ持ち")) {
            return false;
        }
        if (_468 != 0) {
            return false;
        }
        if (!pSensor->receiveMessage(ACTMES_ITEM_GET, getSensor("dummy"))) {
            return false;
        }
        _38C = 2;
        return false;
    case ATYPE_JET_TURTLE:
    case ATYPE_JET_TURTLE_SLOW:
    case ATYPE_BOMBHEI:
    case ATYPE_COINTHROW: {
        if (_3AC != 0) {
            return false;
        }
        if (isDamaging()) {
            return false;
        }
        if (!isActionOk("カメ持ち")) {
            return false;
        }
        if (_468 != 0) {
            return false;
        }

        bool canTake = false;
        TVec3f side;
        const TVec3f& gravity = getGravityVector();
        if (MR::vecKillElement(*getShadowPos() - pSensor->mPosition, gravity, &side) > 100.0f) {
            canTake = true;
        }
        if (!MR::isNearZero(pSensor->mHost->mVelocity)) {
            canTake = true;
        }
        if (mMario->getMovementStates()._B) {
            canTake = false;
        }

        if (!canTake && !mMario->isSwimming() && !mMario->getMovementStates()._1 && _424 == nullptr) {
            if (mMario->getMovementStates()._B) {
                return false;
            }
            doTrampleJump(pSensor);
        }
        break;
    }
    case ATYPE_MORPH_ITEM:
        if (_3D8 != 0) {
            return false;
        }
        if (mMario->mMorphResetTimer != 0) {
            return false;
        }
        break;
    case ATYPE_SWITCH:
        break;
    }

    if (!pSensor->receiveMessage(ACTMES_ITEM_GET, getSensor("dummy"))) {
        return false;
    }

    switch (pSensor->mType) {
    case ATYPE_JET_TURTLE:
    case ATYPE_JET_TURTLE_SLOW:
    case ATYPE_BOMBHEI:
    case ATYPE_COINTHROW:
        mMarioAnim->changePickupAnimation(pSensor);
        if (!mMario->isSwimming()) {
            if (mMario->getMovementStates()._1 && _B92 != -3) {
                if (pSensor->mType == ATYPE_BOMBHEI) {
                    _38C = 40;
                } else {
                    _38C = 25;
                }
            }
            mVelocity.zero();
            mMario->mWalkSpeed = 0.0f;
            if (mMario->getMovementStates()._1) {
                mMario->stopJump();
                mMario->mVerticalSpeed = 0.0f;
            }
        }
        _424 = pSensor;
        _480 = true;
        break;
    case ATYPE_MORPH_ITEM:
        if (_4A4 != nullptr) {
            _4A4->mHost->kill();
        }
        _4A4 = pSensor;
        break;
    case ATYPE_COIN:
    case ATYPE_KINOKO_ONEUP:
    case ATYPE_SWITCH:
        break;
    }

    return true;
}

bool MarioActor::cylinderPushCheck(const TVec3f& rOffset, f32 sensorRadius, f32 radius, f32 height) {
    TVec3f side;
    f32 vertical = MR::vecKillElement(rOffset, _4C4, &side);
    if (vertical > -sensorRadius) {
        f32 sideOverlap = sensorRadius + radius - side.length();
        f32 heightOverlap = sensorRadius + height - vertical;
        if (sideOverlap > 0.0f && heightOverlap > 0.0f) {
            if (sideOverlap > 0.0f) {
                TVec3f offset(_4C4);
                offset.scale(vertical);
                HitSensor* pSensor = getSensor("dummy");
                pSensor->mPosition.set(_2A0 + offset);
                getSensor("dummy")->mRadius = radius;
            } else {
                HitSensor* pSensor = getSensor("dummy");
                pSensor->mPosition.set(_2A0 + side);
                getSensor("dummy")->mRadius = height;
            }
            return true;
        }
    }

    if (mMario->getMovementStates()._A && mMario->getMovementStates()._1) {
        return false;
    }

    vertical = MR::vecKillElement(rOffset, _4B8, &side);
    if (vertical < -sensorRadius) {
        return false;
    }

    f32 sideOverlap = sensorRadius + radius - side.length();
    f32 heightOverlap = sensorRadius + height - vertical;
    if (sideOverlap > 0.0f && heightOverlap > 0.0f) {
        if (sideOverlap > 0.0f) {
            TVec3f offset(_4B8);
            offset.scale(vertical);
            HitSensor* pSensor = getSensor("dummy");
                pSensor->mPosition.set(_2A0 + offset);
            getSensor("dummy")->mRadius = radius;
        } else {
            HitSensor* pSensor = getSensor("dummy");
                pSensor->mPosition.set(_2A0 + side);
            getSensor("dummy")->mRadius = height;
        }
        return true;
    }

    return false;
}

void MarioActor::attackOrPushPolygons() {
    // FIXME: if chain has a mistake
    HitSensor* bodySensor = getSensor("body");

    _FCC = true;

    if (mMario->getMovementStates()._1 && _390 == 0) {
        HitSensor* groundSensor = mMario->mGroundPolygon->mSensor;

        if (groundSensor != nullptr && !MR::isDead(groundSensor->mHost)) {
            groundSensor->receiveMessage(ACTMES_FLOOR_TOUCH, bodySensor);
        }
    }

    if (mMario->getMovementStates()._8) {
        sendWallTouch(mMario->mFrontWallTriangle->mSensor, bodySensor);
    }

    if (mMario->getMovementStates()._19) {
        sendWallTouch(mMario->mBackWallTriangle->mSensor, bodySensor);
    }

    if (mMario->getMovementStates()._1A) {
        sendWallTouch(mMario->mSideWallTriangle->mSensor, bodySensor);
    }

    if (mMario->getMovementStates().jumping && !mMario->getMovementStates()._1 && mMario->getMovementStates()._B) {
        _3E8 = true;
    } else {
        if (_3E8) {
            if (getMovementStates()._1) {
                if (mMario->mGroundPolygon->isValid()) {
                    HitSensor* groundSensor = mMario->mGroundPolygon->mSensor;

                    if (groundSensor != nullptr && !MR::isDead(groundSensor->mHost)) {
                        groundSensor = mMario->mGroundPolygon->mSensor;
                        if (!groundSensor->receiveMessage(ACTMES_PLAYER_HIP_DROP_FLOOR, getSensor("body"))) {
                            _3E8 = false;
                        }
                    }
                }
            } else {
                _3E8 = false;
            }
        }

        if (!mMario->getMovementStates().jumping) {
            _3E8 = false;
        }
    }

    _FCC = false;
}

void MarioActor::sendWallTouch(HitSensor* pReceiver, HitSensor* pSender) {
    if (pReceiver == nullptr) {
        return;
    }

    if (MR::isDead(pReceiver->mHost)) {
        return;
    }

    pReceiver->receiveMessage(ACTMES_WALL_TOUCH, pSender);

    if (!mMario->getMovementStates()._F) {
        return;
    }

    pReceiver->receiveMessage(ACTMES_TORNADO_ATTACK, pSender);
}

bool MarioActor::sendMsgUpperPunch(HitSensor* pSensor) {
    if (!isActionOk("アッパーパンチ")) {
        return false;
    }

    if (pSensor != nullptr && !MR::isDead(pSensor->mHost) && pSensor->receiveMessage(ACTMES_PLAYER_UPPER_PUNCH, getSensor("body"))) {
        playSound("声蹴り", -1);

        if (!mMario->isSwimming()) {
            changeAnimation("アッパーパンチ", nullptr);
        }

        return true;
    }

    return false;
}
