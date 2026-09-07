#include "Game/Camera/CameraTargetObj.hpp"
#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"

// Verbatim CameraTargetPlayer methods from root Game/Camera/CameraTargetObj.cpp.
// CameraTargetObj's constructor is supplied by CameraLocalUtilRuntime.cpp.
// The owner binds the real MarioActor before running movement once per camera phase.
namespace {
    static TVec3f sZeroVec(0.0f, 0.0f, 0.0f);
};  // namespace

CameraTargetPlayer::CameraTargetPlayer(const char* pName)
    : CameraTargetObj(pName), mGravity(0.0f, -1.0f, 0.0f), mGroundPos(0.0f, 0.0f, 0.0f), mCameraArea(), mGroundTriangle(), mPlayerMovementTimer(),
      mIsPlayerMoving(true) {
}

void CameraTargetPlayer::movement() {
    if (MR::isDead(mActor) || MR::isClipped(mActor)) {
        return;
    }

    if (MR::isPlayerInBind()) {
        TPos3f mtx;
        mtx.set(MR::getPlayerBaseMtx());
        mtx.getXDir(mSide);
        mtx.getYDir(mUp);
        mtx.getZDir(mFront);
    } else {
        mActor->getUpVec(&mUp);
        mActor->getFrontVec(&mFront);
        mActor->getSideVec(&mSide);
    }

    if (MR::isPlayerElementModeBee()) {
        MR::calcGravityVector(this, mActor->mPosition, &mGravity, nullptr, 0);
    } else {
        mActor->getGravityVector(&mGravity);
    }

    mGroundPos.set(*mActor->getShadowPos());

    CubeCameraArea* area = MR::getCameraCube();
    if (area != nullptr) {
        mCameraArea = area;
    } else {
        mCameraArea = nullptr;
    }

    mGroundTriangle = MR::getPlayerGroundingPolygon();

    if (MR::isNearZero(mUp)) {
        mUp.set(0.0f, 1.0f, 0.0f);
    } else {
        MR::normalize(&mUp);
    }

    if (mPlayerMovementTimer != MR::getPlayerMovementTimer()) {
        mIsPlayerMoving = true;
    } else {
        mIsPlayerMoving = false;
    }

    mPlayerMovementTimer = MR::getPlayerMovementTimer();
}

const TVec3f& CameraTargetPlayer::getPosition() const {
    return mActor->getTransForCamera();
}

const TVec3f& CameraTargetPlayer::getUpVec() const {
    return mUp;
}

const TVec3f& CameraTargetPlayer::getFrontVec() const {
    return mFront;
}

const TVec3f& CameraTargetPlayer::getSideVec() const {
    return mSide;
}

const TVec3f& CameraTargetPlayer::getGroundPos() const {
    return mGroundPos;
}

const TVec3f& CameraTargetPlayer::getGravityVector() const {
    return mGravity;
}

const TVec3f& CameraTargetPlayer::getLastMove() const {
    if (MR::isDemoActive() && !mIsPlayerMoving) {
        return sZeroVec;
    } else {
        return *MR::getPlayerLastMove();
    }
}

bool CameraTargetPlayer::isTurning() const {
    return mActor->isTurning();
}

bool CameraTargetPlayer::isJumping() const {
    return mActor->isJumping();
}

bool CameraTargetPlayer::isLongDrop() const {
    return mActor->isLongDrop();
}

bool CameraTargetPlayer::isFastDrop() const {
    return mActor->isFastDrop();
}

bool CameraTargetPlayer::isFastRise() const {
    return mActor->isFastRise();
}

bool CameraTargetPlayer::isWaterMode() const {
    return MR::isPlayerInWaterMode();
}

bool CameraTargetPlayer::isOnWaterSurface() const {
    return MR::isPlayerOnWaterSurface();
}

bool CameraTargetPlayer::isFooFighterMode() const {
    return MR::isPlayerFlying();
}

u32 CameraTargetPlayer::getSpecialMode() const {
    return mActor->getSpecialMode();
}

bool CameraTargetPlayer::isCameraStateOn(u32 status) const {
    return mActor->isCameraStateOn((SPECIAL_STATUS_FOR_CAMERA)status);
}

CubeCameraArea* CameraTargetPlayer::getCubeCameraArea() const {
    return mCameraArea;
}

Triangle* CameraTargetPlayer::getGroundTriangle() const {
    return mGroundTriangle;
}

GravityInfo* CameraTargetPlayer::getGravityInfo() const {
    return mActor->getGravityInfo();
}

bool CameraTargetPlayer::isDebugMode() const {
    return mActor->isDebugMode();
}

TPos3f* CameraTargetPlayer::getMapBaseMtx() const {
    return reinterpret_cast< TPos3f* >(mActor->getMapBaseMtx());
}
