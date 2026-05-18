#include "Game/Camera/CameraTargetMtx.hpp"

void CameraTargetMatrix::identity() {
    mMtx[0][0] = 1.0F;
    mMtx[0][1] = 0.0F;
    mMtx[0][2] = 0.0F;
    mMtx[0][3] = 0.0F;
    mMtx[1][0] = 0.0F;
    mMtx[1][1] = 1.0F;
    mMtx[1][2] = 0.0F;
    mMtx[1][3] = 0.0F;
    mMtx[2][0] = 0.0F;
    mMtx[2][1] = 0.0F;
    mMtx[2][2] = 1.0F;
    mMtx[2][3] = 0.0F;
}

CameraTargetMtx::CameraTargetMtx(const char *pName)
    : NameObj(pName), mPosition(0.0F, 0.0F, 0.0F), mLastMove(0.0F, 0.0F, 0.0F), mGravityVector(0.0F, -1.0F, 0.0F),
      mUp(0.0F, 1.0F, 0.0F), mFront(0.0F, 0.0F, 1.0F), mSide(1.0F, 0.0F, 0.0F), mInvalidLastMove(false) {
    mMatrix.identity();
}

CameraTargetMtx::~CameraTargetMtx() = default;

void CameraTargetMtx::movement() {
    const auto next_position = TVec3f{
        mMatrix.mMtx[0][3],
        mMatrix.mMtx[1][3],
        mMatrix.mMtx[2][3],
    };

    if (mInvalidLastMove) {
        mLastMove.set(0.0F, 0.0F, 0.0F);
        mInvalidLastMove = false;
    } else {
        mLastMove = next_position - mPosition;
    }

    mPosition.set(next_position);
    mUp.set(mMatrix.mMtx[0][1], mMatrix.mMtx[1][1], mMatrix.mMtx[2][1]);
    mFront.set(mMatrix.mMtx[0][2], mMatrix.mMtx[1][2], mMatrix.mMtx[2][2]);
    mSide.set(mMatrix.mMtx[0][0], mMatrix.mMtx[1][0], mMatrix.mMtx[2][0]);
    mGravityVector.set(0.0F, -1.0F, 0.0F);
}

void CameraTargetMtx::invalidateLastMove() {
    mInvalidLastMove = true;
}
