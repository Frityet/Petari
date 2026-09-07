#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/JMath/JMath.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

#include <dolphin/ppc_math.h>

// Match the original JSystem floating-point contraction setting.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

const J3DTransformInfo j3dDefaultTransformInfo = {{1.0f, 1.0f, 1.0f}, {0, 0, 0}, {0.0f, 0.0f, 0.0f}};

void J3DMtxCalcJ3DSysInitBasic::init(Vec const& scale, Mtx const& mtx) {
    J3DSys::mCurrentS = scale;
    J3DSys::mParentS = (Vec){1.0f, 1.0f, 1.0f};
    JMAMTXApplyScale(mtx, J3DSys::mCurrentMtx, J3DSys::mCurrentS.x, J3DSys::mCurrentS.y, J3DSys::mCurrentS.z);
}

void J3DMtxCalcJ3DSysInitMaya::init(Vec const& scale, Mtx const& mtx) {
    J3DSys::mParentS = (Vec){1.0f, 1.0f, 1.0f};
    J3DSys::mCurrentS = scale;
    JMAMTXApplyScale(mtx, J3DSys::mCurrentMtx, J3DSys::mCurrentS.x, J3DSys::mCurrentS.y, J3DSys::mCurrentS.z);
}

J3DMtxBuffer* J3DMtxCalc::mMtxBuffer;

J3DJoint* J3DMtxCalc::mJoint;

inline s32 checkScaleOne(const Vec& param_0) {
    if (param_0.x == 1.0f && param_0.y == 1.0f && param_0.z == 1.0f) {
        return true;
    } else {
        return false;
    }
}

void J3DMtxCalcCalcTransformBasic::calcTransform(J3DTransformInfo const& transInfo) {
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo = J3DMtxCalc::getJoint()->getJntNo();

    MtxPtr anmMtx = mtxBuf->getAnmMtx(jntNo);

    J3DSys::mCurrentS.x *= transInfo.mScale.x;
    J3DSys::mCurrentS.y *= transInfo.mScale.y;
    J3DSys::mCurrentS.z *= transInfo.mScale.z;
    J3DGetTranslateRotateMtx(transInfo, anmMtx);

    if (!checkScaleOne(J3DSys::mCurrentS)) {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(anmMtx, anmMtx, transInfo.mScale.x, transInfo.mScale.y, transInfo.mScale.z);
    } else {
        mtxBuf->setScaleFlag(jntNo, 1);
    }

    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);
}

void J3DMtxCalcCalcTransformSoftimage::calcTransform(J3DTransformInfo const& transInfo) {
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo = J3DMtxCalc::getJoint()->getJntNo();

    MtxPtr anmMtx = mtxBuf->getAnmMtx(jntNo);

    J3DGetTranslateRotateMtx(transInfo.mRotation.x, transInfo.mRotation.y, transInfo.mRotation.z, transInfo.mTranslate.x * J3DSys::mCurrentS.x,
                             transInfo.mTranslate.y * J3DSys::mCurrentS.y, transInfo.mTranslate.z * J3DSys::mCurrentS.z, anmMtx);
    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);

    J3DSys::mCurrentS.x *= transInfo.mScale.x;
    J3DSys::mCurrentS.y *= transInfo.mScale.y;
    J3DSys::mCurrentS.z *= transInfo.mScale.z;

    if (!checkScaleOne(J3DSys::mCurrentS)) {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentS.x, J3DSys::mCurrentS.y, J3DSys::mCurrentS.z);
        anmMtx[0][3] = J3DSys::mCurrentMtx[0][3];
        anmMtx[1][3] = J3DSys::mCurrentMtx[1][3];
        anmMtx[2][3] = J3DSys::mCurrentMtx[2][3];
    } else {
        mtxBuf->setScaleFlag(jntNo, 1);
        PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);
    }
}

void J3DMtxCalcCalcTransformMaya::calcTransform(J3DTransformInfo const& transInfo) {
    J3DJoint* joint = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();

    u16 jntNo = joint->getJntNo();

    MtxPtr anmMtx = mtxBuf->getAnmMtx(jntNo);

    J3DGetTranslateRotateMtx(transInfo, anmMtx);

    if (transInfo.mScale.x == 1.0f && transInfo.mScale.y == 1.0f && transInfo.mScale.z == 1.0f) {
        mtxBuf->setScaleFlag(jntNo, 1);
    } else {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(anmMtx, anmMtx, transInfo.mScale.x, transInfo.mScale.y, transInfo.mScale.z);
    }

    if (joint->getScaleCompensate() == 1) {
        f32 invX = JMath::fastReciprocal(J3DSys::mParentS.x);
        f32 invY = JMath::fastReciprocal(J3DSys::mParentS.y);
        f32 invZ = JMath::fastReciprocal(J3DSys::mParentS.z);

        anmMtx[0][0] *= invX;
        anmMtx[0][1] *= invX;
        anmMtx[0][2] *= invX;
        anmMtx[1][0] *= invY;
        anmMtx[1][1] *= invY;
        anmMtx[1][2] *= invY;
        anmMtx[2][0] *= invZ;
        anmMtx[2][1] *= invZ;
        anmMtx[2][2] *= invZ;
    }

    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);

    J3DSys::mParentS.x = transInfo.mScale.x;
    J3DSys::mParentS.y = transInfo.mScale.y;
    J3DSys::mParentS.z = transInfo.mScale.z;
}

void J3DJoint::appendChild(J3DJoint* pChild) {
    if (mChild == NULL) {
        mChild = pChild;
    } else {
        J3DJoint* curChild = mChild;
        while (curChild->getYounger() != NULL) {
            curChild = curChild->getYounger();
        }
        curChild->setYounger(pChild);
    }
}

J3DJoint::J3DJoint() {
    mCallBackUserData = NULL;
    mCallBack = NULL;
    field_0x8 = NULL;
    mChild = NULL;
    mYounger = NULL;
    mJntNo = 0;
    mKind = 1;
    mScaleCompensate = false;
    __memcpy(&mTransformInfo, &j3dDefaultTransformInfo, sizeof(J3DTransformInfo));
    mBoundingSphereRadius = 0.0f;
    mMtxCalc = NULL;
    mMesh = NULL;

    Vec init = {0.0f, 0.0f, 0.0f};
    mMin = init;
    Vec init2 = {0.0f, 0.0f, 0.0f};
    mMax = init2;
}

J3DMtxCalc* J3DJoint::mCurrentMtxCalc;

void J3DJoint::recursiveCalc() {
    Mtx currentMtx;
    Vec currentScale;
    Vec parentScale;
    J3DMtxCalc* previousCalc = nullptr;

    PSMTXCopy(J3DSys::mCurrentMtx, currentMtx);
    currentScale = J3DSys::mCurrentS;
    parentScale = J3DSys::mParentS;

    if (mMtxCalc != nullptr) {
        previousCalc = mCurrentMtxCalc;
        J3DMtxCalc::setJoint(this);
        mCurrentMtxCalc = mMtxCalc;
        mMtxCalc->calc();
    } else if (mCurrentMtxCalc != nullptr) {
        J3DMtxCalc::setJoint(this);
        mCurrentMtxCalc->calc();
    }

    J3DJointCallBack callback = mCallBack;
    if (callback != nullptr) {
        callback(this, 0);
    }

    if (mChild != nullptr) {
        mChild->recursiveCalc();
    }

    PSMTXCopy(currentMtx, J3DSys::mCurrentMtx);
    J3DSys::mCurrentS = currentScale;
    J3DSys::mParentS = parentScale;
    if (previousCalc != nullptr) {
        mCurrentMtxCalc = previousCalc;
    }

    if (callback != nullptr) {
        callback(this, 1);
    }

    if (mYounger != nullptr) {
        mYounger->recursiveCalc();
    }
}

void JMAMTXApplyScale(const Mtx src, Mtx dst, f32 x, f32 y, f32 z) {
    Mtx scale;
    PSMTXScale(scale, x, y, z);
    PSMTXConcat(src, scale, dst);
}

void J3DGetTranslateRotateMtx(const J3DTransformInfo& tx, Mtx dst) {
    f32 cxsz;
    f32 sxcz;

    f32 sx = JMASSin(tx.mRotation.x), cx = JMASCos(tx.mRotation.x);
    f32 sy = JMASSin(tx.mRotation.y), cy = JMASCos(tx.mRotation.y);
    f32 sz = JMASSin(tx.mRotation.z), cz = JMASCos(tx.mRotation.z);

    dst[2][0] = -sy;
    dst[0][0] = cz * cy;
    dst[1][0] = sz * cy;
    dst[2][1] = cy * sx;
    dst[2][2] = cy * cx;

    cxsz = cx * sz;
    sxcz = sx * cz;
    dst[0][1] = sxcz * sy - cxsz;
    dst[1][2] = cxsz * sy - sxcz;

    cxsz = sx * sz;
    sxcz = cx * cz;
    dst[0][2] = sxcz * sy + cxsz;
    dst[1][1] = cxsz * sy + sxcz;

    dst[0][3] = tx.mTranslate.x;
    dst[1][3] = tx.mTranslate.y;
    dst[2][3] = tx.mTranslate.z;
}

void J3DGetTranslateRotateMtx(s16 rx, s16 ry, s16 rz, f32 tx, f32 ty, f32 tz, Mtx dst) {
    f32 cxsz;
    f32 sxcz;

    f32 sx = JMASSin(rx), cx = JMASCos(rx);
    f32 sy = JMASSin(ry), cy = JMASCos(ry);
    f32 sz = JMASSin(rz), cz = JMASCos(rz);

    dst[2][0] = -sy;
    dst[0][0] = cz * cy;
    dst[1][0] = sz * cy;
    dst[2][1] = cy * sx;
    dst[2][2] = cy * cx;

    cxsz = cx * sz;
    sxcz = sx * cz;
    dst[0][1] = sxcz * sy - cxsz;
    dst[1][2] = cxsz * sy - sxcz;

    cxsz = sx * sz;
    sxcz = cx * cz;
    dst[0][2] = sxcz * sy + cxsz;
    dst[1][1] = cxsz * sy + sxcz;

    dst[0][3] = tx;
    dst[1][3] = ty;
    dst[2][3] = tz;
}

f32 JMath::fastReciprocal(f32 value) {
    return ppc_fres(value);
}
