#include "Game/Util/MathUtil.hpp"

namespace MR {
    bool makeAxisAndCosignVecToVec(TVec3f* pAxis, f32* pCos, const TVec3f& rFrom, const TVec3f& rTo) {
        TVec3f from;
        if (isOppositeDirection(rFrom, rTo)) {
            turnRandomVector(&from, rFrom, 0.01f);
            normalize(&from);
        } else {
            from.set(rFrom);
        }

        TVec3f axis = from.cross(rTo);

        if (isNearZero(axis)) {
            pAxis->zero();
            *pCos = 1.0f;

            return false;
        } else {
            normalize(axis, pAxis);
            *pCos = JGeometry::TUtil< f32 >::clamp(from.dot(rTo), -1.0f, 1.0f);

            return true;
        }
    }

    f32 diffAngleAbsHorizontal(const TVec3f& rA, const TVec3f& rB, const TVec3f& rAxis) {
        TVec3f horizonA, horizonB;

        vecKillElement(rA, rAxis, &horizonA);
        vecKillElement(rB, rAxis, &horizonB);

        return diffAngleAbs(horizonA, horizonB);
    }

    f32 diffAngleSignedHorizontal(const TVec3f& rA, const TVec3f& rB, const TVec3f& rAxis) {
        TVec3f horizonA, horizonB;
        vecKillElement(rA, rAxis, &horizonA);
        vecKillElement(rB, rAxis, &horizonB);
        f32 angleAbs = diffAngleAbs(horizonA, horizonB);
        TPos3f mtx;
        PSMTXRotAxisRad(mtx, rAxis, 0.00017453293f);  // TODO: written directly?
        f32 angle1 = horizonA.dot(horizonB);
        PSMTXMultVec(mtx, horizonA, horizonA);
        f32 angle2 = horizonA.dot(horizonB);

        if (angle2 > angle1) {
            return angleAbs;
        } else {
            return -angleAbs;
        }
    }
}
