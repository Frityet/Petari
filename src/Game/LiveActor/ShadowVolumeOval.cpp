#include "resource/TextEncoding.hpp"
#include "Game/LiveActor/ShadowVolumeOval.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include <JSystem/JMath/JMath.hpp>

ShadowVolumeOval::~ShadowVolumeOval() {
}

ShadowVolumeOval::ShadowVolumeOval() : ShadowVolumeModel(CP932("影描画[ボリューム楕球]")), mSize(100.0f, 100.0f, 200.0f) {
    initVolumeModel("ShadowVolumeSphere");
}

bool ShadowVolumeOval::isDraw() const {
    ShadowController* controller = getController();
    return controller->isProjected() && controller->isDraw();
}

void ShadowVolumeOval::loadModelDrawMtx() const {
    ShadowController* controller = getController();
    MtxPtr dropMtx = controller->_18;
    TVec3f direction;
    controller->getDropDir(&direction);
    TVec3f scale(mSize);
    scale.scale(1.0f / 100.0f);
    if (scale.x <= 0.01f) {
        scale.x = 0.01f;
    }
    if (scale.y <= 0.01f) {
        scale.y = 0.01f;
    }
    if (scale.z <= 0.01f) {
        scale.z = 0.01f;
    }
    if (controller->isFollowHostScale()) {
        scale *= controller->getHost()->mScale;
    }

    TPos3f scaleMtx;
    scaleMtx.setInline(*reinterpret_cast< const TPos3f* >(dropMtx));
    scaleMtx.setTrans(TVec3f(0.0f, 0.0f, 0.0f));
    MR::preScaleMtx(scaleMtx.toMtxPtr(), scale);
    TPos3f inverseMtx;
    inverseMtx.invert(scaleMtx);
    TVec3f localDirection;
    inverseMtx.mult(direction, localDirection);
    TPos3f drawMtx;
    drawMtx.identity();
    MR::makeMtxUpNoSupport(&drawMtx, localDirection);
    drawMtx.concat(scaleMtx, drawMtx);
    TVec3f position;
    controller->getProjectionPos(&position);
    drawMtx.setTrans(position);
    TVec3f side;
    TVec3f up;
    TVec3f front;
    drawMtx.getXDir(side);
    drawMtx.getYDir(up);
    drawMtx.getZDir(front);
    if (!MR::normalizeOrZero(&up)) {
        JMAVECScaleAdd(&up, &side, &side, -up.dot(side));
        JMAVECScaleAdd(&up, &front, &front, -up.dot(front));
    }
    drawMtx.setXDir(side);
    drawMtx.setZDir(front);
    PSMTXConcat(MR::getCameraViewMtx(), drawMtx.toMtxPtr(), drawMtx.toMtxPtr());
    GXLoadPosMtxImm(drawMtx.toMtxPtr(), GX_PNMTX0);
}

void ShadowVolumeOval::setSize(const TVec3f& rSize) {
    mSize = rSize;
}
