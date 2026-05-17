#include "Game/LiveActor/HitSensor.hpp"
#include "Game/MapObj/CollectCounter.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioRabbit.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Player/MarioWall.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"

extern "C" int strcmp(const char*, const char*);

namespace {
    const char* cSensorBody = "body";
    const char* cSensorScouter = "ex-eye";
    const char* cSensorEye = "eye";
    const char* cSensorDummy = "dummy";
    const char* cJointSpine1 = "Spine1";
    const char* cAnimSpin2nd = "spin2nd";
    const char* cAnimGroundTwist = "地上ひねり";
    const char* cAnimSomersault = "サマーソルト";
    const char* cAnimSwimSpin = "水泳スピン";
    const char* cAnimWaterSurfaceSpin = "水上スピン";
    const char* cAnimSquatSpin = "しゃがみスピン";
    const char* cAnimFooFighterSpin = "フーファイタースピン";
    const char* cAnimBeeSpin = "ハチスピン";
    const char* cAnimBeeSpinAir = "ハチスピン空中";
    const char* cAnimIceTwistAir = "アイスひねり空中";
    const char* cAnimFireSpinAir = "ファイアスピン空中";
    const char* cAnimFireSpin = "ファイアスピン";
    const char* cAnimIceTwist = "アイスひねり";
    const char* cAnimIceTwistMove = "アイスひねり移動";
    const char* cAnimIceTwistWait = "アイスひねり静止";
    const char* cAnimHammerThrowRelease = "ハンマー投げリリース";
    const char* cAnimHelicopterJump = "ヘリコプタージャンプ";
    const char* cActorCannonShell = "砲弾";
    const char* cActorGoombaForKillAll = "全滅用クリボー";
};

void MarioActor::setupSensors() {
    initHitSensor(4);
    MR::addHitSensorCallback(this, cSensorBody, ATYPE_PLAYER, 0x20, 100.0f);
    MR::addHitSensorCallback(this, cSensorScouter, ATYPE_PUSH, 0x20, 100.0f);
    MR::addHitSensorCallback(this, cSensorEye, ATYPE_EYE, 0x40, 2000.0f);

    MR::addHitSensor(this, cSensorDummy, ATYPE_PLAYER, 1, 0.0f, TVec3f(0.0f, 0.0f, 0.0f));
    getSensor(cSensorDummy)->invalidate();

    _468 = nullptr;
    _46C = nullptr;
    _7DC = 0;

    for (u32 i = 0; i < 0x80; i++) {
        _4D0[i] = 0;
    }

    for (u32 i = 0; i < 4; i++) {
        _428[i] = nullptr;
    }

    initScouter();
    _3E5 = false;
    _3E8 = false;

    MR::initStarPointerTarget(this, 80.0f, TVec3f(0.0f, 80.0f, 0.0f));
}

void MarioActor::updateHitSensor(HitSensor* pSensor) {
    switch (pSensor->mType) {
    case ATYPE_PLAYER:
        if (mMario->isStatusActive(5)) {
            getRealPos(cJointSpine1, &pSensor->mPosition);
            pSensor->mRadius = 60.0f;
            return;
        }

        pSensor->mPosition = _2A0;

        if (mMario->mMovementStates._B && !mMario->mMovementStates._1) {
            pSensor->mPosition.addInline(mMario->mJumpVec);
        }

        pSensor->mRadius = 100.0f;
        return;
    case ATYPE_PUSH:
        pSensor->setType(ATYPE_EYE);
        return;
    case ATYPE_EYE:
        if (pSensor == getSensor(cSensorScouter)) {
            updateScouter();
            return;
        }

        pSensor->mPosition = _2A0;

        f32 radius = 600.0f;

        if (mMario->mMovementStates._2F) {
            radius = 1000.0f;
        }

        if (_468 != nullptr || mMario->isSwimming()) {
            radius = 2000.0f;
        }

        pSensor->mRadius = radius;
        _3E5 = false;
        _3E6 = false;

        if (strcmp(mMarioAnim->mXanimePlayer->getCurrentBckName(), cAnimSpin2nd) == 0) {
            _3E5 = true;
            _3E6 = true;

            if (_944 <= 2) {
                if (_944 == 0) {
                    _945 = 0;
                    _974 = 0;
                }

                _944 = 2;
            }
        }

        if (!mMario->isAnimationTerminate(nullptr)) {
            if (mMario->_430 == 8 && isJumping()) {
                _3E5 = true;
            }

            if (isAnimationRun(cAnimGroundTwist) || isAnimationRun(cAnimSomersault)
                || isAnimationRun(cAnimSwimSpin) || isAnimationRun(cAnimWaterSurfaceSpin)
                || isAnimationRun(cAnimSquatSpin) || isAnimationRun(cAnimFooFighterSpin)
                || isAnimationRun(cAnimBeeSpin) || isAnimationRun(cAnimBeeSpinAir)
                || isAnimationRun(cAnimIceTwistAir) || isAnimationRun(cAnimFireSpinAir)
                || isAnimationRun(cAnimFireSpin) || isAnimationRun(cAnimIceTwist)
                || isAnimationRun(cAnimIceTwistMove) || isAnimationRun(cAnimIceTwistWait)
                || isAnimationRun(cAnimHammerThrowRelease)) {
                _3E5 = true;
            }

            if (mMario->isSwimming() && mMario->mSwim->check7Aand7C()) {
                _3E5 = true;
            }

            if (_3E5) {
                _3E6 = true;
            }
        }

        if (pSensor->mValidByHost && pSensor->mValidBySystem) {
            attackOrPushPolygons();
        }

        if (_424 != nullptr) {
            tryTornadoPull(_424);
        }
    }
}

void MarioActor::doTrampleJump(HitSensor* pSensor) {
    if (mMario->mMovementStates._1A) {
        return;
    }

    if (mMario->isStatusActive(1)) {
        mMario->closeStatus(mMario->mWall);
    }

    MarioConstTable* table = mConst->getTable();
    mMario->_402 = table->mTrampleBegomaOpenTime;
    mMario->mMovementStates._2B = false;

    if (pSensor->mType == ACTMES_TAKEN) {
        _988 = 0;
        trampleJump(table->mTrampleBegoma, table->mTrampleLong);
        changeAnimationNonStop(cAnimHelicopterJump);
        mMario->startPadVib(2ul);
        playSound(cAnimHelicopterJump, -1);
        mMario->startRotationTask(4);
        mMario->_430 = 0xB;
        return;
    }

    if (strcmp(pSensor->mHost->mName, cActorCannonShell) == 0) {
        mMario->playSoundTrampleCombo(_989);
        _989++;

        if (_989 > 1 && _989 < 5) {
            _1B8->setCount(_989);
        }

        if (_989 == 5) {
            _1B8->kill();
            _989 = 0;
            MR::requestOneUp();
            MR::incPlayerLeft();
        }
    }

    if (strcmp(pSensor->mHost->mName, cActorGoombaForKillAll) == 0) {
        mMario->playSoundTrampleCombo(_989);
        _989++;

        if (_989 > 1 && _989 < 8) {
            _1B8->setCount(_989);
        }

        if (_989 >= 8) {
            _1B8->kill();
            MR::requestOneUp();
            MR::incPlayerLeft();
        }
    }

    trampleJump(table->mTrampleNormal, table->mTrampleLong);
}

void MarioActor::trampleJump(f32 ySpeed, f32 ySpeedLvlA) {
    if (mMario->isStatusActive(0x18)) {
        return;
    }

    TVec3f jumpVec(mMario->mJumpVec);
    MR::vecKillElement(jumpVec, getGravityVec(), &jumpVec);

    TVec3f gravity = -getGravityVec();
    gravity.scale(ySpeed);
    jumpVec.addInline(gravity);

    if (_988 == 2) {
        if (mMario->checkLvlA()) {
            jumpVec.scale(1.2f);
        }
        else {
            jumpVec.scale(1.5f);
        }
    }

    if (mMario->checkLvlA()) {
        TVec3f gravityLvlA = -getGravityVec();
        gravityLvlA.scale(ySpeedLvlA);
        jumpVec.addInline(gravityLvlA);
    }

    mMario->tryForceFreeJump(jumpVec);
    mMario->popTask(&Mario::taskOnHipDropSlide);

    if (mPlayerMode != 5) {
        if (!mMario->mMovementStates._A) {
            _988++;

            if (_988 == 1) {
                changeAnimation(cAnimGroundTwist, nullptr);
            }
            else if (_988 == 2) {
                changeAnimation(cAnimSomersault, nullptr);
            }
            else if (_988 >= 3) {
                changeAnimation(cAnimSwimSpin, nullptr);
                _988 = 0;
            }
        }
    }
    else {
        stopAnimation(nullptr);

        if (mMario->mRabbit->mJumpAnimationIndex == 0) {
            changeAnimation(cAnimBeeSpinAir, nullptr);
        }
        else if (mMario->mRabbit->mJumpAnimationIndex == 1) {
            changeAnimation(cAnimFireSpinAir, nullptr);
        }
    }

    playSound("ジャンプ", -1);
    playEffect("踏み");
    mMario->startPadVib(0ul);
    mMario->mMovementStates._2F = false;
    mMario->mMovementStates._22 = false;
    mMario->mMovementStates._3E = 0;
}

void MarioActor::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (!isEnableNerveChange()) {
        return;
    }

    if (pSender->mType == ATYPE_PLAYER) {
        if (_934 && !(getSensor(cSensorEye)->mValidByHost && getSensor(cSensorEye)->mValidBySystem)) {
            addRushSensor(pReceiver, false);
        }

        return;
    }

    HitSensor* eye = getSensor(cSensorEye);

    if (pSender == eye) {
        if (MR::isDead(pReceiver->mHost)) {
            return;
        }

        f32 dist = pReceiver->mPosition.distance(pSender->mPosition);

        if (_934) {
            attackOrPushSensorInRush(pReceiver, dist);
        }
        else if (isDamaging()) {
            attackOrPushSensorInDamage(pReceiver, dist);
        }
        else {
            attackOrPushSensor(pReceiver, dist);
        }
    }

    if (pSender == getSensor(cSensorScouter)) {
        recordScoutingObject(pReceiver);
    }
}

bool MarioActor::sendMsgToSensor(HitSensor* pSensor, u32 msg) {
    return pSensor->receiveMessage(msg, getSensor(cSensorBody));
}

void MarioActor::resetSensorCount() {
    _930 = 0;
    _7DC = 0;
    _46C = nullptr;
}

void MarioActor::recordScoutingObject(HitSensor* pSensor) {
    if (pSensor == _424) {
        return;
    }

    if (!MR::isSensorEnemy(pSensor) && !MR::isSensorMapObj(pSensor) && !MR::isSensorRide(pSensor)) {
        return;
    }

    HitSensor* scouter = getSensor(cSensorScouter);
    TVec3f sensorDiff = pSensor->mPosition - scouter->mPosition;

    if (MR::diffAngleAbsHorizontal(sensorDiff, mMario->mFrontVec, _240) >= 1.5707964f) {
        return;
    }

    _9D4 = pSensor;
    _9D8 = scouter->mPosition;
    _9CC = _9D0;
    _9D0 = 60.0f;
}

void MarioActor::updateScouter() {
    HitSensor* previousTarget = _F24;
    _F24 = nullptr;

    if (_468 != nullptr) {
        if (_9D4 != nullptr && MR::isExistInAttributeGroupSearchTurtle(_9D4->mHost)) {
            if (MR::isSensorEnemy(_9D4)) {
                _F28 = 0x10;
            }
            else {
                _F28 = 2;
            }

            _F24 = _9D4;
        }
        else if (_F28 != 0) {
            _F24 = previousTarget;
            _F28--;
        }
    }

    _9D0 += 80.0f;

    if (_9D0 < 100.0f) {
        _9D0 = 100.0f;
    }

    if (_9D0 > _9CC) {
        _9D4 = nullptr;
    }

    f32 maxDist = 1000.0f;

    if (mMario->isStatusActive(0x18)) {
        maxDist = 2400.0f;
    }

    if (_468 != nullptr) {
        maxDist = 3000.0f;
    }

    if (_9D0 > maxDist) {
        _9D0 = 60.0f;
    }

    TVec3f scouterPos;

    if (mMario->isSwimming()) {
        TVec3f offset(_2D0);
        offset.scale(_9D0);
        scouterPos = _2A0;
        scouterPos.addInline(offset);
    }
    else {
        TVec3f offset(mMario->mFrontVec);
        offset.scale(_9D0);
        scouterPos = _2A0;
        scouterPos.addInline(offset);
    }

    getSensor(cSensorScouter)->mPosition = scouterPos;

    f32 radius = 100.0f;

    if (_9D0 >= 300.0f) {
        radius = 100.0f + (_9D0 - 300.0f) * 0.709807f;
    }

    if (_9D0 < 200.0f) {
        radius = 40.0f;
    }

    getSensor(cSensorScouter)->mRadius = radius;
}

void MarioActor::initScouter() {
    _9CC = 0.0f;
    _9D0 = 60.0f;
    _9D4 = nullptr;
    _9D8.zero();
    getSensor(cSensorScouter)->mRadius = 100.0f;
    getSensor(cSensorScouter)->validate();
}

void MarioActor::initForJump() {
    _988 = 0;
    _989 = 0;
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
