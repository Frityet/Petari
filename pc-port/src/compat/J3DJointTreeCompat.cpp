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

void J3DMtxBuffer::initialize() {
    mJointTree = NULL;
    mpScaleFlagArr = NULL;
    mpEvlpScaleFlagArr = NULL;
    mpAnmMtx = NULL;
    mpWeightEvlpMtx = NULL;
    mpDrawMtxArr[0] = NULL;
    mpDrawMtxArr[1] = NULL;
    mpNrmMtxArr[0] = NULL;
    mpNrmMtxArr[1] = NULL;
    mpBumpMtxArr[0] = NULL;
    mpBumpMtxArr[1] = NULL;
    mMtxNum = 1;
    mCurrentViewNo = 0;
    mpUserAnmMtx = NULL;
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
