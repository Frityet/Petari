#include "Game/Animation/XanimeCore.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

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
