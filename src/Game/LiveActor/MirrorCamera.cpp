#include "Game/LiveActor/MirrorCamera.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util.hpp"
#include "Game/Util/MtxUtil.hpp"

#include <JSystem/J3DGraphAnimator/J3DModelData.hpp>

namespace {
    const GXVtxAttrFmtList* getVertexFormat(const J3DModelData* pModelData, GXAttr attr) {
        const GXVtxAttrFmtList* pFormat = pModelData->mVertexData.getVtxAttrFmtList();
        while (pFormat->attr != GX_VA_NULL) {
            if (pFormat->attr == attr) {
                return pFormat;
            }
            pFormat++;
        }
        return nullptr;
    }
};

MirrorCamera::MirrorCamera(const char* pName) : NameObj(pName), _C(0.0f, 0.0f, 0.0f), _18(0.0f, 1.0f, 0.0f), _24(0.0f) {
    mViewMtx.identity();
    mModelTexMtx.identity();
}

void MirrorCamera::setMirrorMapInfo(J3DModelData* pModelData) {
    TVec3f position;
    TVec3f normal;
    const GXVtxAttrFmtList* pPositionFormat = getVertexFormat(pModelData, GX_VA_POS);
    if (pPositionFormat->type == GX_S16) {
        const s16* pPosition = static_cast<const s16*>(pModelData->mVertexData.getVtxPosArray());
        MR::fixed16ToFloat(&position, TVec3s(pPosition[0], pPosition[1], pPosition[2]), pPositionFormat->frac);
    } else {
        const f32* pPosition = static_cast<const f32*>(pModelData->mVertexData.getVtxPosArray());
        position.set(pPosition[0], pPosition[1], pPosition[2]);
    }
    const GXVtxAttrFmtList* pNormalFormat = getVertexFormat(pModelData, GX_VA_NRM);
    if (pNormalFormat->type == GX_S16) {
        const s16* pNormal = static_cast<const s16*>(pModelData->mVertexData.getVtxNrmArray());
        MR::fixed16ToFloat(&normal, TVec3s(pNormal[0], pNormal[1], pNormal[2]), pNormalFormat->frac);
    } else {
        const f32* pNormal = static_cast<const f32*>(pModelData->mVertexData.getVtxNrmArray());
        normal.set(pNormal[0], pNormal[1], pNormal[2]);
    }
    setMirrorMapInfo(normal, position);
}

void MirrorCamera::init(const JMapInfoIter& rIter) {
    MR::connectToScene(this, MR::MovementType_MirrorCamera, -1, -1, -1);
}

void MirrorCamera::setMirrorMapInfo(const TVec3f& a1, const TVec3f& a2) {
    _18.set< f32 >(a1);
    _C.set< f32 >(a2);
    _24 = -PSVECDotProduct((const Vec*)&_18, (const Vec*)&_C);
}

void MirrorCamera::updateViewMtx() {
    TPos3f cameraInv;
    cameraInv.set(MR::getCameraInvViewMtx());
    TVec3f y;
    TVec3f z;
    TVec3f trans;
    cameraInv.getYDir(y);
    cameraInv.getZDir(z);
    cameraInv.getTrans(trans);
    y -= _18 * (_18.dot(y) * 2.0f);
    z -= _18 * (_18.dot(z) * 2.0f);
    TVec3f x = y.cross(z);
    trans -= _18 * ((_18.dot(trans) + _24) * 2.0f);
    mViewMtx.setTR(x, y, z, trans);
    mViewMtx.invert(mViewMtx);
}

void MirrorCamera::updateModelTexMtx() {
    TProj3f mtx = MR::getCameraProjectionMtx();
    mtx[2][0] = 0.0f;
    mtx[2][1] = 0.0f;
    mtx[2][2] = -1.0f;
    mtx[2][3] = 0.0f;
    mtx[3][0] = 0.0f;
    mtx[3][1] = 0.0f;
    mtx[3][2] = 0.0f;
    mtx[3][3] = 1.0f;
    MR::multMtx(mModelTexMtx.mMtx, mViewMtx.mMtx, mtx);
}

f32 MirrorCamera::getDistance(const TVec3f& a1) const {
    TVec3f stack_14;
    TVec3f stack_8;

    stack_8.set< f32 >(_18);
    stack_14.x = stack_8.dot(_C);
    return stack_8.dot(a1) - stack_14.x;
}

namespace MR {
    MirrorCamera* getMirrorCamera() {
        return MR::getSceneObj< MirrorCamera >(SceneObj_MirrorCamera);
    }

    f32 getDistanceToMirror(const TVec3f& rVec) {
        return getMirrorCamera()->getDistance(rVec);
    }
};  // namespace MR

MirrorCamera::~MirrorCamera() {
}

void MirrorCamera::movement() {
    if (MR::isPlayerInAreaObj("MirrorArea")) {
        updateViewMtx();
        updateModelTexMtx();
    }
}
