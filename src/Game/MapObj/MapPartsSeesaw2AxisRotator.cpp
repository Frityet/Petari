#include "resource/TextEncoding.hpp"
#include "Game/MapObj/MapPartsSeesaw2AxisRotator.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/MapPartsUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace NrvMapPartsSeesaw2AxisRotator {
    NERVE_DECL_NULL(HostTypeWait);
    NERVE(HostTypeStay);
    NEW_NERVE(HostTypeMove, MapPartsSeesaw2AxisRotator, Move);
    NEW_NERVE(HostTypeHipDrop, MapPartsSeesaw2AxisRotator, HipDrop);
    INIT_NERVE(HostTypeWait);
    INIT_NERVE(HostTypeStay);

    void HostTypeStay::execute(Spine* pSpine) const {
        MapPartsSeesaw2AxisRotator* pHost = static_cast<MapPartsSeesaw2AxisRotator*>(pSpine->mExecutor);
        if (MR::isOnPlayer(MR::getBodySensor(pHost->mHost))) {
            pHost->setNerve(&HostTypeMove::sInstance);
        }
    }
};

MapPartsSeesaw2AxisRotator::MapPartsSeesaw2AxisRotator(LiveActor* pActor, const char* pSoundName, f32 soundSpeedThreshold)
    : MapPartsRotatorBase(pActor, CP932("シーソー(2軸)")), mRotateAngle(0.0f), mInertia(500.0f), mRestoreForce(100.0f), mBaseUp(0.0f, 1.0f, 0.0f),
      mIsPlayerOn(false), mRotateSpeed(0.0f), mSoundName(pSoundName), mSoundSpeedThreshold(soundSpeedThreshold) {
    mRotateMtx.identity();
    mBaseMtx.identity();
    mBaseMtxInv.identity();
}

MapPartsSeesaw2AxisRotator::~MapPartsSeesaw2AxisRotator() {
}

void MapPartsSeesaw2AxisRotator::init(const JMapInfoIter& rIter) {
    initNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeWait::sInstance);
    MR::getMapPartsArgRotateAngle(&mRotateAngle, rIter);
    s32 inertia = 0;
    MR::getMapPartsArgRotateAccelType(&inertia, rIter);
    if (inertia > 0) {
        mInertia = inertia;
    }
    s32 restoreForce = 0;
    MR::getMapPartsArgRotateStopTime(&restoreForce, rIter);
    mRestoreForce = restoreForce;
}

void MapPartsSeesaw2AxisRotator::start() {
    mBaseMtx.set(mHost->getBaseMtx());
    mBaseMtxInv.invert(mBaseMtx);
    mBaseMtx.getYDir(mBaseUp);
    MR::normalize(&mBaseUp);
    mRotateMtx.set(mBaseMtx);
    mRotateMtx.zeroTrans();
    setNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeStay::sInstance);
}

void MapPartsSeesaw2AxisRotator::end() {
}

bool MapPartsSeesaw2AxisRotator::isWorking() const {
    return !isNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeWait::sInstance);
}

bool MapPartsSeesaw2AxisRotator::isMoving() const {
    return isWorking();
}

bool MapPartsSeesaw2AxisRotator::receiveMsg(u32 msg) {
    if (msg == 4) {
        if (!MR::isOnPlayer(MR::getBodySensor(mHost))) {
            return false;
        }
        mIsPlayerOn = true;
        if (isNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeMove::sInstance) || isNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeStay::sInstance)) {
            setNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeHipDrop::sInstance);
            return true;
        }
    }
    return false;
}

void MapPartsSeesaw2AxisRotator::rotate() {
    TVec3f playerPos(*MR::getPlayerPos());
    mBaseMtxInv.mult(playerPos, playerPos);
    f32 distance = PSVECMag(&playerPos);
    TVec3f axis;
    PSVECCrossProduct(&mBaseUp, &playerPos, &axis);
    MR::normalize(&axis);
    TPos3f rotateMtx;
    rotateMtx.identity();
    rotateMtx.setRotate(axis, (PI / 180.0f) * mRotateAngle);
    TPos3f targetMtx;
    targetMtx.concat(mBaseMtx, rotateMtx);
    targetMtx.zeroTrans();
    f32 oldSpeed = mRotateSpeed;
    f32 rate = MR::clamp((0.001f * distance) / getInertiaConst(), 0.0f, 1.0f);
    mRotateSpeed = rate - oldSpeed;
    if (mSoundName != nullptr && mSoundSpeedThreshold < MR::abs(mRotateSpeed)) {
        MR::startLevelSound(mHost, mSoundName, -1, -1, -1);
    }
    MR::blendMtxRotate(mRotateMtx.toMtxPtr(), targetMtx.toMtxPtr(), rate, mRotateMtx.toMtxPtr());
}

void MapPartsSeesaw2AxisRotator::restoreMove() {
    f32 rate = MR::clamp(0.0001f * mRestoreForce, 0.0f, 1.0f);
    TPos3f targetMtx;
    targetMtx.set(mBaseMtx);
    targetMtx.zeroTrans();
    MR::blendMtxRotate(mRotateMtx.toMtxPtr(), targetMtx.toMtxPtr(), rate, mRotateMtx.toMtxPtr());
}

f32 MapPartsSeesaw2AxisRotator::getInertiaConst() const {
    if (isNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeHipDrop::sInstance)) {
        return 0.1f * mInertia;
    }
    return mInertia;
}

void MapPartsSeesaw2AxisRotator::exeMove() {
    if (MR::isOnPlayer(MR::getBodySensor(mHost))) {
        rotate();
    }
    restoreMove();
}

void MapPartsSeesaw2AxisRotator::exeHipDrop() {
    if (!mIsPlayerOn) {
        setNerve(&NrvMapPartsSeesaw2AxisRotator::HostTypeMove::sInstance);
        mIsPlayerOn = false;
    } else {
        rotate();
        mIsPlayerOn = false;
    }
}
