#include "Game/MapObj/SuperSpinDriver.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/MapObj/SpinDriverUtil.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "math_types.hpp"

/* it seems like this file was compiled with an earlier compiler version */

namespace {
    static f32 sCanBindTime = 90.0f;
};  // namespace

namespace NrvSuperSpinDriver {
    NEW_NERVE(SuperSpinDriverNrvTryDemo, SuperSpinDriver, TryDemo);
    NEW_NERVE(SuperSpinDriverNrvEmptyNonActive, SuperSpinDriver, EmptyNonActive);
    NEW_NERVE(SuperSpinDriverNrvEmptyAppear, SuperSpinDriver, EmptyAppear);
    NEW_NERVE(SuperSpinDriverNrvEmptyWait, SuperSpinDriver, EmptyWait);
    NEW_NERVE(SuperSpinDriverNrvNonActive, SuperSpinDriver, NonActive);
    NEW_NERVE(SuperSpinDriverNrvAppear, SuperSpinDriver, Appear);
    NEW_NERVE(SuperSpinDriverNrvWait, SuperSpinDriver, Wait);
    NEW_NERVE(SuperSpinDriverNrvCapture, SuperSpinDriver, Capture);
    NEW_NERVE(SuperSpinDriverNrvShootStart, SuperSpinDriver, ShootStart);
    NEW_NERVE_ONEND(SuperSpinDriverNrvShoot, SuperSpinDriver, Shoot, Shoot);
    NEW_NERVE(SuperSpinDriverNrvCoolDown, SuperSpinDriver, CoolDown);
};  // namespace NrvSuperSpinDriver

bool SuperSpinDriver::tryEndCapture() {
    if (MR::isGreaterStep(this, 60) && PSVECDistance(&_C4, &mPosition) < 15.0f) {
        cancelBind();
        _174 = 0;
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvWait::sInstance);
        return true;
    }

    return false;
}

bool SuperSpinDriver::tryForceCancel() {
    if (_8C == nullptr) {
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvCoolDown::sInstance);
        return true;
    }

    return false;
}

bool SuperSpinDriver::tryShootStart() {
    bool isSwingOrPointed = MR::isPadSwing(WPAD_CHAN0) || MR::isPlayerPointedBy2POnTriggerButton();

    if (isSwingOrPointed) {
        MR::deleteEffect(this, "SuperSpinDriverLight");
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvShootStart::sInstance);
        return true;
    }

    return false;
}

bool SuperSpinDriver::tryShoot() {
    if (MR::isGreaterStep(this, 45)) {
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvShoot::sInstance);
        return true;
    }

    return false;
}

bool SuperSpinDriver::tryEndShoot() {
    if (MR::isGreaterEqualStep(this, _150)) {
        endBind();
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvCoolDown::sInstance);
        return true;
    }

    return false;
}

bool SuperSpinDriver::tryEndCoolDown() {
    if (MR::isGreaterStep(this, 60) && _178 == 0) {
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvWait::sInstance);
        return true;
    }

    return false;
}

bool SuperSpinDriver::trySwitchOff() {
    if (MR::isValidSwitchAppear(this) && !MR::isOnSwitchAppear(this)) {
        kill();
        return true;
    }

    return false;
}

void SuperSpinDriver::requestAppear() {
    MR::invalidateClipping(this);

    if (mSpinDriverCamera->isUseAppearCamera(this)) {
        MR::requestStartDemo(this, "出現", &NrvSuperSpinDriver::SuperSpinDriverNrvAppear::sInstance,
                             &NrvSuperSpinDriver::SuperSpinDriverNrvTryDemo::sInstance);
    } else {
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvAppear::sInstance);
    }
}

void SuperSpinDriver::requestEmptyAppear() {
    MR::invalidateClipping(this);

    if (mSpinDriverCamera->isUseAppearCamera(this)) {
        MR::requestStartDemo(this, "出現", &NrvSuperSpinDriver::SuperSpinDriverNrvEmptyAppear::sInstance,
                             &NrvSuperSpinDriver::SuperSpinDriverNrvTryDemo::sInstance);
    } else {
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvEmptyAppear::sInstance);
    }
}

void SuperSpinDriver::requestActive() {
    if (isNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvNonActive::sInstance)) {
        requestAppear();
    } else if (isNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvEmptyNonActive::sInstance)) {
        requestEmptyAppear();
    }
}

void SuperSpinDriver::requestHide() {
    if (!MR::isDead(this)) {
        if (_8C != nullptr) {
            endBind();
        }

        makeActorDead();
    }
}

void SuperSpinDriver::requestShow() {
    if (MR::isDead(this)) {
        makeActorAppeared();
    }
}

void SuperSpinDriver::exeTryDemo() {}

void SuperSpinDriver::exeEmptyNonActive() {
    if (MR::isFirstStep(this)) {
        MR::validateClipping(this);
    }

    if (isRightToUse()) {
        onUse();
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvNonActive::sInstance);
    }
}

void SuperSpinDriver::exeEmptyAppear() {
    if (MR::isFirstStep(this)) {
        mSpinDriverCamera->startAppearCamera(this, _100, _E8, mPosition);

        if (!_17F) {
            MR::startSystemSE("SE_SY_SPIN_DRIVER_APPEAR");
            MR::startSound(this, "SE_OJ_S_SPIN_DRV_APPEAR");
        }
    }

    if (MR::isBckStopped(this)) {
        s32 frames = mSpinDriverCamera->getAppearCameraFrames();

        if (MR::isGreaterStep(this, frames)) {
            setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvEmptyWait::sInstance);

            if (mSpinDriverCamera->isUseAppearCamera(this)) {
                mSpinDriverCamera->endAppearCamera(this);
                MR::endDemoWaitCameraInterpolating(this, "出現");
            }
        }
    }
}

void SuperSpinDriver::exeEmptyWait() {
    if (MR::isFirstStep(this)) {
        MR::validateClipping(this);
    }

    if (isRightToUse()) {
        onUse();
        setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvWait::sInstance);
    }
}

void SuperSpinDriver::exeNonActive() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "NonActive", nullptr);
        MR::validateClipping(this);
    }

    addSwingSignRotateY();
}

void SuperSpinDriver::exeAppear() {
    if (MR::isFirstStep(this)) {
        mSpinDriverCamera->startAppearCamera(this, _100, _E8, mPosition);

        if (!_17F) {
            MR::startSystemSE("SE_SY_SPIN_DRIVER_APPEAR");
            MR::startSound(this, "SE_OJ_S_SPIN_DRV_APPEAR");
        }

        MR::startBck(this, "Appear", nullptr);
        _144 = 0.0f;
    }

    if (MR::isBckStopped(this)) {
        s32 frames = mSpinDriverCamera->getAppearCameraFrames();

        if (MR::isGreaterStep(this, frames)) {
            setNerve(&NrvSuperSpinDriver::SuperSpinDriverNrvWait::sInstance);

            if (mSpinDriverCamera->isUseAppearCamera(this)) {
                mSpinDriverCamera->endAppearCamera(this);
                MR::endDemoWaitCameraInterpolating(this, "出現");
            }
        }
    }
}

void SuperSpinDriver::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startBck(this, "Wait", nullptr);
        MR::validateClipping(this);
    }

    if (MR::isGreaterStep(this, sCanBindTime)) {
        addSwingSignRotateY();
    }

    if (_178 > 0) {
        MR::startLevelSound(this, "SE_OJ_LV_S_SPIN_DRV_SHINE");

        if (!_17C) {
            MR::emitEffect(this, "SuperSpinDriverLight");
            MR::startCSSound("CS_SPIN_BIND", nullptr, 0);
        }

    } else {
        MR::deleteEffect(this, "SuperSpinDriverLight");
    }

    trySwitchOff();
}

void SuperSpinDriver::exeCapture() {
    if (tryForceCancel()) {
        MR::deleteEffect(this, "SuperSpinDriverLight");
    } else {
        if (MR::isFirstStep(this)) {
            MR::emitEffect(this, "SuperSpinDriverLight");
            MR::startBckPlayer("SpinDriverWait", "SuperSpinDriverCapture");
            _144 = 0.0f;
        }

        MR::startLevelSound(this, "SE_OJ_LV_S_SPIN_DRV_SHINE");
        MR::startLevelSound(this, "SE_OJ_LV_SPIN_DRV_CAPTURE");
        moveBindPosToCenter();
        f32 rate = MR::calcNerveRate(this, 60);
        _134 = rate;
        updateBindActorPoseToShoot((f64)rate);
        _144 += 0.0040f;
        MR::tryRumblePadWeak(this, 0);
        _178 = 60;

        if (!tryEndCapture()) {
            if (!tryShootStart()) {
                return;
            }
        }
    }
}

void SuperSpinDriver::exeShootStart() {
    if (!tryForceCancel()) {
        if (MR::isFirstStep(this)) {
            MR::startSound(_8C, "SE_PM_SPIN_ATTACK", -1, -1);
            MR::startCSSound("CS_SPIN_DRIVE_LONG", "SE_SY_CS_S_SPIN_DRV_START", 0);
            MR::startSound(this, "SE_OJ_S_SPIN_DRV_PREP_JUMP", -1, -1);

            if (MR::isInAreaObj("Water", mPosition)) {
                MR::startSound(this, "SE_PM_SPIN_DRV_IN_WATER_1", -1, -1);
            }

            MR::deleteEffectAll(this);
            MR::emitEffect(this, "SuperSpinDriverStart");
            MR::startBck(this, "Start", nullptr);
            MR::startBckPlayer("SuperSpinDriverStart", "SuperSpinDriverShoot");
            _118 = _C4;
            updateBindActorPoseToShoot(1.0f);
        }

        f32 rate = MR::calcNerveRate(this, 15);
        TVec3f start(_118);
        start *= 1.0f - rate;
        TVec3f end(mPosition);
        end *= rate;
        _C4 = end;
        _C4 += start;
        _144 *= 0.8f;
        MR::tryRumblePadMiddle(this, 0);
        tryShoot();
    }
}

void SuperSpinDriver::exeShoot() {
    if (!tryForceCancel()) {
        if (MR::isFirstStep(this)) {
            calcShootMotionTime();
            MR::validateHitSensor(this, "body");

            if (MR::hasME()) {
                MR::startSystemME("ME_MAGIC_L");
            } else {
                MR::startSystemSE("SE_SY_S_SPIN_DRV_ME_ALT", -1, -1);
            }

            MR::startSound(this, "SE_OJ_S_SPIN_DRV_JUMP", -1, -1);
            MR::startSound(_8C, "SE_PV_JUMP_JOY", -1, -1);

            if (MR::isInAreaObj("Water", mPosition)) {
                MR::startSound(this, "SE_PM_SPIN_DIV_IN_WATER_2", -1, -1);
            }

            MR::startBckPlayer("SpaceFlyStart", "SuperSpinDriverFlyStart");
            MR::shakeCameraStrong();
            MR::tryRumblePadVeryStrong(this, 0);
            _DC.set< f32 >(_100);
            mOperateRing->reset();
            startPathDraw();
        }

        f32 rate = MR::calcNerveRate(this, _150);
        updatePathDraw(rate);
        updateOperateRate();
        updateBindPosition(rate);

        if (!MR::isNearZero(_DC, 0.001f)) {
            TVec3f operateDir(mOperateRing->mDirection);
            operateDir *= mOperateRing->mRadiusRate;
            operateDir *= 0.8f;

            TVec3f headDir(_DC);
            headDir += operateDir;

            TVec3f normalized;
            MR::normalize(headDir, &normalized);
            turnBindHead(normalized, 0.4f);
        }

        if (_154 <= getNerveStep() && getNerveStep() <= _158) {
            f32 rotateRate = MR::normalize(static_cast<f32>(getNerveStep()), static_cast<f32>(_154), static_cast<f32>(_158));
            _138 = _13C * MR::getEaseOutValue(rotateRate, 0.0f, 1.0f, 1.0f);
        }

        f32 fallRate = MR::normalize(static_cast<f32>(getNerveStep()), static_cast<f32>(_158), static_cast<f32>(_15C));
        _148 = PI * fallRate;
        updateShootMotion();
        mSpinDriverCamera->update(_DC, _10C);
        tryEndShoot();
    }
}

void SuperSpinDriver::endShoot() {
    MR::invalidateHitSensor(this, "body");
    mOperateRing->reset();
}

void SuperSpinDriver::exeCoolDown() {
    // BUG, is supposed to be a conditional to call tryEndCoolDown
    if (MR::isFirstStep(this)) {
    }

    if (!tryEndCoolDown()) {
        trySwitchOff();
    }
}

void SuperSpinDriver::updateShootMotion() {
    if (MR::isStep(this, _154)) {
        MR::startBckPlayer("SpaceFlyLoop", "SuperSpinDriverFlyLoop");
    }

    if (MR::isLessStep(this, _158)) {
        MR::startLevelSound(_8C, "SE_PM_LV_S_SPIN_DRV_FLY");
    }

    if (MR::isStep(this, _158)) {
        MR::startBckPlayer("SpaceFlyEnd", "SuperSpinDriverFlyEnd");
        MR::startSound(_8C, "SE_PM_S_SPIN_DRV_COOL_DOWN");
        MR::startSound(_8C, "SE_PV_JUMP_S");
    }

    if (MR::isStep(this, _15C)) {
        MR::startBckPlayer("Fall", "SuperSpinDriverFall");
    }
}

void SuperSpinDriver::cancelBind() {
    if (_8C != nullptr) {
        MR::endBindAndPlayerJump(this, _D0, 0);
        _8C = nullptr;
    }

    mSpinDriverCamera->cancel();
}

void SuperSpinDriver::endBind() {
    MR::endBindAndSpinDriverJump(this, _D0);
    _8C = nullptr;
    mSpinDriverCamera->end();
}

void SuperSpinDriver::updateBindActorMatrix() {
    TRot3f ringMtx;
    ringMtx.identity();
    ringMtx.setEulerY(_138);
    ringMtx.mMtx[0][3] = 0.0f;
    ringMtx.mMtx[1][3] = -75.0f * mOperateRing->mRadiusRate;
    ringMtx.mMtx[2][3] = 0.0f;

    TMtx34f rollMtx;
    rollMtx.identity();
    f32 sinV = sin(_148);
    f32 cosV = cos(_148);
    rollMtx.mMtx[0][0] = 1.0f;
    rollMtx.mMtx[0][1] = 0.0f;
    rollMtx.mMtx[0][2] = 0.0f;
    rollMtx.mMtx[1][0] = 0.0f;
    rollMtx.mMtx[1][1] = cosV;
    rollMtx.mMtx[1][2] = -sinV;
    rollMtx.mMtx[2][0] = 0.0f;
    rollMtx.mMtx[2][1] = sinV;
    rollMtx.mMtx[2][2] = cosV;

    TPos3f pose;
    pose.setQuat(_B4);
    pose.setTrans(_C4);
    pose.concat(pose, rollMtx);
    pose.concat(pose, ringMtx);
    MR::setBaseTRMtx(_8C, pose);
}

void SuperSpinDriver::updateBindActorPoseToShoot(f32 rate) {
    TPos3f matrix;
    matrix.identity();
    MR::makeMtxUpFront(&matrix, _100, _E8);

    TQuat4f quat;
    matrix.getQuat(quat);
    _B4.slerp(_A4, quat, rate);
}

void SuperSpinDriver::turnBindHead(const TVec3f& rDir, f32 rate) {
    TVec3f currentDir;
    _B4.getYDir(currentDir);

    TQuat4f rotate;
    rotate.setRotate(currentDir, rDir, rate);
    PSQUATMultiply(&rotate, &_B4, &_B4);
    _B4.normalize();
}

void SuperSpinDriver::moveBindPosToCenter() {
    TVec3f center(mPosition);
    _C4 += _D0;

    TVec3f toCenter = center - _C4;
    f32 distance;
    MR::separateScalarAndDirection(&distance, &toCenter, toCenter);

    f32 rate = distance / 120.0f;
    TVec3f accelBase(toCenter);
    accelBase *= 1.5f;
    TVec3f accel(accelBase);
    accel *= rate;
    _D0 += accel;
    _D0.x *= 0.8f;
    _D0.y *= 0.8f;
    _D0.z *= 0.8f;
}

void SuperSpinDriver::startPathDraw() {
    if (mPathDrawer != nullptr) {
        if (MR::isDead(mPathDrawer)) {
            mPathDrawer->appear();
            MR::emitEffect(this, "EndGlow");
        }
    }
}

void SuperSpinDriver::endPathDraw() {
    if (mPathDrawer != nullptr) {
        if (!MR::isDead(mPathDrawer)) {
            MR::emitEffect(this, "EndGlow");
        }
    }
}

void SuperSpinDriver::updatePathDraw(f32 rate) {
    if (mPathDrawer != nullptr) {
        mPathDrawer->setCoord(rate);

        if (_160 >= 0) {
            MR::updateStorageSpinDriverPathDrawRange(_160, mPathDrawer->_B0);
        }
    }
}

void SuperSpinDriver::updateOperateRate() {
    f32 rate = 0.0f;

    if (_17D && _154 > 0 && _158 > 0 && _15C > 0) {
        if (MR::isLessStep(this, _154)) {
            rate = 0.0f;
        } else if (MR::isLessStep(this, _158)) {
            s32 fadeFrames = _15C - _158;

            if (fadeFrames > 5) {
                fadeFrames = 5;
            }

            rate = MR::normalize(static_cast<f32>(getNerveStep()), static_cast<f32>(_154), static_cast<f32>(_154 + fadeFrames));
        } else if (MR::isLessStep(this, _15C)) {
            rate = 1.0f - MR::normalize(static_cast<f32>(getNerveStep()), static_cast<f32>(_158), static_cast<f32>(_15C));
        }
    }

    mOperateRing->setRadiusRate(rate);
}

void SuperSpinDriver::updateBindPosition(f32 rate) {
    mShootPath->calcPosition(&_10C, rate);

    TVec3f direction;
    mShootPath->calcDirection(&direction, rate, 0.01f);

    if (!MR::isNearZero(direction, 0.001f)) {
        _DC = direction;
    }

    mOperateRing->update(_10C, _DC);

    TVec3f oldPos(_C4);
    TVec3f bindPos(_10C);
    bindPos += mOperateRing->_A4;
    _C4.set< f32 >(bindPos);
    _D0 = _C4 - oldPos;
}

void SuperSpinDriver::calcShootMotionTime() {
    if (_150 < 20) {
        _154 = -1;
        _158 = -1;
        _15C = 0;
        return;
    }

    s32 endStartFrame = cSpaceFlyEndFrame + 20;

    if (_150 < endStartFrame) {
        _154 = -1;
        _158 = 0;
        _15C = _150 - 20;
        return;
    }

    _158 = _150 - endStartFrame;
    _15C = _150 - 20;

    s32 startFrame = static_cast<s32>(0.2f * _150);
    if (startFrame > 90) {
        startFrame = 90;
    }

    _154 = cSpaceFlyStartFrame * (startFrame / cSpaceFlyStartFrame);

    if (_158 <= _154) {
        _154 = 0;
    }

    _138 = 0.0f;
    s32 rotateCount = static_cast<s32>((0.05f * static_cast<f32>(_158 - _154)) / TWO_PI);
    _13C = TWO_PI * static_cast<f32>(rotateCount);
}

void SuperSpinDriver::addSwingSignRotateY() {
    bool isSwingOrPointed = MR::isPadSwing(WPAD_CHAN0) || MR::isPlayerPointedBy2POnTriggerButton();

    if (isSwingOrPointed) {
        _144 += 0.1f;

        if (_144 > 0.23f) {
            _144 = 0.23f;
        }
    }
}

void SuperSpinDriver::onUse() {
    if (mEmptyModel != nullptr) {
        mEmptyModel->kill();
    }

    MR::showModel(this);
}

void SuperSpinDriver::offUse() {
    if (mEmptyModel != nullptr) {
        mEmptyModel->appear();
    }

    MR::hideModelAndOnCalcAnim(this);
}

bool SuperSpinDriver::isNeedEmptyModel() const {
    if (mColor == 1) {
        return true;
    } else {
        return false;
    }
}

bool SuperSpinDriver::isRightToUse() const {
    if (mColor == 1) {
        return MR::isOnGameEventFlagGreenDriver();
    } else {
        return true;
    }
}

SuperSpinDriver::~SuperSpinDriver() {}

namespace MR {
    NameObj* createSuperSpinDriverYellow(const char* pName) { return new SuperSpinDriver(pName, 0); }

    NameObj* createSuperSpinDriverGreen(const char* pName) { return new SuperSpinDriver(pName, 1); }

    NameObj* createSuperSpinDriverPink(const char* pName) { return new SuperSpinDriver(pName, 2); }
};  // namespace MR
