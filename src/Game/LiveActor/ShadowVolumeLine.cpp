#include "compat/Cp932Literal.hpp"
#include "Game/LiveActor/ShadowVolumeLine.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/MathUtil.hpp"
#include "JSystem/JMath/JMath.hpp"

ShadowVolumeLine::ShadowVolumeLine() : ShadowVolumeDrawer(CP932("影描画[ボリュームライン]")) {
    mFromShadowController = 0;
    mToShadowController = 0;
    mFromWidth = 100.0f;
    mToWidth = 100.0f;
}

void ShadowVolumeLine::loadModelDrawMtx() const {
    GXLoadPosMtxImm(MR::getCameraViewMtx(), 0);
    GXSetCurrentMtx(0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
}

void ShadowVolumeLine::drawShape() const {
    const ShadowController* fromController = mFromShadowController;
    const ShadowController* toController = mToShadowController;
    TVec3f points[8];
    TVec3f fromPos, toPos;
    calcBaseDropPosition(&fromPos, fromController);
    calcBaseDropPosition(&toPos, toController);

    TVec3f direction;
    MR::normalizeOrZero(toPos - fromPos, &direction);
    if (MR::isNearZero(direction)) {
        return;
    }

    TVec3f fromDropDir, toDropDir;
    fromController->getDropDir(&fromDropDir);
    toController->getDropDir(&toDropDir);

    TVec3f fromSide, toSide;
    PSVECCrossProduct(&fromDropDir, &direction, &fromSide);
    if (MR::isNearZero(fromSide)) {
        return;
    }
    MR::normalize(&fromSide);
    PSVECCrossProduct(&toDropDir, &direction, &toSide);
    if (MR::isNearZero(toSide)) {
        return;
    }
    MR::normalize(&toSide);

    f32 fromLength = mFromWidth + calcBaseDropLength(fromController);
    f32 toLength = mToWidth + calcBaseDropLength(toController);
    JMAVECScaleAdd(&fromSide, &fromPos, &points[0], -mFromWidth);
    JMAVECScaleAdd(&fromSide, &fromPos, &points[1], mFromWidth);
    JMAVECScaleAdd(&fromDropDir, &points[0], &points[2], fromLength);
    JMAVECScaleAdd(&fromDropDir, &points[1], &points[3], fromLength);
    JMAVECScaleAdd(&toSide, &toPos, &points[4], -mToWidth);
    JMAVECScaleAdd(&toSide, &toPos, &points[5], mToWidth);
    JMAVECScaleAdd(&toDropDir, &points[4], &points[6], toLength);
    JMAVECScaleAdd(&toDropDir, &points[5], &points[7], toLength);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    TDDraw::sendPoint(points[1]);
    TDDraw::sendPoint(points[5]);
    TDDraw::sendPoint(points[7]);
    TDDraw::sendPoint(points[3]);
    GXEnd();

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    TDDraw::sendPoint(points[0]);
    TDDraw::sendPoint(points[2]);
    TDDraw::sendPoint(points[6]);
    TDDraw::sendPoint(points[4]);
    GXEnd();

    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 10);
    TDDraw::sendPoint(points[0]);
    TDDraw::sendPoint(points[1]);
    TDDraw::sendPoint(points[2]);
    TDDraw::sendPoint(points[3]);
    TDDraw::sendPoint(points[6]);
    TDDraw::sendPoint(points[7]);
    TDDraw::sendPoint(points[4]);
    TDDraw::sendPoint(points[5]);
    TDDraw::sendPoint(points[0]);
    TDDraw::sendPoint(points[1]);
    GXEnd();
}

void ShadowVolumeLine::setFromShadowController(const ShadowController* pController) {
    mFromShadowController = pController;
}

void ShadowVolumeLine::setToShadowController(const ShadowController* pController) {
    mToShadowController = pController;
}

void ShadowVolumeLine::setFromWidth(f32 width) {
    mFromWidth = width;
}

void ShadowVolumeLine::setToWidth(f32 width) {
    mToWidth = width;
}

ShadowVolumeLine::~ShadowVolumeLine() {
}
