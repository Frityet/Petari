#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"
#include "JSystem/JMath/JMath.hpp"

static Mtx mtrans_org = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};

static Mtx tmpmtx_sc = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};

static Mtx tmpmtx_rx = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};

static Mtx tmpmtx_ry = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};

static Mtx tmpmtx_rz = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};

namespace MR {
    void blendMtxRotate(MtxPtr mtxA, MtxPtr mtxB, f32 blend, MtxPtr dst) {
        Quaternion quatA, quatB, quatR;
        C_QUATMtx(&quatA, mtxA);
        C_QUATMtx(&quatB, mtxB);
        JMAQuatLerp(&quatA, &quatB, blend, &quatR);
        PSMTXQuat(dst, &quatR);
    }

    void blendMtxRotateSlerp(MtxPtr mtxA, MtxPtr mtxB, f32 blend, MtxPtr dst) {
        Quaternion quatA, quatB, quatR;
        C_QUATMtx(&quatA, mtxA);
        C_QUATMtx(&quatB, mtxB);
        C_QUATSlerp(&quatA, &quatB, &quatR, blend);
        PSMTXQuat(dst, &quatR);
    }

    void blendMtx(MtxPtr mtxA, MtxPtr mtxB, f32 blend, MtxPtr dst) {
        TVec3f transA, transB;
        extractMtxTrans(mtxA, &transA);
        extractMtxTrans(mtxB, &transB);

        TVec3f scaledB = transB * blend;
        TVec3f scaledA = transA * (1.0f - blend);
        TVec3f transR = scaledA + scaledB;

        Quaternion quatA, quatB, quatR;
        C_QUATMtx(&quatA, mtxA);
        C_QUATMtx(&quatB, mtxB);

        JMAQuatLerp(&quatA, &quatB, blend, &quatR);

        PSMTXQuat(dst, &quatR);
        setMtxTrans(dst, transR.x, transR.y, transR.z);
    }

    void makeMtxWithoutScale(TPos3f* pDst, const TPos3f& rSrc) {
        TVec3f trans;
        rSrc.getTrans(trans);

        TVec3f axisX, axisY, axisZ;
        rSrc.getXYZDir(axisX, axisY, axisZ);

        pDst->setTrans(trans);

        MR::normalize(&axisX);
        MR::normalize(&axisY);
        MR::normalize(&axisZ);

        pDst->setXYZDir(axisX, axisY, axisZ);
    }

    void addTransMtx(MtxPtr mtx, const TVec3f& rVec) {
        mtx[0][3] += rVec.x;
        mtx[1][3] += rVec.y;
        mtx[2][3] += rVec.z;
    }

    void addTransMtxLocal(MtxPtr mtx, const TVec3f& rVec) {
        addTransMtxLocalX(mtx, rVec.x);
        addTransMtxLocalY(mtx, rVec.y);
        addTransMtxLocalZ(mtx, rVec.z);
    }

    void addTransMtxLocalX(MtxPtr mtx, f32 x_coord) {
        mtx[0][3] = mtx[0][3] + (mtx[0][0] * x_coord);
        mtx[1][3] = mtx[1][3] + (mtx[1][0] * x_coord);
        mtx[2][3] = mtx[2][3] + (mtx[2][0] * x_coord);
    }

    void addTransMtxLocalY(MtxPtr mtx, f32 y_coord) {
        mtx[0][3] = mtx[0][3] + (mtx[0][1] * y_coord);
        mtx[1][3] = mtx[1][3] + (mtx[1][1] * y_coord);
        mtx[2][3] = mtx[2][3] + (mtx[2][1] * y_coord);
    }

    void addTransMtxLocalZ(MtxPtr mtx, f32 z_coord) {
        mtx[0][3] = mtx[0][3] + (mtx[0][2] * z_coord);
        mtx[1][3] = mtx[1][3] + (mtx[1][2] * z_coord);
        mtx[2][3] = mtx[2][3] + (mtx[2][2] * z_coord);
    }

    MtxPtr tmpMtxTrans(const TVec3f& rVec) {
        setMtxTrans(mtrans_org, rVec.x, rVec.y, rVec.z);
        return mtrans_org;
    }

    MtxPtr tmpMtxScale(f32 sx, f32 sy, f32 sz) {
        tmpmtx_sc[0][0] = sx;
        tmpmtx_sc[1][1] = sy;
        tmpmtx_sc[2][2] = sz;
        return tmpmtx_sc;
    }

    MtxPtr tmpMtxRotXRad(f32 rad) {
        f32 cosX = JMACosRadian(rad);
        f32 sinX = JMASinRadian(rad);
        tmpmtx_rx[1][1] = cosX;
        tmpmtx_rx[2][1] = sinX;
        tmpmtx_rx[1][2] = -sinX;
        tmpmtx_rx[2][2] = cosX;
        return tmpmtx_rx;
    }

    MtxPtr tmpMtxRotYRad(f32 rad) {
        f32 cosY = JMACosRadian(rad);
        f32 sinY = JMASinRadian(rad);
        tmpmtx_ry[0][0] = cosY;
        tmpmtx_ry[0][2] = sinY;
        tmpmtx_ry[2][0] = -sinY;
        tmpmtx_ry[2][2] = cosY;
        return tmpmtx_ry;
    }

    MtxPtr tmpMtxRotZRad(f32 rad) {
        f32 cosZ = JMACosRadian(rad);
        f32 sinZ = JMASinRadian(rad);
        tmpmtx_rz[0][0] = cosZ;
        tmpmtx_rz[1][0] = sinZ;
        tmpmtx_rz[0][1] = -sinZ;
        tmpmtx_rz[1][1] = cosZ;
        return tmpmtx_rz;
    }

    MtxPtr tmpMtxRotXDeg(f32 deg) {
        f32 cosX = JMACosDegree(deg);
        f32 sinX = JMASinDegree(deg);
        tmpmtx_rx[1][1] = cosX;
        tmpmtx_rx[1][2] = sinX;
        tmpmtx_rx[2][1] = -sinX;
        tmpmtx_rx[2][2] = cosX;
        return tmpmtx_rx;
    }

    MtxPtr tmpMtxRotYDeg(f32 deg) {
        f32 cosY = JMACosDegree(deg);
        f32 sinY = JMASinDegree(deg);
        tmpmtx_ry[0][0] = cosY;
        tmpmtx_ry[0][2] = -sinY;
        tmpmtx_ry[2][0] = sinY;
        tmpmtx_ry[2][2] = cosY;
        return tmpmtx_ry;
    }

    MtxPtr tmpMtxRotZDeg(f32 deg) {
        f32 cosZ = JMACosDegree(deg);
        f32 sinZ = JMASinDegree(deg);
        tmpmtx_rz[0][0] = cosZ;
        tmpmtx_rz[0][1] = sinZ;
        tmpmtx_rz[1][0] = -sinZ;
        tmpmtx_rz[1][1] = cosZ;
        return tmpmtx_rz;
    }

    void orderRotateMtx(s16 order, const TVec3f& rRad, MtxPtr dst) {
        const TVec3f* rad = &rRad;
        MtxPtr mtxX = tmpMtxRotXRad(rad->x);
        MtxPtr mtxY = tmpMtxRotYRad(rad->y);
        MtxPtr mtxZ = tmpMtxRotZRad(rad->z);

        MtxPtr first, second, third;

        switch (order) {
        case 0:
            first = mtxY;
            second = mtxX;
            third = mtxZ;
            break;
        case 1:
            first = mtxZ;
            second = mtxX;
            third = mtxY;
            break;
        case 2:
            first = mtxX;
            second = mtxY;
            third = mtxZ;
            break;
        case 3:
            first = mtxZ;
            second = mtxY;
            third = mtxX;
            break;
        case 4:
            first = mtxX;
            second = mtxZ;
            third = mtxZ;
            break;
        case 5:
        default:
            first = mtxY;
            second = mtxZ;
            third = mtxX;
            break;
        }

        MR::multMtx(dst, second, first);
        MR::multMtx(dst, dst, third);
    }

}  // namespace MR
