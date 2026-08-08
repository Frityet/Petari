#include "Game/Map/SphereSelectorHandle.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util.hpp"

namespace {
    const volatile f32 cZero = 0.0f;
    static s32 cBgmAppearState = 2;
    static u32 cBgmAppearFrames = 60;
    static s32 cBgmDisappearState = 1;
    static u32 cBgmDisappearFrames = 90;
    static s32 cBgmRotateState = 4;
    static u32 cBgmRotateFrames = 60;
    static s32 cBgmNotRotateState = 3;
    static u32 cBgmNotRotateFrames = 30;
    static s32 cBgmConfirmState = 6;
    static u32 cBgmConfirmFrames = 60;
    static s32 cBgmNotConfirmState = 5;
    static u32 cBgmNotConfirmFrames = 60;
};  // namespace

namespace NrvSphereSelectorHandle {
    NERVE(SphereSelectorHandleNrvWait);
    NERVE(SphereSelectorHandleNrvHold);
    NERVE(SphereSelectorHandleNrvSpin);
    NERVE(SphereSelectorHandleNrvDemoRotate);
    NERVE(SphereSelectorHandleNrvDisappear);
    NERVE(SphereSelectorHandleNrvInvalidRotate);
    NERVE(SphereSelectorHandleNrvGalaxyConfirmStart);
    NERVE(SphereSelectorHandleNrvGalaxyConfirmWait);
    NERVE(SphereSelectorHandleNrvGalaxyConfirmCancel);
    NERVE(SphereSelectorHandleNrvIdleForFileSelect);
    NERVE(SphereSelectorHandleNrvIdleEndForFileSelect);

    INIT_NERVE(SphereSelectorHandleNrvWait);
    INIT_NERVE(SphereSelectorHandleNrvHold);
    INIT_NERVE(SphereSelectorHandleNrvSpin);
    INIT_NERVE(SphereSelectorHandleNrvDemoRotate);
    INIT_NERVE(SphereSelectorHandleNrvDisappear);
    INIT_NERVE(SphereSelectorHandleNrvInvalidRotate);
    INIT_NERVE(SphereSelectorHandleNrvGalaxyConfirmStart);
    INIT_NERVE(SphereSelectorHandleNrvGalaxyConfirmWait);
    INIT_NERVE(SphereSelectorHandleNrvGalaxyConfirmCancel);
    INIT_NERVE(SphereSelectorHandleNrvIdleForFileSelect);
    INIT_NERVE(SphereSelectorHandleNrvIdleEndForFileSelect);
};  // namespace NrvSphereSelectorHandle

SphereSelectorHandle::SphereSelectorHandle(const char* pName)
    : LiveActor(pName), mIsFileSelect(false), mFrontDir(0.0f, 0.0f, -1.0f), mRotateSpeed(0.0f), mPrevRotateSpeed(0.0f), mTiltSpeed(0.0f),
      mPrevTiltSpeed(0.0f), mRotateAxis(1.0f, 0.0f, 0.0f), mUpDir(0.0f, 1.0f, 0.0f), mPointerOffscreenStep(0), mConfirmPosition(gZeroVec), _11C(0.0f),
      _120(1.0f), _124(0.0f), mIsBgmRotating(false) {
    clearPointerVelocity();
    mBaseMtx.identity();
}

void SphereSelectorHandle::init(const JMapInfoIter& rIter) {
    MR::connectToScene(this, MR::MovementType_Environment, MR::CalcAnimType_MapObj, -1, -1);
    MR::getJMapInfoArg0NoInit(rIter, &mIsFileSelect);
    MR::invalidateClipping(this);
    initNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance);
    MR::tryRegisterDemoCast(this, rIter);
    MR::registerDemoSimpleCastAll(this);
    SphereSelectorFunction::registerTarget(this);
    SphereSelectorFunction::setHandle(this);
    makeActorDead();
}

void SphereSelectorHandle::appear() {
    LiveActor::appear();
    TVec3f camZ = MR::getCamZdir();
    mFrontDir.negate(camZ);
    mFrontDir.y = 0.0f;
    MR::normalize(&mFrontDir);
    resetRotateParam();
    mPointerOffscreenStep = 0;
    clearPointerVelocity();
    mRotation.zero();

    TVec3f crossUp(0.0f, 1.0f, 0.0f);
    mRotateAxis.cross(crossUp, mFrontDir);
    TVec3f rotateUp(0.0f, 1.0f, 0.0f);
    MR::rotateVecDegree(&mUpDir, rotateUp, mRotateAxis, 40.0f);
    MR::normalize(&mUpDir);

    MR::setStageBGMState(cBgmAppearState, cBgmAppearFrames);
    setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance);
}

bool SphereSelectorHandle::isPointing() const {
    return MR::isStarPointerInScreen(WPAD_CHAN0);
}

bool SphereSelectorHandle::isHolding() const {
    return isNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvHold::sInstance);
}

void SphereSelectorHandle::validateRotate() {
    setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance);
}

void SphereSelectorHandle::invalidateRotate() {
    setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvInvalidRotate::sInstance);
}

void SphereSelectorHandle::control() {
    rotateAxisY();
    rotateAxisX();
    updateBaseMtx();
    changeBgmRotateState();
    playRotateSE();

    if (MR::isStarPointerInScreen(WPAD_CHAN0) || MR::isDemoActive()) {
        mPointerOffscreenStep = 0;
    } else {
        mPointerOffscreenStep++;
    }
}

bool SphereSelectorHandle::receiveOtherMsg(u32 msg, HitSensor*, HitSensor*) {
    if (SphereSelectorFunction::isMsgSelectStart(msg)) {
        appear();
        return true;
    }

    if (SphereSelectorFunction::isMsgSelectEnd(msg)) {
        setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvDisappear::sInstance);
        return true;
    }

    if (SphereSelectorFunction::isMsgConfirmStart(msg)) {
        if (mIsFileSelect) {
            setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvIdleForFileSelect::sInstance);
        } else {
            setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvGalaxyConfirmStart::sInstance);
        }
        return true;
    }

    if (SphereSelectorFunction::isMsgConfirmCancel(msg)) {
        if (mIsFileSelect) {
            setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvIdleEndForFileSelect::sInstance);
        } else {
            setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvGalaxyConfirmCancel::sInstance);
        }
        return true;
    }

    if (SphereSelectorFunction::isMsgTargetSelected(msg) && (isNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance) ||
                                                             isNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvSpin::sInstance) ||
                                                             isNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvDemoRotate::sInstance))) {
        setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvHold::sInstance);
        return true;
    }

    return false;
}

bool SphereSelectorHandle::tryRelease() {
    if (!SphereSelectorFunction::isPadButton()) {
        setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvSpin::sInstance);
        return true;
    }

    return false;
}

void SphereSelectorHandle::clearPointerVelocity() {
    for (s32 i = 0; i < 3; i++) {
        mPointerVelocity[i].set(0.0f, 0.0f);
    }
}

void SphereSelectorHandle::stackPointerVelocity() {
    for (s32 i = 1; i < 3; i++) {
        mPointerVelocity[i].set(mPointerVelocity[i - 1]);
    }

    mPointerVelocity[0].set(*MR::getStarPointerScreenVelocity(WPAD_CHAN0));
}

TVec2f* SphereSelectorHandle::getPointerVelocity() {
    s32 index = 0;
    for (s32 i = 1; i < 3; i++) {
        if (mPointerVelocity[index].length() < mPointerVelocity[i].length()) {
            index = i;
        }
    }

    return &mPointerVelocity[index];
}

void SphereSelectorHandle::resetRotateParam() {
    mRotateSpeed = 0.0f;
    mPrevRotateSpeed = 0.0f;
    mTiltSpeed = 0.0f;
    mPrevTiltSpeed = 0.0f;
}

void SphereSelectorHandle::rotateAxisY() {
    MR::clampBoth(&mRotateSpeed, mPrevRotateSpeed - 0.2f, mPrevRotateSpeed + 0.2f);
    MR::clampBoth(&mRotateSpeed, -5.0f, 5.0f);
    mPrevRotateSpeed = mRotateSpeed;

    f32 zero0 = cZero;
    f32 rotate = fmod(360.0f + ((mRotation.y + mRotateSpeed) - zero0), 360.0);
    f32 zero1 = cZero;
    mRotation.y = zero1 + rotate;
}

void SphereSelectorHandle::rotateAxisX() {
    MR::clampBoth(&mTiltSpeed, mPrevTiltSpeed - 0.2f, mPrevTiltSpeed + 0.2f);
    MR::clampBoth(&mTiltSpeed, -2.0f, 2.0f);
    mPrevTiltSpeed = mTiltSpeed;

    MR::rotateVecDegree(&mUpDir, mRotateAxis, mTiltSpeed);
    MR::normalize(&mUpDir);

    TVec3f lower;
    TVec3f lowerUp(0.0f, 1.0f, 0.0f);
    MR::rotateVecDegree(&lower, lowerUp, mRotateAxis, -15.0f);

    TVec3f upper;
    TVec3f upperUp(0.0f, 1.0f, 0.0f);
    MR::rotateVecDegree(&upper, upperUp, mRotateAxis, 80.0f);

    if (mUpDir.dot(mFrontDir) < lower.dot(mFrontDir)) {
        mUpDir.set(lower);
    } else if (upper.dot(mFrontDir) < mUpDir.dot(mFrontDir)) {
        mUpDir.set(upper);
    }
}

void SphereSelectorHandle::updateBaseMtx() {
    TPos3f posture;
    MR::makeMtxUpFront(&posture, mUpDir, mFrontDir);
    posture.zeroTrans();

    TVec3f axis(0.0f, 1.0f, 0.0f);
    TPos3f rotation;
    rotation.makeRotate(axis, mRotation.y * 0.017453292f);
    mBaseMtx.concat(posture, rotation);
    mBaseMtx.setTrans(mPosition);
}

void SphereSelectorHandle::changeBgmRotateState() {
    if (MR::abs(mTiltSpeed) > 0.03f || MR::abs(mRotateSpeed) > 0.03f) {
        if (!mIsBgmRotating) {
            MR::setStageBGMState(cBgmRotateState, cBgmRotateFrames);
        }
        mIsBgmRotating = true;
    } else {
        if (mIsBgmRotating) {
            MR::setStageBGMState(cBgmNotRotateState, cBgmNotRotateFrames);
        }
        mIsBgmRotating = false;
    }
}

void SphereSelectorHandle::playRotateSE() {
    if (MR::abs(mTiltSpeed) <= 0.03f && MR::abs(mRotateSpeed) <= 0.03f) {
        return;
    }

    f32 level = MR::abs(mTiltSpeed) * 0.5f;
    f32 rotateLevel = MR::abs(mRotateSpeed) / 5.0f;
    if (level <= rotateLevel) {
        level = rotateLevel;
    }
    if (level > 1.0f) {
        level = 1.0f;
    }

    MR::startAtmosphereLevelSE("SE_AT_LV_ASTRO_DOME_WIND_1", static_cast< s32 >(100.0f * level), -1);
    if (MR::abs(mRotateSpeed) >= 4.0f || MR::abs(mTiltSpeed) >= 1.6f) {
        MR::startAtmosphereLevelSE("SE_AT_LV_ASTRO_DOME_WIND_2", -1, -1);
    }
}

void SphereSelectorHandle::setStateConfirmStartAtFirstStep() {
    if (MR::isFirstStep(this)) {
        resetRotateParam();
        MR::setStageBGMState(cBgmConfirmState, cBgmConfirmFrames);
    }
}

void SphereSelectorHandle::exeWait() {
    if (MR::isFirstStep(this)) {
        resetRotateParam();
    }

    if (MR::isStarPointerInScreen(WPAD_CHAN0)) {
        SphereSelectorFunction::registerPointingTarget(this, Unknown_1);
    }

    if (mPointerOffscreenStep > 60) {
        setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvDemoRotate::sInstance);
    }
}

void SphereSelectorHandle::exeHold() {
    if (MR::isFirstStep(this)) {
        resetRotateParam();
        clearPointerVelocity();
        MR::startSystemSE("SE_DM_ASTRO_HANDLE_GRAB", -1, -1);
    }

    if (MR::isStarPointerInScreen(WPAD_CHAN0)) {
        stackPointerVelocity();
        TVec2f* pointerVelocity = getPointerVelocity();

        mRotateSpeed = 0.2f * pointerVelocity->x;
        if (mRotateSpeed * mPrevRotateSpeed < 0.0f || MR::abs(mRotateSpeed) < MR::abs(mPrevRotateSpeed)) {
            mRotateSpeed = MR::getLinerValue(0.9f, mRotateSpeed, mPrevRotateSpeed, 1.0f);
        }

        pointerVelocity = getPointerVelocity();
        mTiltSpeed = 0.075f * pointerVelocity->y;
        if (mTiltSpeed * mPrevTiltSpeed < 0.0f || MR::abs(mTiltSpeed) < MR::abs(mPrevTiltSpeed)) {
            mTiltSpeed = MR::getLinerValue(0.9f, mTiltSpeed, mPrevTiltSpeed, 1.0f);
        }
    } else if (mPointerOffscreenStep > 5) {
        mRotateSpeed *= 0.95f;
        mTiltSpeed *= 0.95f;
    }

    tryRelease();
}

void SphereSelectorHandle::exeSpin() {
    mRotateSpeed *= 0.95f;
    mTiltSpeed *= 0.95f;

    if (MR::isStarPointerInScreen(WPAD_CHAN0)) {
        SphereSelectorFunction::registerPointingTarget(this, Unknown_1);
    }

    if (MR::isNearZero(mRotateSpeed, 0.001f) && MR::isNearZero(mTiltSpeed, 0.001f)) {
        setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance);
    }
}

void SphereSelectorHandle::exeDemoRotate() {
    mRotateSpeed = 0.03f;

    if (MR::isStarPointerInScreen(WPAD_CHAN0)) {
        SphereSelectorFunction::registerPointingTarget(this, Unknown_1);
    }

    if (mPointerOffscreenStep == 0) {
        setNerve(&NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance);
    }
}

void SphereSelectorHandle::exeDisappear() {
    if (MR::isFirstStep(this)) {
        resetRotateParam();
        MR::setStageBGMState(cBgmDisappearState, cBgmDisappearFrames);
    }

    if (MR::isStep(this, cBgmDisappearFrames)) {
        kill();
    }
}

void SphereSelectorHandle::exeGalaxyConfirmStart() {
    s32 frame = SphereSelectorFunction::getConfirmStartCancelFrame();
    if (MR::isFirstStep(this)) {
        setStateConfirmStartAtFirstStep();
        mConfirmPosition.zero();
    }

    MR::setNerveAtStep(this, &NrvSphereSelectorHandle::SphereSelectorHandleNrvGalaxyConfirmWait::sInstance, frame);
}

void SphereSelectorHandle::exeGalaxyConfirmCancel() {
    s32 frame = SphereSelectorFunction::getConfirmStartCancelFrame();
    if (MR::isFirstStep(this)) {
        MR::setStageBGMState(cBgmNotConfirmState, cBgmNotConfirmFrames);
    }

    f32 rate = MR::calcNerveEaseInRate(this, frame);
    TVec3f zero(0.0f, 0.0f, 0.0f);
    mPosition.lerp(mConfirmPosition, zero, rate);
    MR::setNerveAtStep(this, &NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance, frame);
}

void SphereSelectorHandle::exeIdleEndForFileSelect() {
    if (MR::isFirstStep(this)) {
        MR::setStageBGMState(cBgmNotConfirmState, cBgmNotConfirmFrames);
    }

    MR::setNerveAtStep(this, &NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::sInstance, SphereSelectorFunction::getConfirmStartCancelFrame());
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvWait::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeWait();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvHold::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeHold();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvSpin::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeSpin();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvDemoRotate::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeDemoRotate();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvDisappear::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeDisappear();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvInvalidRotate::execute(Spine* pSpine) const {
    SphereSelectorHandle* actor = reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor);
    if (MR::isFirstStep(actor)) {
        actor->resetRotateParam();
    }
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvGalaxyConfirmStart::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeGalaxyConfirmStart();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvGalaxyConfirmWait::execute(Spine* pSpine) const {
    SphereSelectorHandle* actor = reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor);
    if (MR::isFirstStep(actor)) {
        actor->mPosition.set(actor->mConfirmPosition);
    }
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvGalaxyConfirmCancel::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeGalaxyConfirmCancel();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvIdleForFileSelect::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->setStateConfirmStartAtFirstStep();
}

void NrvSphereSelectorHandle::SphereSelectorHandleNrvIdleEndForFileSelect::execute(Spine* pSpine) const {
    reinterpret_cast< SphereSelectorHandle* >(pSpine->mExecutor)->exeIdleEndForFileSelect();
}

SphereSelectorHandle::~SphereSelectorHandle() {
}
