#include "Game/LiveActor/ShadowVolumeSphere.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/CameraUtil.hpp"

ShadowVolumeSphere::~ShadowVolumeSphere() {
}

ShadowVolumeSphere::ShadowVolumeSphere() : ShadowVolumeModel("影描画[ボリューム球]"), mRadius(100.0f) {
    initVolumeModel("ShadowVolumeSphere");
}

void ShadowVolumeSphere::setRadius(f32 radius) {
    mRadius = radius;
}

bool ShadowVolumeSphere::isDraw() const {
    ShadowController* controller = getController();

    return controller->isProjected() && controller->isDraw();
}

void ShadowVolumeSphere::loadModelDrawMtx() const {
    ShadowController* pController = getController();
    f32 scale = mRadius / 100.0f;
    if (pController->isFollowHostScale()) {
        scale *= pController->getHost()->mScale.x;
    }

    TPos3f mtx;
    mtx.identity();
    mtx.mMtx[0][0] = scale;
    mtx.mMtx[1][1] = scale;
    mtx.mMtx[2][2] = scale;
    TVec3f position;
    pController->getProjectionPos(&position);
    mtx.setTrans(position);
    PSMTXConcat(MR::getCameraViewMtx(), mtx.mMtx, mtx.mMtx);
    GXLoadPosMtxImm(mtx.mMtx, GX_PNMTX0);
}
