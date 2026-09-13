#include <aurora/exception.hpp>
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

#include <cmath>
#include <stdexcept>

namespace {
    constexpr f32 cDegreesToRadians = 3.14159265358979323846F / 180.0F;

}  // namespace

namespace MR {
    void makeMtxUpFront(TPos3f* pDst, const TVec3f& rUp, const TVec3f& rFront) {
        TVec3f axisY;
        MR::normalize(rUp, &axisY);

        TVec3f axisX = axisY.cross(rFront);
        MR::normalize(&axisX);

        TVec3f axisZ = axisX.cross(axisY);

        pDst->setXYZDir(axisX, axisY, axisZ);
    }

    void makeMtxUpFrontPos(TPos3f* pDst, const TVec3f& rUp, const TVec3f& rFront, const TVec3f& rPos) {
        makeMtxUpFront(pDst, rUp, rFront);
        pDst->setTrans(rPos);
    }

    void makeMtxRotate(MtxPtr mtx, s16 rx, s16 ry, s16 rz) {
        f32 sinY = JMASSin(ry);
        f32 cosZ = JMASCos(rz);
        f32 sinZ = JMASSin(rz);
        f32 cosX = JMASCos(rx);
        f32 sinX = JMASSin(rx);
        f32 cosY = JMASCos(ry);

        f32 sinZsinY = sinZ * sinY;
        f32 cosZsinY = cosZ * sinY;

        f32 sinXsinZsinY = sinX * sinZsinY;
        f32 cosXcosZ = cosX * cosZ;

        mtx[2][0] = -sinY;

        f32 cosZcosY = cosZ * cosY;
        f32 sinZcosY = sinZ * cosY;

        mtx[0][0] = cosZcosY;
        mtx[1][0] = sinZcosY;

        f32 cosXsinZ = cosX * sinZ;
        f32 sinXcosZ = sinX * cosZ;
        f32 sinXsinZ = sinX * sinZ;

        f32 sinXcosZsinY = sinX * cosZsinY;
        f32 cosXcosZsinY = cosX * cosZsinY;
        f32 cosXsinZsinY = cosX * sinZsinY;

        mtx[0][3] = 0.0f;
        mtx[0][1] = sinXcosZsinY - cosXsinZ;
        mtx[0][2] = cosXcosZsinY + sinXsinZ;

        f32 sinXcosY = sinX * cosY;
        f32 cosXcosY = cosX * cosY;

        mtx[2][1] = sinXcosY;
        mtx[1][1] = sinXsinZsinY + cosXcosZ;
        mtx[1][2] = cosXsinZsinY - sinXcosZ;

        mtx[2][2] = cosXcosY;
        mtx[1][3] = 0.0f;
        mtx[2][3] = 0.0f;
    }

    void makeMtxRotate(MtxPtr pMatrix, f32 rotationX, f32 rotationY, f32 rotationZ) {
        makeMtxRotate(pMatrix, static_cast<s16>(rotationX * DEGREE_TO_S16),
                      static_cast<s16>(rotationY * DEGREE_TO_S16),
                      static_cast<s16>(rotationZ * DEGREE_TO_S16));
    }

    void makeMtxRotate(MtxPtr pMatrix, const TVec3f &rRotation) {
        makeMtxRotate(pMatrix, rRotation.x, rRotation.y, rRotation.z);
    }

    void makeMtxRotateY(MtxPtr pMatrix, f32 rotationY) {
        const auto radians = rotationY * cDegreesToRadians;
        const auto sinY = std::sin(radians);
        const auto cosY = std::cos(radians);

        pMatrix[0][0] = cosY;
        pMatrix[1][0] = 0.0F;
        pMatrix[2][0] = -sinY;
        pMatrix[0][1] = 0.0F;
        pMatrix[1][1] = 1.0F;
        pMatrix[2][1] = 0.0F;
        pMatrix[0][2] = sinY;
        pMatrix[1][2] = 0.0F;
        pMatrix[2][2] = cosY;
        pMatrix[0][3] = 0.0F;
        pMatrix[1][3] = 0.0F;
        pMatrix[2][3] = 0.0F;
    }

    void makeMtxTransRotateY(MtxPtr pMatrix, f32 tx, f32 ty, f32 tz, f32 rotationY) {
        makeMtxRotateY(pMatrix, rotationY);
        pMatrix[0][3] = tx;
        pMatrix[1][3] = ty;
        pMatrix[2][3] = tz;
    }

    void makeMtxTR(MtxPtr pMatrix, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz) {
        const auto sinX = std::sin(rx * cDegreesToRadians);
        const auto sinY = std::sin(ry * cDegreesToRadians);
        const auto sinZ = std::sin(rz * cDegreesToRadians);
        const auto cosX = std::cos(rx * cDegreesToRadians);
        const auto cosY = std::cos(ry * cDegreesToRadians);
        const auto cosZ = std::cos(rz * cDegreesToRadians);

        pMatrix[0][0] = cosZ * cosY;
        pMatrix[1][0] = sinZ * cosY;
        pMatrix[2][0] = -sinY;
        pMatrix[0][1] = cosZ * sinY * sinX - sinZ * cosX;
        pMatrix[1][1] = sinZ * sinY * sinX + cosZ * cosX;
        pMatrix[2][1] = cosY * sinX;
        pMatrix[0][2] = cosZ * sinY * cosX + sinZ * sinX;
        pMatrix[1][2] = sinZ * sinY * cosX - cosZ * sinX;
        pMatrix[2][2] = cosY * cosX;
        pMatrix[0][3] = tx;
        pMatrix[1][3] = ty;
        pMatrix[2][3] = tz;
    }

    void makeMtxTR(MtxPtr pMatrix, const TVec3f &rTranslation, const TVec3f &rRotation) {
        makeMtxTR(pMatrix, rTranslation.x, rTranslation.y, rTranslation.z, rRotation.x, rRotation.y, rRotation.z);
    }

    void makeMtxTRS(MtxPtr pMatrix, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz) {
        makeMtxTR(pMatrix, tx, ty, tz, rx, ry, rz);
        pMatrix[0][0] *= sx;
        pMatrix[1][0] *= sx;
        pMatrix[2][0] *= sx;
        pMatrix[0][1] *= sy;
        pMatrix[1][1] *= sy;
        pMatrix[2][1] *= sy;
        pMatrix[0][2] *= sz;
        pMatrix[1][2] *= sz;
        pMatrix[2][2] *= sz;
    }

    void makeMtxTRS(MtxPtr pMatrix, const TVec3f &rTranslation, const TVec3f &rRotation, const TVec3f &rScale) {
        makeMtxTRS(pMatrix, rTranslation.x, rTranslation.y, rTranslation.z, rRotation.x, rRotation.y, rRotation.z, rScale.x,
                   rScale.y, rScale.z);
    }

    void preScaleMtx(MtxPtr pMatrix, f32 scale) {
        preScaleMtx(pMatrix, scale, scale, scale);
    }

    void preScaleMtx(MtxPtr pMatrix, const TVec3f &rScale) {
        preScaleMtx(pMatrix, rScale.x, rScale.y, rScale.z);
    }

    void preScaleMtx(MtxPtr pMatrix, f32 scaleX, f32 scaleY, f32 scaleZ) {
        if (pMatrix == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Matrix scaling requires a real matrix.");
        }

        for (auto row = 0; row < 3; ++row) {
            pMatrix[row][0] *= scaleX;
            pMatrix[row][1] *= scaleY;
            pMatrix[row][2] *= scaleZ;
        }
    }

    void makeMtxUpNoSupport(TPos3f* pDst, const TVec3f& rUp) {
        TVec3f support;
        if (MR::getMaxAbsElementIndex(rUp) == 2) {
            support.set(0.0f, 1.0f, 0.0f);
        } else {
            support.set(0.0f, 0.0f, 1.0f);
        }

        TVec3f axisY;
        MR::normalize(rUp, &axisY);

        TVec3f axisX = axisY.cross(support);
        MR::normalize(&axisX);

        TVec3f axisZ = axisX.cross(axisY);

        pDst->setXYZDir(axisX, axisY, axisZ);
    }

    void makeMtxUpNoSupportPos(TPos3f* pDst, const TVec3f& rUp, const TVec3f& rPos) {
        TVec3f support;
        if (MR::getMaxAbsElementIndex(rUp) == 2) {
            support.set< f32 >(0.0f, 1.0f, 0.0f);
        } else {
            support.set< f32 >(0.0f, 0.0f, 1.0f);
        }

        MR::makeMtxUpFrontPos(pDst, rUp, support, rPos);
    }
}  // namespace MR

// Original methods from Game/Util/MtxUtil.cpp.
namespace MR {
    void makeRTFromMtxPtr(TVec3f* pOutTrans, TVec3f* pOutRot, MtxPtr src, bool toDegree) {
        if (pOutTrans) {
            ((TPos3f*)src)->getTrans(*pOutTrans);
        }

        if (pOutRot) {
            ((TRot3f*)src)->getEuler(*pOutRot);

            if (toDegree) {
                pOutRot->set(*pOutRot * (180.0f / PI));
            }
        }
    }

    bool isSameMtx(MtxPtr a, MtxPtr b) {
        f32* pA = (f32*)a;
        f32* pB = (f32*)b;
        for (int i = 0; i < 12; i++) {
            if (*pA != *pB) {
                return false;
            }
            pA++;
            pB++;
        }
        return true;
    }
} // namespace MR

namespace MR {
    void extractMtxTrans(MtxPtr mtx, TVec3f* pOut) {
        pOut->x = mtx[0][3];
        pOut->y = mtx[1][3];
        pOut->z = mtx[2][3];
    }
}  // namespace MR

// Original shared math from Game/Util/MtxUtil.cpp.
namespace MR {
    void makeMtxFrontNoSupport(TPos3f* pDst, const TVec3f& rFront) {
        TVec3f support;
        if (MR::getMaxAbsElementIndex(rFront) == 1) {
            support.set(1.0f, 0.0f, 0.0f);
        } else {
            support.set(0.0f, 1.0f, 0.0f);
        }

        TVec3f axisZ;
        MR::normalize(rFront, &axisZ);

        TVec3f axisX = support.cross(axisZ);
        MR::normalize(&axisX);

        TVec3f axisY = axisZ.cross(axisX);

        pDst->setXYZDir(axisX, axisY, axisZ);
    }
}
