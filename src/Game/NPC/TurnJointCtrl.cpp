#include "Game/NPC/TurnJointCtrl.hpp"
#include "Game/LiveActor/DynamicJointCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/JointController.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include <cmath>

namespace {
    void makeMtxRotVecDegree(MtxPtr pMtx, const TVec3f& rFrom, const TVec3f& rTo, f32 degree) {
        TVec3f axis;
        PSVECCrossProduct(&rFrom, &rTo, &axis);
        f32 dot = rFrom.dot(rTo);
        if (MR::normalizeOrZero(&axis)) {
            PSMTXIdentity(pMtx);
            return;
        }

        f32 maxAngle = __fabs(MR::toRadian(degree));
        f32 angle = acos(static_cast< f64 >(dot));
        angle = angle >= maxAngle ? maxAngle : angle;
        PSMTXRotAxisRad(pMtx, &axis, -angle);
    }
};  // namespace

TurnJointCtrl::TurnJointCtrl(LiveActor* pActor)
    : mHostActor(pActor), mControlRate(new JointCtrlRate()), mMaxYawDegree(45.0f), mMaxPitchUpDegree(22.5f), mMaxPitchDownDegree(22.5f),
      mStarePos(0.0f, 0.0f, 0.0f), _5C(1000.0f), _60(0.04f), _64(60), mEnabled(true) {
    mFace.mEnabled = false;
    mFace.mRate = 0.0f;
    mFace.mController = nullptr;
    mWaist.mEnabled = false;
    mWaist.mRate = 0.0f;
    mWaist.mController = nullptr;
    MR::extractMtxZDir(pActor->getBaseMtx(), &mStarePos);
    mStarePos += pActor->mPosition;
}

void TurnJointCtrl::init(f32 yaw, f32 pitchUp, f32 pitchDown) {
    mMaxYawDegree = yaw;
    mMaxPitchUpDegree = pitchUp;
    mMaxPitchDownDegree = pitchDown;
}

void TurnJointCtrl::addFace(const char* pName, f32 rate, AXIS pitchAxis, AXIS yawAxis, AXIS frontAxis) {
    mFace.mController = MR::createJointController< TurnJointCtrl >(this, mHostActor, pName, &TurnJointCtrl::updateJointMtxCallBackFace, nullptr);
    mFace.mRate = rate;
    mFace.mFrontAxis = frontAxis;
    mFace.mPitchAxis = pitchAxis;
    mFace.mYawAxis = yawAxis;
    mFace.mEnabled = true;
}

void TurnJointCtrl::addWaist(const char* pName, f32 rate, AXIS pitchAxis, AXIS yawAxis, AXIS frontAxis) {
    mWaist.mController = MR::createJointController< TurnJointCtrl >(this, mHostActor, pName, &TurnJointCtrl::updateJointMtxCallBackWaist, nullptr);
    mWaist.mRate = rate;
    mWaist.mFrontAxis = frontAxis;
    mWaist.mPitchAxis = pitchAxis;
    mWaist.mYawAxis = yawAxis;
    mWaist.mEnabled = true;
}

void TurnJointCtrl::startCtrl(s32 frames) {
    mEnabled = true;
    mControlRate->startCtrl(frames);
    mControlRate->update();
}

void TurnJointCtrl::endCtrl(s32 frames) {
    mEnabled = false;
    mControlRate->endCtrl(frames);
    mControlRate->update();
}

void TurnJointCtrl::validate() {
    startCtrl(0);
}

void TurnJointCtrl::invalidate() {
    endCtrl(0);
}

void TurnJointCtrl::setStarePos(const TVec3f& rPos) {
    if (!mEnabled && MR::isNearZero(mControlRate->_0, 0.001f)) {
        return;
    }

    if (MR::isNear(mHostActor, rPos, _5C)) {
        if (MR::isNearZero(mControlRate->_0, 0.001f)) {
            mControlRate->startCtrl(_64);
        }
    } else if (MR::isNearZero(mControlRate->_0 - 1.0f, 0.001f)) {
        mControlRate->endCtrl(_64);
    }

    MR::vecBlendSphere(mStarePos, rPos, &mStarePos, _60);
}

void TurnJointCtrl::update() {
    mControlRate->update();
}

void TurnJointCtrl::setCallBackFunction() {
    if (!mEnabled && MR::isNearZero(mControlRate->_0, 0.001f)) {
        return;
    }

    if (mFace.mEnabled) {
        mFace.mController->registerCallBack();
    }
    if (mWaist.mEnabled) {
        mWaist.mController->registerCallBack();
    }
}

bool TurnJointCtrl::updateJointMtxCallBackFace(TPos3f* pMtx, const JointControllerInfo&) {
    return updateJointMtxCallBack(pMtx, mFace);
}

bool TurnJointCtrl::updateJointMtxCallBackWaist(TPos3f* pMtx, const JointControllerInfo&) {
    return updateJointMtxCallBack(pMtx, mWaist);
}

void TurnJointCtrl::getMtxDir(TVec3f* pDir, const TPos3f* pMtx, AXIS axis) {
    switch (axis) {
    case X:
        pMtx->getXDir(*pDir);
        break;
    case Y:
        pMtx->getYDir(*pDir);
        break;
    case Z:
        pMtx->getZDir(*pDir);
        break;
    }
    MR::normalizeOrZero(pDir);
}

bool TurnJointCtrl::updateJointMtxCallBack(TPos3f* pMtx, const Ctrl& rCtrl) {
    if (!mEnabled && MR::isNearZero(mControlRate->_0, 0.001f)) {
        return false;
    }

    TVec3f position;
    pMtx->getTrans(position);
    TVec3f target(mStarePos);
    target -= position;
    if (MR::normalizeOrZero(&target)) {
        return false;
    }

    TPos3f result(*pMtx);
    result.setTrans(TVec3f(0.0f, 0.0f, 0.0f));
    TPos3f rotation;
    TVec3f pitchAxis;
    TVec3f yawAxis;
    TVec3f front;
    TVec3f projected;
    getMtxDir(&pitchAxis, pMtx, rCtrl.mPitchAxis);
    MR::vecKillElement(target, pitchAxis, &projected);
    if (!MR::normalizeOrZero(&projected)) {
        getMtxDir(&front, pMtx, rCtrl.mFrontAxis);
        TVec3f cross;
        PSVECCrossProduct(&target, &front, &cross);
        MR::normalizeOrZero(&cross);
        MR::vecBlendSphere(front, projected, &projected, rCtrl.mRate);
        if (pitchAxis.dot(cross) > 0.0f) {
            ::makeMtxRotVecDegree(rotation.toMtxPtr(), projected, front, mMaxPitchUpDegree * mControlRate->_0);
        } else {
            ::makeMtxRotVecDegree(rotation.toMtxPtr(), projected, front, mMaxPitchDownDegree * mControlRate->_0);
        }
        PSMTXConcat(rotation.toMtxPtr(), result.toMtxPtr(), result.toMtxPtr());
    }

    getMtxDir(&yawAxis, pMtx, rCtrl.mYawAxis);
    MR::vecKillElement(target, yawAxis, &projected);
    if (!MR::normalizeOrZero(&projected)) {
        getMtxDir(&front, pMtx, rCtrl.mFrontAxis);
        MR::vecBlendSphere(front, projected, &projected, rCtrl.mRate);
        ::makeMtxRotVecDegree(rotation.toMtxPtr(), projected, front, mMaxYawDegree * mControlRate->_0);
        PSMTXConcat(rotation.toMtxPtr(), result.toMtxPtr(), result.toMtxPtr());
    }

    result.setTrans(position);
    pMtx->set(result);
    return true;
}
