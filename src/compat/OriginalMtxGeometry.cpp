#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

namespace MR {
    void makeMtxUpSide(TPos3f* pDst, const TVec3f& rUp, const TVec3f& rSide) {
        TVec3f axisY;
        MR::normalize(rUp, &axisY);

        TVec3f axisZ = axisY.cross(rSide);
        MR::normalize(&axisZ);

        TVec3f axisX = axisZ.cross(axisY);

        pDst->setXYZDir(axisX, axisY, axisZ);
    }

    void makeMtxFrontUp(TPos3f* pDst, const TVec3f& rFront, const TVec3f& rUp) {
        TVec3f axisZ;
        MR::normalize(rFront, &axisZ);

        TVec3f axisX = rUp.cross(axisZ);
        MR::normalize(&axisX);

        TVec3f axisY = axisZ.cross(axisX);

        pDst->setXYZDir(axisX, axisY, axisZ);
    }

    void rotAxisVecRad(const TVec3f& rAxis, const TVec3f& rVec, TVec3f* pOut, f32 rad) {
        Mtx rot;
        PSMTXRotAxisRad(rot, &rVec, rad);
        PSMTXMultVec(rot, &rAxis, pOut);
    }
}
