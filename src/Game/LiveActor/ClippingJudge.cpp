#include "Game/LiveActor/ClippingJudge.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/Util.hpp"

ClippingJudge::ClippingJudge(const char* pName) : NameObj(pName), mFrustum() {
    for (s32 i = 0; i < 8; i++) {
        mClipDistances[i] = -1.0f;
    }

    mClipDistances[0] = 0.0f;
    mClipDistances[1] = 60000.0f;
    mClipDistances[2] = 50000.0f;
    mClipDistances[3] = 40000.0f;
    mClipDistances[4] = 30000.0f;
    mClipDistances[5] = 20000.0f;
    mClipDistances[6] = 10000.0f;
    mClipDistances[7] = 5000.0f;
}

void ClippingJudge::init(const JMapInfoIter& rIter) {
}

void ClippingJudge::movement() {
    calcViewingVolume(&mFrustum, MR::getFarZ());

    for (u32 i = 1; i < 8; i++) {
        calcViewingVolume(&mClipFrustums[i], mClipDistances[i]);
    }
}

bool ClippingJudge::isJudgedToClipFrustum(const TVec3f& rVec, f32 a2) const {
    return !mFrustum.mayIntersectBall3(rVec, a2);
}

bool ClippingJudge::isJudgedToClipFrustum(const TVec3f& rVec, f32 a2, s32 index) const {
    return (!index) ? !mFrustum.mayIntersectBall3(rVec, a2) : !mClipFrustums[index].mayIntersectBall3(rVec, a2);
}

void ClippingJudge::calcViewingVolume(THex3f* pVolume, f32 farZ) {
    f32 nearZ = 500.0f;
    if (MR::isSubjectiveCameraOnForObjClipping()) {
        nearZ = 100.0f;
    }

    f32 aspect = MR::getAspect();
    f32 fovy = MR::getFovy();
    TPos3f cameraMtx;
    cameraMtx.setPositionFromLookAt(MR::getCameraViewMtx());
    f32 halfHeight = nearZ * static_cast< f32 >(tan(fovy * (PI_180 * 0.5f)));
    f32 halfWidth = aspect * halfHeight;

    TVec3f front;
    cameraMtx.getZDir(front);
    TVec3f rightBottom(halfWidth, -halfHeight, nearZ);
    TVec3f rightTop(halfWidth, halfHeight, nearZ);
    TVec3f leftTop(-halfWidth, halfHeight, nearZ);
    TVec3f leftBottom(-halfWidth, -halfHeight, nearZ);
    TVec3f eye;
    cameraMtx.getTrans(eye);
    cameraMtx.mult(rightTop, rightTop);
    cameraMtx.mult(leftTop, leftTop);
    cameraMtx.mult(leftBottom, leftBottom);
    cameraMtx.mult(rightBottom, rightBottom);

    pVolume->mPlanes[3].set(eye, leftTop, rightTop);
    pVolume->mPlanes[1].set(eye, rightTop, rightBottom);
    pVolume->mPlanes[2].set(eye, rightBottom, leftBottom);
    pVolume->mPlanes[0].set(eye, leftBottom, leftTop);

    TVec3f back(-front);
    TVec3f planePoint;
    planePoint.scaleAdd(farZ, front, eye);
    pVolume->mPlanes[5].set(back, planePoint);
    planePoint.scaleAdd(nearZ, front, eye);
    pVolume->mPlanes[4].set(front, planePoint);
}

namespace MR {
    ClippingJudge* getClippingJudge() {
        return getClippingDirector()->mJudge;
    }
};  // namespace MR

ClippingJudge::~ClippingJudge() {
}
