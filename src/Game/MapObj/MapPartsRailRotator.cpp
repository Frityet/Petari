#include "resource/TextEncoding.hpp"
#include "Game/MapObj/MapPartsRailRotator.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/MapPartsUtil.hpp"
#include "Game/Util/MathUtil.hpp"

namespace NrvMapPartsRailRotator {
    NERVE_DECL_NULL(HostTypeWait);
    NEW_NERVE(HostTypeRotateAtPoint, MapPartsRailRotator, Rotate);
    NEW_NERVE(HostTypeRotateBetweenPoints, MapPartsRailRotator, Rotate);
    NERVE_DECL_NULL(HostTypeDone);
    INIT_NERVE(HostTypeWait);
    INIT_NERVE(HostTypeDone);
};

MapPartsRailRotator::MapPartsRailRotator(LiveActor* pActor)
    : MapPartsFunction(pActor, CP932("レイル回転")), mRotateAxis(0), mRotateType(0), mRotateSpeed(0.0f), mTargetAngle(0.0f), mAngle(0.0f),
      mHostRotateMtx(nullptr) {
    _2C.identity();
    _5C.identity();
}

MapPartsRailRotator::~MapPartsRailRotator() {
}

void MapPartsRailRotator::init(const JMapInfoIter&) {
    initNerve(&NrvMapPartsRailRotator::HostTypeWait::sInstance);
}

void MapPartsRailRotator::initWithRotateMtx(const JMapInfoIter& rIter, MtxPtr pMtx) {
    mHostRotateMtx = pMtx;
    init(rIter);
}

void MapPartsRailRotator::start() {
}

void MapPartsRailRotator::end() {
    _2C.identity();
    _5C.identity();
    mAngle = 0.0f;
    setNerve(&NrvMapPartsRailRotator::HostTypeWait::sInstance);
}

bool MapPartsRailRotator::isWorking() const {
    return mRotateSpeed != 0.0f && mTargetAngle > 0.0f;
}

bool MapPartsRailRotator::hasRotation(s32 point) const {
    f32 speed = -1.0f;
    MR::getMapPartsArgRailRotateSpeed(&speed, mHost, point);
    f32 angle = 0.0f;
    MR::getMapPartsArgRailRotateAngle(&angle, mHost, point);
    return speed != 0.0f && angle > 0.0f;
}

void MapPartsRailRotator::rotateAtPoint(s32 point) {
    updateInfo(point);
    if (!isWorking()) {
        setNerve(&NrvMapPartsRailRotator::HostTypeDone::sInstance);
    } else {
        setNerve(&NrvMapPartsRailRotator::HostTypeRotateAtPoint::sInstance);
    }
}

bool MapPartsRailRotator::hasRotationBetweenPoints(s32 point) const {
    if (!hasRotation(point)) {
        return false;
    }
    s32 type = 0;
    MR::getMapPartsArgRailRotateType(&type, mHost, point);
    return type == 1;
}

void MapPartsRailRotator::rotateBetweenPoints(s32 point, f32 time) {
    updateInfo(point);
    if (!isWorking()) {
        setNerve(&NrvMapPartsRailRotator::HostTypeDone::sInstance);
    } else {
        mRotateSpeed = (mTargetAngle / time) * MR::sign(mRotateSpeed);
        setNerve(&NrvMapPartsRailRotator::HostTypeRotateBetweenPoints::sInstance);
    }
}

void MapPartsRailRotator::updateHostRotateMtx() {
    if (mHostRotateMtx != nullptr) {
        PSMTXCopy(mHostRotateMtx, _2C.toMtxPtr());
    } else if (mHost->getBaseMtx() != nullptr) {
        _2C.setInline(mHost->getBaseMtx());
        _2C.zeroTrans();
    } else {
        _2C.setRotateDegree(mHost->mRotation);
    }
}

void MapPartsRailRotator::updateInfo(s32 point) {
    s32 speedCalcType = -1;
    MR::getMapPartsArgSpeedCalcType(&speedCalcType, mHost, point);
    if (MR::isMapPartsRailSpeedCalcTypeTime(speedCalcType)) {
        s32 time = 0;
        MR::getMapPartsArgRailRotateTime(&time, mHost, point);
        f32 angle = 0.0f;
        MR::getMapPartsArgRailRotateAngle(&angle, mHost, point);
        mRotateSpeed = angle / time;
    } else {
        f32 speed = -1.0f;
        MR::getMapPartsArgRailRotateSpeed(&speed, mHost, point);
        mRotateSpeed = 0.01f * speed;
    }
    f32 angle = -1.0f;
    MR::getMapPartsArgRailRotateAngle(&angle, mHost, point);
    angle *= getJMapArgAngleFactor();
    mTargetAngle = angle;
    MR::getMapPartsArgRailRotateAxis(&mRotateAxis, mHost, point);
    MR::getMapPartsArgRailRotateType(&mRotateType, mHost, point);
    if (angle < 0.0f) {
        mRotateSpeed = 0.0f;
    }
    mAngle = 0.0f;
    updateHostRotateMtx();
}

bool MapPartsRailRotator::isReachedTargetAngle() const {
    return mTargetAngle <= MR::abs(mAngle);
}

void MapPartsRailRotator::calcRotateAxisDir(AxisType type, TVec3f* pAxis) const {
    switch (type) {
    case Axis_X:
        _2C.getXDir(*pAxis);
        break;
    case Axis_Y:
        _2C.getYDir(*pAxis);
        break;
    case Axis_Z:
        _2C.getZDir(*pAxis);
        break;
    }
}

void MapPartsRailRotator::updateRotateMtx(AxisType type, f32 angle) {
    TVec3f axis;
    calcRotateAxisDir(type, &axis);
    _5C.identity();
    _5C.zeroTrans();
    _5C.setRotate(axis, (PI / 180.0f) * angle);
    _5C.concat(_5C, _2C);
}

void MapPartsRailRotator::exeRotate() {
    if (isFirstStep()) {
        updateHostRotateMtx();
    }
    mAngle += mRotateSpeed;
    if (isReachedTargetAngle()) {
        updateRotateMtx(static_cast<AxisType>(mRotateAxis), mTargetAngle * MR::sign(mRotateSpeed));
        sendMsgToHost(0xCC);
        setNerve(&NrvMapPartsRailRotator::HostTypeDone::sInstance);
    } else {
        updateRotateMtx(static_cast<AxisType>(mRotateAxis), mAngle);
    }
}

f32 MapPartsRailRotator::getJMapArgAngleFactor() const {
    return 1.0f;
}
