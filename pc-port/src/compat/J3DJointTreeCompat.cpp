#include "JSystem/J3DGraphAnimator/J3DJointTree.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

// Original joint-tree and animation-buffer construction/calculation bodies.

J3DJointTree::J3DJointTree()
    : mHierarchy(NULL), mFlags(0), mModelDataType(0), mRootNode(NULL), mBasicMtxCalc(NULL), mJointNodePointer(NULL), mJointNum(0), mWEvlpMtxNum(0),
      mWEvlpMixMtxNum(0), mWEvlpMixMtxIndex(0), mWEvlpMixWeight(0), mInvJointMtx(NULL), mWEvlpImportantMtxIdx(0), field_0x40(0), mJointName(NULL) {
}

void J3DJointTree::calc(J3DMtxBuffer* pMtxBuffer, Vec const& scale, f32 const (&mtx)[3][4]) {
    getBasicMtxCalc()->init(scale, mtx);
    getBasicMtxCalc()->setMtxBuffer(pMtxBuffer);

    J3DJoint* root = getRootNode();
    if (root == NULL)
        return;

    root->setCurrentMtxCalc(getBasicMtxCalc());
    root->recursiveCalc();
}

void J3DMtxCalc::setMtxBuffer(J3DMtxBuffer* mtxBuffer) {
    J3DMtxCalc::mMtxBuffer = mtxBuffer;
}

J3DJointTree::~J3DJointTree() {
}

J3DDrawMtxData::J3DDrawMtxData() {
    mEntryNum = 0;
    mDrawMtxFlag = NULL;
    mDrawMtxIndex = NULL;
}

J3DDrawMtxData::~J3DDrawMtxData() {
}

// Original zero-initialized J3DSys traversal globals (RMGK01 BSS).
Mtx J3DSys::mCurrentMtx;
Vec J3DSys::mCurrentS;
Vec J3DSys::mParentS;

void J3DJointTree::findImportantMtxIndex() {
    s32 wEvlpMtxNum = getWEvlpMtxNum();
    u32 tableIdx = 0;
    u16 drawFullWgtMtxNum = getDrawFullWgtMtxNum();
    u16* wEvlpMixIndex = getWEvlpMixMtxIndex();
    f32* wEvlpMixWeight = getWEvlpMixWeight();
    u16* wEvlpImportantMtxIdx = getWEvlpImportantMtxIndex();

    for (u16 i = 0; i < drawFullWgtMtxNum; i++) {
        wEvlpImportantMtxIdx[i] = getDrawMtxIndex(i);
    }

    for (s32 i = 0; i < wEvlpMtxNum; i++) {
        s32 mixNum = getWEvlpMixMtxNum(i);
        u16 bestIdx = 0;
        f32 bestWeight = -0.1f;

        for (s32 j = 0; j < mixNum; j++, tableIdx++) {
            if (bestWeight < wEvlpMixWeight[tableIdx]) {
                bestWeight = wEvlpMixWeight[tableIdx];
                bestIdx = wEvlpMixIndex[tableIdx];
            }
        }

        wEvlpImportantMtxIdx[i + getDrawFullWgtMtxNum()] = bestIdx;
    }
}
