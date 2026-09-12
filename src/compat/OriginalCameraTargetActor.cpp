#include "Game/Camera/CameraTargetObj.hpp"
#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

// Verbatim actor-target methods from root Game/Camera/CameraTargetObj.cpp.
// The shared base constructor and player-target methods have separate providers.
CameraTargetActor::CameraTargetActor(const char* pName)
    : CameraTargetObj(pName), mActor(), mUp(0.0f, 1.0f, 0.0f), mFront(0.0f, 0.0f, 1.0f), mSide(0.0f, 0.0f, 1.0f), mCameraArea() {
}

void CameraTargetActor::movement() {
    if (MR::isDead(mActor) || MR::isClipped(mActor)) {
        return;
    }

    if (mActor->getBaseMtx() != nullptr) {
        MR::calcUpVec(&mUp, mActor);
        MR::calcFrontVec(&mFront, mActor);
        MR::calcSideVec(&mSide, mActor);
    } else {
        TRot3f matrix;
        MR::makeMtxRotate(matrix, mActor->mRotation.x, mActor->mRotation.y, mActor->mRotation.z);

        matrix.getYDir(mUp);
        matrix.getZDir(mFront);
        matrix.getXDir(mSide);
    }

    CubeCameraArea* area = static_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", mActor->mPosition));

    if (area == nullptr) {
        mCameraArea = nullptr;
    } else {
        mCameraArea = area;
    }
}

const TVec3f& CameraTargetActor::getPosition() const {
    return mActor->mPosition;
}

const TVec3f& CameraTargetActor::getUpVec() const {
    return mUp;
}

const TVec3f& CameraTargetActor::getFrontVec() const {
    return mFront;
}

const TVec3f& CameraTargetActor::getSideVec() const {
    return mSide;
}

const TVec3f& CameraTargetActor::getLastMove() const {
    return mActor->mVelocity;
}

const TVec3f& CameraTargetActor::getGroundPos() const {
    return mActor->mPosition;
}

const TVec3f& CameraTargetActor::getGravityVector() const {
    if (getGravityInfo() != nullptr) {
        return getGravityInfo()->mGravityVector;
    } else {
        return mUp;
    }
}

CubeCameraArea* CameraTargetActor::getCubeCameraArea() const {
    return mCameraArea;
}

Triangle* CameraTargetActor::getGroundTriangle() const {
    return nullptr;
}
