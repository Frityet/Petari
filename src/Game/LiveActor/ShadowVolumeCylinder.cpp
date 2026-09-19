#include "compat/Cp932Literal.hpp"
#include "Game/LiveActor/ShadowVolumeCylinder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

ShadowVolumeCylinder::ShadowVolumeCylinder() : ShadowVolumeModel(CP932("影描画[ボリューム円柱]")) {
    mRadius = 100.0f;
    initVolumeModel("ShadowVolumeCylinder");
}

void ShadowVolumeCylinder::setRadius(f32 radius) {
    mRadius = radius;
}

void ShadowVolumeCylinder::loadModelDrawMtx() const {
    const ShadowController* pController = getController();
    TVec3f position;
    calcBaseDropPosition(&position);
    TVec3f dropDirection;
    pController->getDropDir(&dropDirection);
    TPos3f mtx;
    MR::makeMtxUpNoSupportPos(&mtx, -dropDirection, position);

    f32 radiusScale = mRadius / 100.0f;
    if (pController->isFollowHostScale()) {
        radiusScale *= pController->getHost()->mScale.x;
    }

    TVec3f scale(radiusScale, calcBaseDropLength() / 100.0f, radiusScale);
    MR::preScaleMtx(mtx.toMtxPtr(), scale);
    PSMTXConcat(MR::getCameraViewMtx(), mtx.toMtxPtr(), mtx.toMtxPtr());
    GXLoadPosMtxImm(mtx.toMtxPtr(), GX_PNMTX0);
}
