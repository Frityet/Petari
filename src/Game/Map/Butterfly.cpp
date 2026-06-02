#include "Game/Map/Butterfly.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util.hpp"

namespace {
    const Vec sMarioCapPosOffset = {0.0f, 50.2f, 24.9f};
};

namespace NrvButterfly {
    NEW_NERVE(HostTypeWait, Butterfly, Wait);
    NEW_NERVE(HostTypeRunAway, Butterfly, RunAway);
    NEW_NERVE(HostTypeHive, Butterfly, Hive);
    NEW_NERVE(HostTypePerchOn, Butterfly, PerchOn);
    NEW_NERVE(HostTypeGotoSleepingMario, Butterfly, GotoSleepingMario);
    NEW_NERVE(HostTypeReadyToPerchOnSleepingMario, Butterfly, ReadyToPerchOnSleepingMario);
    NEW_NERVE(HostTypePerchOnSleepingMario, Butterfly, PerchOnSleepingMario);
};  // namespace NrvButterfly

Butterfly::Butterfly(const char* pName) : LiveActor(pName), mHomePosition(gZeroVec) {
    mColorFrame = 0;
    mPerchSensor = nullptr;
    mAppearedStarPiece = false;
    mQuat.set(0.0f, 0.0f, 0.0f, 1.0f);
}

Butterfly::~Butterfly() {}

void Butterfly::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    mHomePosition.x = mPosition.x;
    mHomePosition.y = mPosition.y;
    mHomePosition.z = mPosition.z;
    initModelManagerWithAnm("Butterfly", nullptr, false);
    MR::connectToSceneMapObj(this);
    MR::getJMapInfoArg0NoInit(rIter, &mColorFrame);
    MR::startBrk(this, "Butterfly");
    MR::setBrkFrameAndStop(this, mColorFrame);
    initHitSensor(1);

    TVec3f sensorOffset;
    sensorOffset.x = 0.0f;
    sensorOffset.y = 0.0f;
    sensorOffset.z = 0.0f;
    MR::addHitSensorMtxAnimal(this, "body", 8, 30.0f, MR::getJointMtx(this, "buttBody"), sensorOffset);
    initBinder(30.0f, 0.0f, 0);
    initSound(2, false);

    s32 starPieceArg = -1;
    MR::getJMapInfoArg1NoInit(rIter, &starPieceArg);
    if (starPieceArg == -1) {
        MR::declareStarPiece(this, 1);
    }
    else {
        mAppearedStarPiece = true;
    }

    TVec3f pointerOffset;
    pointerOffset.x = 0.0f;
    pointerOffset.y = 0.0f;
    pointerOffset.z = 0.0f;
    MR::initStarPointerTarget(this, 100.0f, pointerOffset);
    MR::setClippingTypeSphere(this, 30.0f);
    MR::setClippingFar50m(this);
    initNerve(&NrvButterfly::HostTypeWait::sInstance);
    makeActorAppeared();
    MR::startBck(this, "Butterfly", nullptr);
    MR::setBckFrameAtRandom(this);

    if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::syncStageSwitchAppear(this);
        makeActorDead();
    }
}

void Butterfly::initAfterPlacement() {
    if (MR::isEqualStageName("HeavensDoorGalaxy")) {
        TVec3f gravity;
        MR::calcGravityVector(this, &gravity, nullptr, 0);

        TVec3f up = -gravity;
        MR::makeQuatUpNoSupport(&mQuat, up);
    }
    else {
        TVec3f rotate(mRotation);
        rotate.scale(0.017453292f);
        mQuat.setEuler(rotate.x, rotate.y, rotate.z);
    }
}

void Butterfly::control() {
    updatePosture();

    s32 port = 0;
    if (MR::isStarPointerInScreenAnyPort(&port)) {
        if (MR::getStarPointerScreenSpeed(port) < 3.0f) {
            MR::setStarPointerTargetRadius3d(this, 200.0f);
        }
        else {
            MR::setStarPointerTargetRadius3d(this, 100.0f);
        }

        tryAppearStarPeace(port);
    }
}

void Butterfly::calcAndSetBaseMtx() {
    TPos3f baseMtx;
    mQuat.makeMtx(baseMtx);
    baseMtx.setTrans(mPosition);
    MR::setBaseTRMtx(this, baseMtx);
}

void Butterfly::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isSensorNpc(pReceiver) && isNerve(&NrvButterfly::HostTypeHive::sInstance)) {
        mPerchSensor = pReceiver;
        setNerve(&NrvButterfly::HostTypePerchOn::sInstance);
    }
}

void Butterfly::updatePosture() {
    TVec3f gravity;
    MR::calcGravityVector(this, &gravity, nullptr, 0);

    TVec3f upDir;
    mQuat.getYDir(upDir);

    TVec3f frontDir;
    mQuat.getZDir(frontDir);

    TVec3f negGravity = -gravity;
    TQuat4f upQuat;
    upQuat.setRotate(upDir, negGravity, 1.0f);

    TVec3f velocity(mVelocity);
    if (PSVECMag(&velocity) < 0.5f) {
        mQuat.normalize();
        return;
    }

    MR::vecKillElement(velocity, gravity, &velocity);
    if (MR::isNearZero(velocity, 0.001f)) {
        mQuat.normalize();
        return;
    }

    MR::normalize(&velocity);

    TVec3f targetFront;
    JMAVECScaleAdd(&upDir, &velocity, &targetFront, -upDir.dot(velocity));
    MR::normalizeOrZero(&targetFront);

    if (!MR::isNearZero(targetFront, 0.001f)) {
        TVec3f turnedFront;
        MR::turnVecToVecCos(&turnedFront, frontDir, targetFront, 0.997f, upDir, 0.02f);

        TQuat4f turnQuat;
        turnQuat.setRotate(frontDir, turnedFront, 1.0f);
        PSQUATMultiply(&turnQuat, &mQuat, &mQuat);
    }

    mQuat.normalize();
}

void Butterfly::addRunAwayVelocity() {
    s32 port = *MR::getStarPointerLastPointedPort(this);

    TVec2f pointerPos;
    pointerPos.set(*MR::getStarPointerScreenPosition(port));

    TVec2f screenPos;
    MR::calcScreenPosition(&screenPos, mPosition);

    TVec2f screenAway = screenPos - pointerPos;
    if (!MR::isNearZero(screenAway, 0.001f)) {
        MR::normalize(&pointerPos);
    }

    TVec3f camX = MR::getCamXdir();
    TVec3f camY = MR::getCamYdir();
    TVec3f negCamY = -camY;

    TVec3f yMove(negCamY);
    yMove.scale(screenAway.y);

    TVec3f xMove(camX);
    xMove.scale(screenAway.x);

    TVec3f runAway(xMove);
    runAway.add(yMove);

    TVec3f gravity;
    MR::calcGravityVector(this, &gravity, nullptr, 0);

    if (runAway.dot(gravity) > 0.0f) {
        MR::vecKillElement(runAway, gravity, &runAway);
    }

    runAway.setLength(12.0f);
    mVelocity.add(runAway);
}

bool Butterfly::tryRunAway() {
    s32 port = *MR::getStarPointerLastPointedPort(this);

    if (!MR::isStarPointerInScreen(port)) {
        return false;
    }

    if (!MR::isStarPointerPointing2P(this, nullptr, false, false)) {
        return false;
    }

    if (MR::testCorePadButtonB(port)) {
        return false;
    }

    if (PSVECDistance(&mPosition, &mHomePosition) > 500.0f) {
        return false;
    }

    TVec2f screenPos;
    MR::calcScreenPosition(&screenPos, mPosition);

    TVec2f pointerPos;
    pointerPos.set(*MR::getStarPointerScreenPosition(port));

    TVec2f away = screenPos - pointerPos;
    TVec2f* pointerVelocity = MR::getStarPointerScreenVelocity(port);

    if (pointerVelocity->x * away.x + pointerVelocity->y * away.y <= 0.0f) {
        return false;
    }

    if (!MR::tryStarPointerCheckWithoutRumble(this, false)) {
        return false;
    }

    addRunAwayVelocity();
    setNerve(&NrvButterfly::HostTypeRunAway::sInstance);
    return true;
}

bool Butterfly::tryHive() {
    s32 port = 0;
    if (!MR::isStarPointerInScreenAnyPort(&port)) {
        return false;
    }

    if (PSVECDistance(&mPosition, &mHomePosition) > 500.0f) {
        return false;
    }

    TVec2f pointerPos;
    pointerPos.set(*MR::getStarPointerScreenPosition(port));

    TVec2f screenPos;
    MR::calcScreenPosition(&screenPos, mPosition);

    f32 x = pointerPos.x - screenPos.x;
    f32 y = pointerPos.y - screenPos.y;
    if (JGeometry::TUtil<f32>::sqrt(x * x + y * y) > 100.0f) {
        return false;
    }

    if (!MR::testCorePadButtonB(port)) {
        return false;
    }

    setNerve(&NrvButterfly::HostTypeHive::sInstance);
    return true;
}

bool Butterfly::tryPerchOnSleepingMario() {
    if (MR::calcDistanceToPlayer(this) > 500.0f) {
        return false;
    }

    if (!MR::isPlayerSleeping()) {
        return false;
    }

    setNerve(&NrvButterfly::HostTypeGotoSleepingMario::sInstance);
    return true;
}

bool Butterfly::tryAppearStarPeace(long port) {
    if (mAppearedStarPiece) {
        return false;
    }

    f32 distance;
    if (!MR::calcStarPointerScreenDistanceToTarget(this, &distance, port)) {
        return false;
    }

    if (distance > 10.0f) {
        return false;
    }

    if (!MR::appearStarPiece(this, mPosition, 1, 10.0f, 40.0f, false)) {
        return false;
    }

    MR::startSound(this, "SE_OJ_STAR_PIECE_BURST", -1, -1);
    mAppearedStarPiece = true;
    return true;
}

void Butterfly::exeWait() {
    if (tryRunAway()) {
        return;
    }

    if (tryHive()) {
        return;
    }

    if (tryPerchOnSleepingMario()) {
        return;
    }

    MR::setBckRate(this, MR::converge(MR::getBckRate(this), 1.0f, 0.02f));
    mVelocity.x *= 0.99f;
    mVelocity.y *= 0.99f;
    mVelocity.z *= 0.99f;

    TVec3f toHome = mHomePosition - mPosition;
    if (!MR::isNearZero(toHome, 0.001f)) {
        MR::normalize(&toHome);

        TVec3f homeVelocity(toHome);
        homeVelocity.scale(0.01f);
        mVelocity.add(homeVelocity);

        if (PSVECMag(&mVelocity) > 2.0f) {
            mVelocity.setLength(2.0f);
        }

        if (MR::isInvalidClipping(this) && PSVECDistance(&mHomePosition, &mPosition) < 200.0f) {
            MR::validateClipping(this);
        }
    }
}

void Butterfly::exeRunAway() {
    if (MR::isFirstStep(this) && !MR::isInvalidClipping(this)) {
        MR::invalidateClipping(this);
    }

    MR::setBckRate(this, MR::converge(MR::getBckRate(this), 3.0f, 0.02f));
    mVelocity.x *= 0.97f;
    mVelocity.y *= 0.97f;
    mVelocity.z *= 0.97f;

    if (MR::isStep(this, 30)) {
        setNerve(&NrvButterfly::HostTypeWait::sInstance);
    }
}

void Butterfly::exeHive() {
    if (tryRunAway()) {
        return;
    }

    s32 port = 0;
    if (!MR::isStarPointerInScreenAnyPort(&port)) {
        setNerve(&NrvButterfly::HostTypeWait::sInstance);
        return;
    }

    if (PSVECDistance(&mPosition, &mHomePosition) > 500.0f) {
        setNerve(&NrvButterfly::HostTypeWait::sInstance);
        return;
    }

    TVec3f pointingPos;
    MR::calcStarPointerWorldPointingPos(&pointingPos, mPosition, port);

    TVec3f toPoint = pointingPos - mPosition;
    toPoint.scale(0.05f);
    mVelocity.add(toPoint);

    if (PSVECMag(&mVelocity) > 2.5f) {
        mVelocity.setLength(2.5f);
    }

    MR::setBckRate(this, MR::converge(MR::getBckRate(this), 1.2f, 0.02f));
}

void Butterfly::exePerchOn() {
    HitSensor* bodySensor = getSensor("body");

    if (MR::isNear(bodySensor, mPerchSensor, bodySensor->mRadius + mPerchSensor->mRadius)) {
        mVelocity.zero();
        MR::setBckRate(this, MR::converge(MR::getBckRate(this), 0.5f, 0.02f));
    }
    else {
        TVec3f toSensor = mPerchSensor->mPosition - mPosition;
        toSensor.setLength(0.05f);
        mVelocity.add(toSensor);

        if (PSVECMag(&mVelocity) > 2.5f) {
            mVelocity.setLength(2.5f);
        }

        MR::setBckRate(this, MR::converge(MR::getBckRate(this), 1.2f, 0.02f));
    }

    tryRunAway();
}

void Butterfly::exeGotoSleepingMario() {
    if (!MR::isLessStep(this, 300)) {
        TPos3f capMtx;
        MR::calcPlayerJointMtx(&capMtx, "CapPosition");

        TVec3f capOffset(sMarioCapPosOffset);
        TVec3f capPosition;
        capMtx.mult(capOffset, capPosition);

        TVec3f toCap = capPosition - mPosition;
        toCap.setLength(0.05f);
        mVelocity.add(toCap);

        if (PSVECMag(&mVelocity) > 2.0f) {
            mVelocity.setLength(2.0f);
        }

        if (PSVECDistance(&mPosition, &capPosition) < 10.0f && MR::checkPassBckFrame(this, 0.0f)) {
            mVelocity.zero();
            setNerve(&NrvButterfly::HostTypeReadyToPerchOnSleepingMario::sInstance);
        }
    }
}

void Butterfly::exeReadyToPerchOnSleepingMario() {
    TPos3f capMtx;
    MR::calcPlayerJointMtx(&capMtx, "CapPosition");

    TVec3f capOffset(sMarioCapPosOffset);
    TVec3f capPosition;
    capMtx.mult(capOffset, capPosition);

    MR::vecBlend(mPosition, capPosition, &mPosition, 0.5f);
    MR::setBckRate(this, MR::converge(MR::getBckRate(this), 0.5f, 0.02f));

    if (MR::checkPassBckFrame(this, 0.0f)) {
        setNerve(&NrvButterfly::HostTypePerchOnSleepingMario::sInstance);
    }
}

void Butterfly::exePerchOnSleepingMario() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Wait", nullptr);
    }

    TPos3f capMtx;
    MR::calcPlayerJointMtx(&capMtx, "CapPosition");

    TVec3f capOffset(sMarioCapPosOffset);
    capMtx.mult(capOffset, mPosition);

    if (!MR::isPlayerSleeping()) {
        MR::startBck(this, "Butterfly", nullptr);
        setNerve(&NrvButterfly::HostTypeWait::sInstance);
    }
}
