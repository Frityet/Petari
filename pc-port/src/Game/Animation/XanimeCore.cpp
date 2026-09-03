#include "Game/Animation/XanimeCore.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/JMath/JMath.hpp"

void JMAEulerToQuat(s16, s16, s16, Quaternion*);

void XanimeTrack::init() {
    _0 = nullptr;
    mWeight = 0.0f;
    _C = 0;
    _8 = 0.0f;
}

XjointInfo::XjointInfo() {
    _0._0.set(1.0f, 1.0f, 1.0f);
    _0._C.x = 0.0f;
    _0._C.y = 0.0f;
    _0._C.z = 0.0f;
    _0._20 = 0.0f;
    _0._1C = 0.0f;
    _0._18 = 0.0f;
    _0._24 = 1.0f;
    _28 = _0;
    _5C = 1.0f;
    _60 = 1.0f;
    _50.zero();
}

void XanimeCore::shareJointTransform(const XanimeCore* pOther) {
    mTransformList = pOther->mTransformList;
}

XjointTransform::XjointTransform() {
    _0 = nullptr;
    _4 = 0xFFFF;
    mScale.set(1.0f);
    _14.set(1.0f);
    _64 = nullptr;
    _6C = nullptr;
    _68 = 0;
    _20 = 0.0f;
    _24 = 0.0f;
    _28 = 0.0f;
    _2C.set(0.0f);
    _38.set(0.0f);
    _44 = 0.0f;
    _48 = 0.0f;
    _4C = 0.0f;
    _50 = 0;
    _52 = 0;
    _54 = 0;
    _56 = 0;
    _58 = 0.0f;
    _5C = 0.0f;
    _60 = 0.0f;
}

void XanimeCore::enableJointTransform(J3DModelData* pModelData) {
    mTransformList = new XjointTransform[mJointCount];

    for (u32 i = 0; i < mJointCount; i++) {
        J3DJoint* joint = pModelData->getJointNodePointer(i);
        mTransformList[i]._0 = joint;
        mTransformList[i].mTransformInfo = joint->getTransformInfo();

        if (mTransformList[i]._4 == 0xFFFF) {
            for (u32 j = 0; j < mJointCount; j++) {
                XjointTransform& transform = mTransformList[j];
                if (transform._0 == nullptr) {
                    continue;
                }
                if (transform._0->getChild() == joint) {
                    mTransformList[i]._4 = j;
                    break;
                }
                if (transform._0->getYounger() == joint && transform._4 != 0xFFFF) {
                    mTransformList[i]._4 = transform._4;
                    break;
                }
            }
        }
    }
}

void XanimeCore::reconfigJointTransform(J3DModelData* pModelData) {
    for (u32 i = 0; i < mJointCount; i++) {
        J3DJoint* joint = pModelData->getJointNodePointer(i);
        mTransformList[i]._0 = joint;
        mTransformList[i].mTransformInfo = joint->getTransformInfo();
    }
}

// missing new array call
void XanimeCore::initMember(u32 trackCount) {
    _1C = 1.0f;
    _20 = 1.0f;
    mFrameRatio = 0.0f;
    _28 = 0;
    _29 = 0;
    _6 = 0;
    _C = 0;
    mTrackList = new XanimeTrack[trackCount];

    s32 curTrack = 0;

    while (curTrack < trackCount) {
        mTrackList[curTrack].init();
        curTrack++;
    }

    setWeight(0, 1.0f);
}

XanimeCore::XanimeCore(u32 trackCount, u32 jointCount, u8 a3) {
    _4 = a3;
    mTrackCount = trackCount;
    mJointCount = jointCount;
    mJointList = new XjointInfo[jointCount];
    mTransformList = 0;
    initMember(trackCount);
}

XanimeCore::XanimeCore(u32 trackCount, XanimeCore* pOtherCore) {
    mTrackCount = trackCount;
    mJointCount = pOtherCore->mJointCount;
    _4 = pOtherCore->_4;
    mJointList = pOtherCore->mJointList;
    mTransformList = pOtherCore->mTransformList;
    initMember(trackCount);
}

void XanimeCore::doFreeze() {
    _28 = 1;
    _1C = 0.0f;
}

void XanimeCore::setBck(u32 index, J3DAnmTransform* pAnimation) {
    mTrackList[index]._0 = pAnimation;
    XanimeTrack& track = mTrackList[index];
    track._8 = 0.0f;
    track._C = 1;
    mTrackList[index]._C = 0;
}

void XanimeCore::setWeight(u8 idx, f32 weight) {
    mTrackList[idx].mWeight = weight;
}

void XanimeCore::updateFrame() {
    for (u32 i = 0; i < mTrackCount; i++) {
        XanimeTrack& track = mTrackList[i];
        if (track._0 == nullptr) {
            continue;
        }
        if (track._C) {
            track._0->setFrame(track._8 * track._0->getFrameMax());
        } else {
            track._0->setFrame(mFrameRatio * track._0->getFrameMax());
        }
    }

    if (_28) {
        _28 = 0;
        _29 = 1;
    } else {
        _29 = 0;
    }
}

void XanimeCore::freezeCopy(J3DModelData* pModelData, XanimeCore* pOther, u32 jointIndex, u32 interp) {
    J3DJoint* joint = pModelData->getJointNodePointer(jointIndex);

    J3DJoint* child = joint->getChild();
    if (child != nullptr) {
        freezeCopy(pModelData, pOther, child->getJntNo(), interp);
        while ((child = child->getYounger()) != nullptr) {
            freezeCopy(pModelData, pOther, child->getJntNo(), interp);
        }
    }

    pOther->mJointList[jointIndex]._0 = mJointList[jointIndex]._28;
    pOther->mJointList[jointIndex]._5C = 0.0f;

    f32 rate = 1.0f;
    if (interp) {
        rate = 1.0f / interp;
    }
    pOther->mJointList[jointIndex]._60 = rate;
}

void XanimeCore::calcScaleBlendMaya(const TVec3f& scale, const TVec3f& translation) {
    J3DJoint* joint = getJoint();
    u16 jointIndex = joint->getJntNo();
    J3DMtxBuffer* mtxBuffer = getMtxBuffer();
    MtxPtr matrix = mtxBuffer->getAnmMtx(jointIndex);

    if (mTransformList != nullptr) {
        if (mTransformList[jointIndex]._64 != nullptr) {
            PSMTXConcat(matrix, mTransformList[jointIndex]._64, matrix);
        }
        if (mTransformList[jointIndex]._68 != nullptr) {
            PSMTXConcat(matrix, mTransformList[jointIndex]._68, matrix);
        }
    }

    if (mTransformList != nullptr && mTransformList[jointIndex]._4 != 0xFFFF) {
        MtxPtr parentMatrix = mTransformList[mTransformList[jointIndex]._4]._68;
        if (parentMatrix != nullptr) {
            Mtx inverse;
            PSMTXInverse(parentMatrix, inverse);
            PSMTXConcat(inverse, matrix, matrix);
        }
    }

    matrix[0][3] = translation.x;
    matrix[1][3] = translation.y;
    matrix[2][3] = translation.z;
    if (mTransformList != nullptr) {
        matrix[0][3] += mTransformList[jointIndex]._2C.x;
        matrix[1][3] += mTransformList[jointIndex]._2C.y;
        matrix[2][3] += mTransformList[jointIndex]._2C.z;
    }

    TVec3f localScale(scale);
    if (mTransformList != nullptr) {
        localScale.x *= mTransformList[jointIndex].mScale.x * mTransformList[jointIndex]._14.x;
        localScale.y *= mTransformList[jointIndex].mScale.y * mTransformList[jointIndex]._14.y;
        localScale.z *= mTransformList[jointIndex].mScale.z * mTransformList[jointIndex]._14.z;
    }
    if (localScale.x == 1.0f && localScale.y == 1.0f && localScale.z == 1.0f) {
        mtxBuffer->setScaleFlag(jointIndex, 1);
    } else {
        mtxBuffer->setScaleFlag(jointIndex, 0);
        JMAMTXApplyScale(matrix, matrix, localScale.x, localScale.y, localScale.z);
    }

    if (mTransformList != nullptr && mTransformList[jointIndex]._4 != 0xFFFF) {
        if (mTransformList[mTransformList[jointIndex]._4].mScale.x != 1.0f) {
            f32 inverse = JMath::fastReciprocal(mTransformList[mTransformList[jointIndex]._4].mScale.x);
            matrix[0][0] *= inverse;
            matrix[0][1] *= inverse;
            matrix[0][2] *= inverse;
        }
        if (mTransformList[mTransformList[jointIndex]._4].mScale.y != 1.0f) {
            f32 inverse = JMath::fastReciprocal(mTransformList[mTransformList[jointIndex]._4].mScale.y);
            matrix[1][0] *= inverse;
            matrix[1][1] *= inverse;
            matrix[1][2] *= inverse;
        }
        if (mTransformList[mTransformList[jointIndex]._4].mScale.z != 1.0f) {
            f32 inverse = JMath::fastReciprocal(mTransformList[mTransformList[jointIndex]._4].mScale.z);
            matrix[2][0] *= inverse;
            matrix[2][1] *= inverse;
            matrix[2][2] *= inverse;
        }
    }
    if (joint->getScaleCompensate() == 1) {
        if (J3DSys::mParentS.x != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.x);
            matrix[0][0] *= inverse;
            matrix[0][1] *= inverse;
            matrix[0][2] *= inverse;
        }
        if (J3DSys::mParentS.y != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.y);
            matrix[1][0] *= inverse;
            matrix[1][1] *= inverse;
            matrix[1][2] *= inverse;
        }
        if (J3DSys::mParentS.z != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.z);
            matrix[2][0] *= inverse;
            matrix[2][1] *= inverse;
            matrix[2][2] *= inverse;
        }
    }

    PSMTXConcat(J3DSys::mCurrentMtx, matrix, J3DSys::mCurrentMtx);
    if (mTransformList != nullptr && mTransformList[jointIndex]._6C != nullptr) {
        TVec3f currentTranslation;
        MR::extractMtxTrans(J3DSys::mCurrentMtx, &currentTranslation);
        MR::setMtxTrans(J3DSys::mCurrentMtx, 0.0f, 0.0f, 0.0f);
        PSMTXConcat(mTransformList[jointIndex]._6C, J3DSys::mCurrentMtx, J3DSys::mCurrentMtx);
        MR::setMtxTrans(J3DSys::mCurrentMtx, currentTranslation.x, currentTranslation.y, currentTranslation.z);
    }
    if (mTransformList != nullptr) {
        J3DSys::mCurrentMtx[0][3] += mTransformList[jointIndex]._38.x;
        J3DSys::mCurrentMtx[1][3] += mTransformList[jointIndex]._38.y;
        J3DSys::mCurrentMtx[2][3] += mTransformList[jointIndex]._38.z;
    }
    PSMTXCopy(J3DSys::mCurrentMtx, matrix);
    if (mTransformList != nullptr) {
        matrix[0][3] += mTransformList[jointIndex]._20;
        matrix[1][3] += mTransformList[jointIndex]._24;
        matrix[2][3] += mTransformList[jointIndex]._28;
    }

    J3DSys::mParentS.x = scale.x;
    J3DSys::mParentS.y = scale.y;
    J3DSys::mParentS.z = scale.z;
}

void XanimeCore::calcScaleBlendSI(const TVec3f& scale, const TVec3f& translation) {
    J3DJoint* joint = getJoint();
    u16 jointIndex = joint->getJntNo();
    J3DMtxBuffer* mtxBuffer = getMtxBuffer();
    MtxPtr matrix = mtxBuffer->getAnmMtx(jointIndex);
    Vec& currentScale = J3DSys::mCurrentS;

    if (mTransformList != nullptr) {
        if (mTransformList[jointIndex]._64 != nullptr) {
            PSMTXConcat(matrix, mTransformList[jointIndex]._64, matrix);
        }
        if (mTransformList[jointIndex]._68 != nullptr) {
            PSMTXConcat(matrix, mTransformList[jointIndex]._68, matrix);
        }
    }

    TVec3f localScale(scale);
    if (mTransformList != nullptr) {
        localScale.x *= mTransformList[jointIndex].mScale.x * mTransformList[jointIndex]._14.x;
        localScale.y *= mTransformList[jointIndex].mScale.y * mTransformList[jointIndex]._14.y;
        localScale.z *= mTransformList[jointIndex].mScale.z * mTransformList[jointIndex]._14.z;
    }
    if (localScale.x == 1.0f && localScale.y == 1.0f && localScale.z == 1.0f) {
        mtxBuffer->setScaleFlag(jointIndex, 1);
    } else {
        mtxBuffer->setScaleFlag(jointIndex, 0);
        JMAMTXApplyScale(matrix, matrix, localScale.x, localScale.y, localScale.z);
    }

    if (mTransformList != nullptr && mTransformList[jointIndex]._4 != 0xFFFF) {
        if (mTransformList[mTransformList[jointIndex]._4].mScale.x != 1.0f) {
            f32 inverse = JMath::fastReciprocal(mTransformList[mTransformList[jointIndex]._4].mScale.x);
            matrix[0][0] *= inverse;
            matrix[0][1] *= inverse;
            matrix[0][2] *= inverse;
        }
        if (mTransformList[mTransformList[jointIndex]._4].mScale.y != 1.0f) {
            f32 inverse = JMath::fastReciprocal(mTransformList[mTransformList[jointIndex]._4].mScale.y);
            matrix[1][0] *= inverse;
            matrix[1][1] *= inverse;
            matrix[1][2] *= inverse;
        }
        if (mTransformList[mTransformList[jointIndex]._4].mScale.z != 1.0f) {
            f32 inverse = JMath::fastReciprocal(mTransformList[mTransformList[jointIndex]._4].mScale.z);
            matrix[2][0] *= inverse;
            matrix[2][1] *= inverse;
            matrix[2][2] *= inverse;
        }
    }
    if (joint->getScaleCompensate() == 1) {
        if (J3DSys::mParentS.x != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.x);
            matrix[0][0] *= inverse;
            matrix[0][1] *= inverse;
            matrix[0][2] *= inverse;
        }
        if (J3DSys::mParentS.y != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.y);
            matrix[1][0] *= inverse;
            matrix[1][1] *= inverse;
            matrix[1][2] *= inverse;
        }
        if (J3DSys::mParentS.z != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.z);
            matrix[2][0] *= inverse;
            matrix[2][1] *= inverse;
            matrix[2][2] *= inverse;
        }
    }

    matrix[0][3] = translation.x * currentScale.x;
    matrix[1][3] = translation.y * currentScale.y;
    matrix[2][3] = translation.z * currentScale.z;
    if (mTransformList != nullptr) {
        matrix[0][3] += mTransformList[jointIndex]._2C.x;
        matrix[1][3] += mTransformList[jointIndex]._2C.y;
        matrix[2][3] += mTransformList[jointIndex]._2C.z;
    }

    PSMTXConcat(J3DSys::mCurrentMtx, matrix, J3DSys::mCurrentMtx);
    currentScale.x *= scale.x;
    currentScale.y *= scale.y;
    currentScale.z *= scale.z;
    if (currentScale.x == 1.0f && currentScale.y == 1.0f && currentScale.z == 1.0f) {
        mtxBuffer->setScaleFlag(jointIndex, 1);
        PSMTXCopy(J3DSys::mCurrentMtx, matrix);
    } else {
        mtxBuffer->setScaleFlag(jointIndex, 0);
        JMAMTXApplyScale(J3DSys::mCurrentMtx, matrix, currentScale.x, currentScale.y, currentScale.z);
        matrix[0][3] = J3DSys::mCurrentMtx[0][3];
        matrix[1][3] = J3DSys::mCurrentMtx[1][3];
        matrix[2][3] = J3DSys::mCurrentMtx[2][3];
    }
    if (mTransformList != nullptr) {
        matrix[0][3] += mTransformList[jointIndex]._20;
        matrix[1][3] += mTransformList[jointIndex]._24;
        matrix[2][3] += mTransformList[jointIndex]._28;
    }
}

void XanimeCore::calcScaleBlendSpecial() {
    u16 jointIndex = getJoint()->getJntNo();
    MtxPtr matrix = getMtxBuffer()->getAnmMtx(jointIndex);
    TVec3f scale;
    TVec3f translation;
    scale = mJointList[jointIndex]._28._0;
    translation = mJointList[jointIndex]._28._C;
    PSMTXQuat(matrix, &mJointList[jointIndex]._28.mRotation);
    calcScaleBlendMaya(scale, translation);
}

void XanimeCore::calcBlend(TVec3f* pScale, TVec3f* pTranslation) {
    u16 jointIndex = getJoint()->getJntNo();
    MtxPtr matrix = getMtxBuffer()->getAnmMtx(jointIndex);
    f32 accumulatedWeight = 0.0f;
    pScale->zero();
    pTranslation->zero();
    Quaternion rotation;
    rotation.x = rotation.y = rotation.z = 0.0f;
    rotation.w = 1.0f;

    f32 totalWeight = 0.0f;
    for (s32 i = 0; i < mTrackCount; i++) {
        if (mTrackList[i]._0 != nullptr) {
            totalWeight += mTrackList[i].mWeight;
        }
    }

    if (totalWeight == 0.0f) {
        *pScale = mJointList[jointIndex]._28._0;
        *pTranslation = mJointList[jointIndex]._28._C;
        PSMTXQuat(matrix, &mJointList[jointIndex]._28.mRotation);
        return;
    }

    f32 inverseWeight = 1.0f / totalWeight;
    for (s32 i = 0; i < mTrackCount; i++) {
        XanimeTrack& track = mTrackList[i];
        if (track._0 == nullptr || track.mWeight == 0.0f) {
            continue;
        }

        J3DTransformInfo transform;
        track._0->getTransform(jointIndex, &transform);
        Quaternion trackRotation;
        JMAEulerToQuat(transform.mRotation.x, transform.mRotation.y, transform.mRotation.z, &trackRotation);
        f32 weight = inverseWeight * track.mWeight;
        *pScale += TVec3f(transform.mScale) * weight;
        *pTranslation += TVec3f(transform.mTranslate) * weight;
        accumulatedWeight += weight;
        JMAQuatLerp(&rotation, &trackRotation, weight / accumulatedWeight, &rotation);
    }

    if (_29) {
        mJointList[jointIndex]._0 = mJointList[jointIndex]._28;
    }
    mJointList[jointIndex]._50 = *pTranslation;

    f32 rate = _1C;
    if (rate < 1.0f) {
        XtransformInfo& frozen = mJointList[jointIndex]._0;
        *pScale = frozen._0 * (1.0f - rate) + *pScale * rate;
        *pTranslation = frozen._C * (1.0f - rate) + *pTranslation * rate;
        JMAQuatLerp(&frozen.mRotation, &rotation, rate, &rotation);
    }

    if (_20 != 1.0f) {
        XtransformInfo& previous = mJointList[jointIndex]._28;
        *pScale = previous._0 * (1.0f - _20) + *pScale * _20;
        *pTranslation = previous._C * (1.0f - _20) + *pTranslation * _20;
        JMAQuatLerp(&previous.mRotation, &rotation, _20, &rotation);
    }

    mJointList[jointIndex]._28._0 = *pScale;
    mJointList[jointIndex]._28._C = *pTranslation;
    mJointList[jointIndex]._28.mRotation = rotation;
    PSMTXQuat(matrix, &rotation);
}

void XanimeCore::calcSingle(TVec3f* pScale, TVec3f* pTranslation) {
    u16 jointIndex = getJoint()->getJntNo();
    MtxPtr matrix = getMtxBuffer()->getAnmMtx(jointIndex);
    if (mTrackList[0]._0 == nullptr) {
        *pScale = mJointList[jointIndex]._28._0;
        *pTranslation = mJointList[jointIndex]._28._C;
        PSMTXQuat(matrix, &mJointList[jointIndex]._28.mRotation);
        return;
    }

    J3DTransformInfo transform;
    mTrackList[0]._0->getTransform(jointIndex, &transform);
    Quaternion rotation;
    JMAEulerToQuat(transform.mRotation.x, transform.mRotation.y, transform.mRotation.z, &rotation);
    pScale->set(transform.mScale);
    pTranslation->set(transform.mTranslate);

    if (_29) {
        mJointList[jointIndex]._0 = mJointList[jointIndex]._28;
    }
    mJointList[jointIndex]._50 = *pTranslation;

    f32 rate = _1C;
    if (rate < 1.0f) {
        XtransformInfo& frozen = mJointList[jointIndex]._0;
        MR::vecBlend(frozen._0, *pScale, pScale, rate);
        MR::vecBlend(frozen._C, *pTranslation, pTranslation, rate);
        JMAQuatLerp(&frozen.mRotation, &rotation, rate, &rotation);
    }

    mJointList[jointIndex]._28._0 = *pScale;
    mJointList[jointIndex]._28._C = *pTranslation;
    mJointList[jointIndex]._28.mRotation = rotation;
    PSMTXQuat(matrix, &rotation);
}

void XanimeCore::calcBlendSpecial() {
    u16 jointIndex = getJoint()->getJntNo();
    TVec3f scale(0.0f, 0.0f, 0.0f);
    TVec3f translation(0.0f, 0.0f, 0.0f);
    f32 accumulatedWeight = 0.0f;
    Quaternion rotation;
    rotation.x = rotation.y = rotation.z = 0.0f;
    rotation.w = 1.0f;

    f32 totalWeight = 0.0f;
    for (s32 i = 0; i < mTrackCount; i++) {
        if (mTrackList[i]._0 != nullptr) {
            totalWeight += mTrackList[i].mWeight;
        }
    }

    if (totalWeight == 0.0f) {
        return;
    }

    f32 inverseWeight = 1.0f / totalWeight;
    for (s32 i = 0; i < mTrackCount; i++) {
        XanimeTrack& track = mTrackList[i];
        if (track._0 == nullptr || track.mWeight == 0.0f) {
            continue;
        }

        J3DTransformInfo transform;
        track._0->getTransform(jointIndex, &transform);
        Quaternion trackRotation;
        JMAEulerToQuat(transform.mRotation.x, transform.mRotation.y, transform.mRotation.z, &trackRotation);
        f32 weight = inverseWeight * track.mWeight;
        scale += TVec3f(transform.mScale) * weight;
        translation += TVec3f(transform.mTranslate) * weight;
        accumulatedWeight += weight;
        JMAQuatLerp(&rotation, &trackRotation, weight / accumulatedWeight, &rotation);
    }

    if (_29) {
        mJointList[jointIndex]._0 = mJointList[jointIndex]._28;
    }
    mJointList[jointIndex]._50 = translation;

    f32 rate = _1C;
    if (mJointList[jointIndex]._5C != 1.0f) {
        mJointList[jointIndex]._5C += mJointList[jointIndex]._60;
        mJointList[jointIndex]._5C = MR::clamp(mJointList[jointIndex]._5C, 0.0f, 1.0f);
        rate = mJointList[jointIndex]._5C;
    }

    if (rate < 1.0f) {
        XtransformInfo& frozen = mJointList[jointIndex]._0;
        scale = frozen._0 * (1.0f - rate) + scale * rate;
        translation = frozen._C * (1.0f - rate) + translation * rate;
        JMAQuatLerp(&frozen.mRotation, &rotation, rate, &rotation);
    }

    if (_20 != 1.0f) {
        XtransformInfo& previous = mJointList[jointIndex]._28;
        scale = previous._0 * (1.0f - _20) + scale * _20;
        translation = previous._C * (1.0f - _20) + translation * _20;
        JMAQuatLerp(&previous.mRotation, &rotation, _20, &rotation);
    }

    mJointList[jointIndex]._28._0 = scale;
    mJointList[jointIndex]._28._C = translation;
    mJointList[jointIndex]._28.mRotation = rotation;
}

void XanimeCore::initT(J3DModelData* pModelData) {
    for (u32 i = 0; i < mJointCount; i++) {
        const J3DTransformInfo& transform = pModelData->getJointNodePointer(i)->getTransformInfo();
        Quaternion rotation;
        JMAEulerToQuat(transform.mRotation.x, transform.mRotation.y, transform.mRotation.z, &rotation);
        mJointList[i]._0._0.set(transform.mScale);
        mJointList[i]._0._C.set(transform.mTranslate);
        mJointList[i]._0.mRotation = rotation;
        mJointList[i]._28._0 = mJointList[i]._0._0;
        mJointList[i]._28._C = mJointList[i]._0._C;
        mJointList[i]._28.mRotation = mJointList[i]._0.mRotation;
    }
}

void XanimeCore::fixT(TVec3f* pTranslation) {
    u16 jointIndex = getJoint()->getJntNo();
    J3DModelData* modelData = j3dSys.getModel()->getModelData();
    if (jointIndex == 0 || jointIndex == _C) {
        return;
    }
    pTranslation->set(modelData->getJointNodePointer(jointIndex)->getTransformInfo().mTranslate);
}

XanimeCore::~XanimeCore() {
}

XtransformInfo::XtransformInfo() {
    _0.zero();
    _C.zero();
    _20 = 0.0f;
    _1C = 0.0f;
    _18 = 0.0f;
    _24 = 1.0f;
}

void XanimeCore::calcScaleBlendBasic(const TVec3f& scale, const TVec3f& translation) {
    J3DMtxBuffer* mtxBuffer = getMtxBuffer();
    u16 jointIndex = getJoint()->getJntNo();
    MtxPtr matrix = mtxBuffer->getAnmMtx(jointIndex);
    matrix[0][3] = translation.x;
    matrix[1][3] = translation.y;
    matrix[2][3] = translation.z;

    J3DSys::mCurrentS.x *= scale.x;
    J3DSys::mCurrentS.y *= scale.y;
    J3DSys::mCurrentS.z *= scale.z;

    if (J3DSys::mCurrentS.x == 1.0f && J3DSys::mCurrentS.y == 1.0f && J3DSys::mCurrentS.z == 1.0f) {
        mtxBuffer->setScaleFlag(jointIndex, 1);
    } else {
        mtxBuffer->setScaleFlag(jointIndex, 0);
        JMAMTXApplyScale(matrix, matrix, scale.x, scale.y, scale.z);
    }

    PSMTXConcat(J3DSys::mCurrentMtx, matrix, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, matrix);
}

void XanimeCore::calcScaleBlendMayaNoTransform(const TVec3f& scale, const TVec3f& translation) {
    J3DJoint* joint = getJoint();
    J3DMtxBuffer* mtxBuffer = getMtxBuffer();
    u16 jointIndex = joint->getJntNo();
    MtxPtr matrix = mtxBuffer->getAnmMtx(jointIndex);
    matrix[0][3] = translation.x;
    matrix[1][3] = translation.y;
    matrix[2][3] = translation.z;

    TVec3f localScale(scale);
    if (localScale.x == 1.0f && localScale.y == 1.0f && localScale.z == 1.0f) {
        mtxBuffer->setScaleFlag(jointIndex, 1);
    } else {
        mtxBuffer->setScaleFlag(jointIndex, 0);
        JMAMTXApplyScale(matrix, matrix, localScale.x, localScale.y, localScale.z);
    }

    if (joint->getScaleCompensate() == 1) {
        if (J3DSys::mParentS.x != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.x);
            matrix[0][0] *= inverse;
            matrix[0][1] *= inverse;
            matrix[0][2] *= inverse;
        }
        if (J3DSys::mParentS.y != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.y);
            matrix[1][0] *= inverse;
            matrix[1][1] *= inverse;
            matrix[1][2] *= inverse;
        }
        if (J3DSys::mParentS.z != 1.0f) {
            f32 inverse = JMath::fastReciprocal(J3DSys::mParentS.z);
            matrix[2][0] *= inverse;
            matrix[2][1] *= inverse;
            matrix[2][2] *= inverse;
        }
    }

    PSMTXConcat(J3DSys::mCurrentMtx, matrix, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, matrix);
    J3DSys::mParentS.x = scale.x;
    J3DSys::mParentS.y = scale.y;
    J3DSys::mParentS.z = scale.z;
}

void XanimeCore::calc() {
    j3dSys.mCurrentMtxCalc = this;
    if (_6 == 1) {
        calcBlendSpecial();
        return;
    }
    if (_6 == 2) {
        calcScaleBlendSpecial();
        return;
    }

    TVec3f scale;
    TVec3f translation;
    if (mTrackCount == 1) {
        calcSingle(&scale, &translation);
    } else {
        calcBlend(&scale, &translation);
    }
    if (_6 == 3) {
        fixT(&translation);
    }

    switch (_4) {
    case 1:
        calcScaleBlendSI(scale, translation);
        break;
    case 0:
        if (mTransformList == nullptr) {
            calcScaleBlendBasic(scale, translation);
            break;
        }
    case 2:
        if (mTransformList != nullptr) {
            calcScaleBlendMaya(scale, translation);
        } else {
            calcScaleBlendMayaNoTransform(scale, translation);
        }
        break;
    }
}

void XanimeCore::init(const Vec& scale, const Mtx& matrix) {
    switch (_4) {
    case 0:
    case 1:
    case 2:
        J3DMtxCalcJ3DSysInitMaya::init(scale, matrix);
        break;
    }
}
