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

    bool isSameMtxRot(MtxPtr a, MtxPtr b) {
        f32* pA = (f32*)a;
        f32* pB = (f32*)b;
        for (u32 i = 0; i < 12; i++) {
            if ((i & 3) != 3) {
                if (*pA != *pB) {
                    return false;
                }
            }
            pA++;
            pB++;
        }
        return true;
    }

    bool isRotAxisY(MtxPtr a, MtxPtr b) {
        TVec3f yDirA, yDirB;
        ((TRot3f*)a)->getYDir(yDirA);
        ((TRot3f*)b)->getYDir(yDirB);
        bool result = false;

        if (JGeometry::TUtil< f32 >::epsilonEquals(yDirA.x, yDirB.x, 0.001f) && JGeometry::TUtil< f32 >::epsilonEquals(yDirA.y, yDirB.y, 0.001f) &&
            JGeometry::TUtil< f32 >::epsilonEquals(yDirA.z, yDirB.z, 0.001f)) {
            result = true;
        }
        return result;
    }

    void calcMtxRotAxis(TVec3f* pOut, MtxPtr a, MtxPtr b) {
        TVec3f localZ(0.0f, 0.0f, 1.0f);

        Mtx invA;
        PSMTXInverse(a, invA);

        TVec3f axisZA, axisZB;
        PSMTXMultVecSR(invA, &localZ, &axisZA);
        PSMTXMultVecSR(b, &localZ, &axisZB);

        TVec3f cross = axisZA.cross(axisZB);

        if (MR::normalizeOrZero(&cross)) {
            *pOut = localZ;
        } else {
            *pOut = cross;
        }
    }

    f32 diffAngleAbs(f32 angleA, f32 angleB) {
        f32 normalize = normalizeAngleAbs(angleA - angleB);

        if (normalize > PI) {
            normalize = (TWO_PI - normalize);
        }

        return normalize;
    }

    f32 diffAngleAbs(const TVec2f& rA, const TVec2f& rB) {
        f32 length1 = rA.length();
        f32 length2 = rB.length();
        f32 x = rA.dot(rB) / (length1 * length2);

        if (x >= 1.0f) {
            return 0.0f;
        } else if (x <= -1.0f) {
            return PI;
        } else {
            return acosEx(x);
        }
    }

    bool isNormalize(const TVec3f& rVec, f32 tolerance) {
        return MR::abs(1.0f - rVec.length()) <= tolerance;
    }

    void getRotatedAxisZ(TVec3f* pDst, const TVec3f& pSrc) {
        TVec3f vec(0.0f, 0.0f, 1.0f);
        TPos3f mtx;
        MR::makeMtxTR(mtx, 0.0f, 0.0f, 0.0f, pSrc.x, pSrc.y, pSrc.z);
        PSMTXMultVec(mtx, &vec, pDst);
    }
}
