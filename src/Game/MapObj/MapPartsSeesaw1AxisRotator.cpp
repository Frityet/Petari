#include "resource/TextEncoding.hpp"
#include "Game/MapObj/MapPartsSeesaw1AxisRotator.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/MapPartsUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace NrvMapPartsSeesaw1AxisRotator {
    NERVE_DECL_NULL(HostTypeWait);
    NEW_NERVE(HostTypeMoveStart, MapPartsSeesaw1AxisRotator, Move);
    NEW_NERVE(HostTypeMove, MapPartsSeesaw1AxisRotator, Move);
    NEW_NERVE(HostTypeStay, MapPartsSeesaw1AxisRotator, Stay);
    NEW_NERVE(HostTypeHipDrop, MapPartsSeesaw1AxisRotator, HipDrop);
    INIT_NERVE(HostTypeWait);
};

MapPartsSeesaw1AxisRotator::MapPartsSeesaw1AxisRotator(LiveActor* pActor, const char* pSoundName, f32 soundSpeedThreshold)
    : MapPartsRotatorBase(pActor, CP932("シーソー(1軸)")), mRotateSpeed(0.0f), mInertia(1000.0f), mRotateAngle(0.0f), mRestoreForce(0.0f),
      mIsPlayerOn(false), mRotateAxis(0.0f, 0.0f, 1.0f), mAngularSpeed(0.0f), mForce(0.0f), mBaseUp(0.0f, 1.0f, 0.0f),
      mSoundName(pSoundName), mSoundSpeedThreshold(soundSpeedThreshold) {
    mRotateMtx.identity();
}

MapPartsSeesaw1AxisRotator::~MapPartsSeesaw1AxisRotator() {
}

void MapPartsSeesaw1AxisRotator::init(const JMapInfoIter& rIter) {
    initNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeWait::sInstance);
    MR::getMapPartsArgRotateSpeed(&mRotateSpeed, rIter);
    mRotateSpeed *= 0.01f;
    MR::getMapPartsArgRotateAngle(&mRotateAngle, rIter);
    s32 inertia = 0;
    MR::getMapPartsArgRotateAccelType(&inertia, rIter);
    if (inertia > 0) {
        mInertia = inertia;
    }
    s32 restoreForce = 0;
    MR::getMapPartsArgRotateStopTime(&restoreForce, rIter);
    mRestoreForce = restoreForce;
    TPos3f baseMtx;
    baseMtx.set(mHost->getBaseMtx());
    baseMtx.getYDir(mBaseUp);
    MR::normalize(&mBaseUp);
}

void MapPartsSeesaw1AxisRotator::start() {
    TPos3f rotateMtx;
    rotateMtx.identity();
    rotateMtx.setRotateDegree(mHost->mRotation);
    mRotateMtx.set(rotateMtx);
    TPos3f baseMtx;
    baseMtx.identity();
    baseMtx.set(mHost->getBaseMtx());
    baseMtx.getZDir(mRotateAxis);
    MR::normalize(&mRotateAxis);
    f32 angle = 0.0f;
    calcRotatedAngle(&angle, mRotateMtx);
    if (mRotateAngle > 0.0f && mRotateAngle < angle) {
        TPos3f correctionMtx;
        correctionMtx.identity();
        correctionMtx.setRotate(mRotateAxis, 1.05f * ((PI / 180.0f) * (angle - mRotateAngle)));
        mRotateMtx.concat(correctionMtx);
    }
    mAngularSpeed = 0.0f;
    mForce = 0.0f;
    setNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeStay::sInstance);
}

void MapPartsSeesaw1AxisRotator::end() {
    mAngularSpeed = 0.0f;
    mForce = 0.0f;
    setNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeWait::sInstance);
}

bool MapPartsSeesaw1AxisRotator::isWorking() const {
    return true;
}

bool MapPartsSeesaw1AxisRotator::isMoving() const {
    return isWorking();
}

bool MapPartsSeesaw1AxisRotator::receiveMsg(u32 msg) {
    if (msg == 4) {
        if (!MR::isOnPlayer(MR::getBodySensor(mHost))) {
            return false;
        }
        mIsPlayerOn = true;
        if (tryHipDrop()) {
            return true;
        }
    }
    return false;
}

void MapPartsSeesaw1AxisRotator::exeMove() {
    if (isNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeMoveStart::sInstance) && 0.1f < MR::abs(mAngularSpeed)) {
        setNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeMove::sInstance);
        return;
    }
    rotate();
    if (mSoundName != nullptr && mSoundSpeedThreshold < MR::abs(mAngularSpeed)) {
        MR::startLevelSound(mHost, mSoundName, -1, -1, -1);
    }
}

void MapPartsSeesaw1AxisRotator::exeStay() {
    if (isFirstStep()) {
        mForce = 0.0f;
    }
    if (MR::isOnPlayer(MR::getBodySensor(mHost)) || mForce != 0.0f) {
        setNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeMoveStart::sInstance);
    }
}

void MapPartsSeesaw1AxisRotator::exeHipDrop() {
    if (isFirstStep()) {
        addForceHipDrop();
    }
    rotate();
    if (!mIsPlayerOn) {
        setNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeMove::sInstance);
        mIsPlayerOn = false;
    } else {
        mIsPlayerOn = false;
    }
}

void MapPartsSeesaw1AxisRotator::rotate() {
    TPos3f baseMtx;
    baseMtx.identity();
    baseMtx.set(mHost->getBaseMtx());
    baseMtx.getZDir(mRotateAxis);
    mAngularSpeed += mForce;
    updateVelocity();
    if (mAngularSpeed == 0.0f && mForce == 0.0f) {
        setNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeStay::sInstance);
        return;
    }
    mForce = 0.0f;
    if (isGoingToReachTargetAngle()) {
        mAngularSpeed *= -0.5f;
        return;
    }
    TPos3f rotateMtx;
    rotateMtx.identity();
    rotateMtx.setRotate(mRotateAxis, (PI / 180.0f) * mAngularSpeed);
    rotateMtx.concat(rotateMtx, mRotateMtx);
    mRotateMtx.set(rotateMtx);
    TVec3f rotation;
    mRotateMtx.getEuler(rotation);
    mHost->mRotation.set(rotation * (180.0f / PI));
}

void MapPartsSeesaw1AxisRotator::updateVelocity() {
    if (MR::isOnPlayer(MR::getBodySensor(mHost))) {
        TVec3f gravity;
        MR::calcGravityVector(mHost, &gravity, nullptr, 0);
        TVec3f offset(*MR::getPlayerPos());
        offset.sub(mHost->mPosition);
        TVec3f torque;
        PSVECCrossProduct(&offset, &gravity, &torque);
        f32 sign = MR::sign(torque.dot(mRotateAxis));
        mAngularSpeed += sign * (getDistanceFromRotAxis() / (0.1f * mInertia));
    }
    if (!isNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeMoveStart::sInstance) && getStep() > 2) {
        updateRestoreForce();
        mAngularSpeed *= 0.99f;
    }
    clampAngularSpeed();
}

void MapPartsSeesaw1AxisRotator::updateRestoreForce() {
    TPos3f baseMtx;
    baseMtx.identity();
    baseMtx.set(mHost->getBaseMtx());
    TVec3f up;
    baseMtx.getYDir(up);
    MR::normalize(&up);
    TVec3f torque;
    PSVECCrossProduct(&up, &mBaseUp, &torque);
    f32 dot = torque.dot(mRotateAxis);
    if (MR::isNearZero(dot, 0.001f) && MR::abs(mAngularSpeed) <= 0.1f) {
        mAngularSpeed = 0.0f;
        return;
    }
    mAngularSpeed += (MR::sign(dot) * (0.01f * mRestoreForce)) / (0.1f * mInertia);
}

void MapPartsSeesaw1AxisRotator::clampAngularSpeed() {
    mAngularSpeed = mAngularSpeed < -mRotateSpeed ? -mRotateSpeed : mAngularSpeed > mRotateSpeed ? mRotateSpeed : mAngularSpeed;
}

f32 MapPartsSeesaw1AxisRotator::getDistanceFromRotAxis() const {
    TVec3f position(mHost->mPosition);
    TVec3f offset(*MR::getPlayerPos());
    offset.sub(position);
    TPos3f baseMtx;
    baseMtx.identity();
    baseMtx.set(mHost->getBaseMtx());
    TVec3f axis;
    baseMtx.getXDir(axis);
    TVec3f projected;
    projected.scale(axis.dot(offset), axis);
    return 0.001f * PSVECMag(&projected);
}

void MapPartsSeesaw1AxisRotator::addForceHipDrop() {
    TVec3f gravity;
    MR::calcGravityVector(mHost, &gravity, nullptr, 0);
    TVec3f offset(*MR::getPlayerPos());
    offset.sub(mHost->mPosition);
    TVec3f torque;
    PSVECCrossProduct(&offset, &gravity, &torque);
    f32 sign = MR::sign(torque.dot(mRotateAxis));
    f32 inertia = 0.1f * mInertia;
    mAngularSpeed += sign * ((40.0f * getDistanceFromRotAxis()) / inertia);
}

bool MapPartsSeesaw1AxisRotator::isGoingToReachTargetAngle() const {
    if (!(mRotateAngle > 0.0f)) {
        return false;
    }
    TPos3f rotateMtx;
    rotateMtx.identity();
    rotateMtx.setRotate(mRotateAxis, (PI / 180.0f) * mAngularSpeed);
    rotateMtx.concat(rotateMtx, mRotateMtx);
    f32 angle = 0.0f;
    calcRotatedAngle(&angle, rotateMtx);
    return mRotateAngle <= angle;
}

void MapPartsSeesaw1AxisRotator::calcRotatedAngle(f32* pAngle, const TPos3f& rMtx) const {
    TVec3f up;
    rMtx.getYDir(up);
    MR::normalize(&up);
    TVec3f referenceUp;
    TVec3f gravity;
    MR::calcGravityVector(mHost, &gravity, nullptr, 0);
    if (!gravity.isZero()) {
        referenceUp.set(-gravity);
    } else {
        referenceUp.set(0.0f, 1.0f, 0.0f);
    }
    MR::normalize(&referenceUp);
    f32 dot = up.dot(TVec3f(referenceUp));
    if (MR::isNearZero(1.0f - dot, 0.001f)) {
        dot = 1.0f;
    }
    if (MR::isNearZero(dot, 0.001f)) {
        dot = 0.0f;
    }
    *pAngle = (180.0f / PI) * JMAAcosRadian(dot);
}

bool MapPartsSeesaw1AxisRotator::tryHipDrop() {
    if (isNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeMove::sInstance) || isNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeMoveStart::sInstance) ||
        isNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeStay::sInstance)) {
        setNerve(&NrvMapPartsSeesaw1AxisRotator::HostTypeHipDrop::sInstance);
        return true;
    }
    return false;
}
