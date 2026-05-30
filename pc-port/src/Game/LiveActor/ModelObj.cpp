#include "Game/LiveActor/ModelObj.hpp"

#include <cmath>

#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace {
    constexpr f32 cDegToRad = 3.14159265358979323846F / 180.0F;

    smgpc::render::J3dMatrix3x4 makeTRSMatrix(const TVec3f& position, const TVec3f& rotation, const TVec3f& scale) {
        const auto rx = rotation.x * cDegToRad;
        const auto ry = rotation.y * cDegToRad;
        const auto rz = rotation.z * cDegToRad;
        const auto sx = std::sin(rx);
        const auto cx = std::cos(rx);
        const auto sy = std::sin(ry);
        const auto cy = std::cos(ry);
        const auto sz = std::sin(rz);
        const auto cz = std::cos(rz);

        const auto r00 = cz * cy;
        const auto r01 = (cz * sy * sx) - (sz * cx);
        const auto r02 = (cz * sy * cx) + (sz * sx);
        const auto r10 = sz * cy;
        const auto r11 = (sz * sy * sx) + (cz * cx);
        const auto r12 = (sz * sy * cx) - (cz * sx);
        const auto r20 = -sy;
        const auto r21 = cy * sx;
        const auto r22 = cy * cx;

        return smgpc::render::J3dMatrix3x4{{
            r00 * scale.x,
            r01 * scale.y,
            r02 * scale.z,
            position.x,
            r10 * scale.x,
            r11 * scale.y,
            r12 * scale.z,
            position.y,
            r20 * scale.x,
            r21 * scale.y,
            r22 * scale.z,
            position.z,
        }};
    }
}  // namespace

ModelObj::ModelObj(const char* pName, const char* pModelName, MtxPtr pMtx, int drawBufferType, int movementType, int calcAnimType, bool useScale)
    : LiveActor(pName), mMtx(pMtx) {
    if (drawBufferType < -1) {
        drawBufferType = MR::DrawBufferType_MapObj;
    }
    if (movementType < -1) {
        movementType = MR::MovementType_MapObjDecoration;
    }
    if (calcAnimType < -1) {
        calcAnimType = MR::CalcAnimType_MapObjDecoration;
    }

    initModelManagerWithAnm(pModelName, nullptr, useScale);
    MR::connectToScene(this, movementType, calcAnimType, drawBufferType, -1);
    initSound(8, false);
    initEffectKeeper(8, nullptr, false);

    if (mMtx != nullptr) {
        mPosition.set(mMtx[0][3], mMtx[1][3], mMtx[2][3]);
    }
}

void ModelObj::init(const JMapInfoIter& rIter) {
    (void)MR::getJMapInfoTrans(rIter, &mPosition);
    (void)MR::getJMapInfoRotate(rIter, &mRotation);
    (void)MR::getJMapInfoScale(rIter, &mScale);
    makeActorAppeared();
}

void ModelObj::calcAndSetBaseMtx() {
    if (mMtx != nullptr) {
        mPosition.set(mMtx[0][3], mMtx[1][3], mMtx[2][3]);
        MR::setBaseTRMtx(this, mMtx);
        return;
    }

    setBaseMatrix(makeTRSMatrix(mPosition, mRotation, mScale));
}
