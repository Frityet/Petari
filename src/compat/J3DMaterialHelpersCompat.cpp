#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

// Complete original base material methods, excluding block factories and size queries.
void J3DMaterial::initialize() {
    mShape = NULL;
    mNext = NULL;
    mJoint = NULL;
    mMaterialMode = 1;
    mIndex = -1;
    mInvalid = 0;
    mDiffFlag = 0;
    mColorBlock = NULL;
    mTexGenBlock = NULL;
    mTevBlock = NULL;
    mIndBlock = NULL;
    mPEBlock = NULL;
    mpOrigMaterial = NULL;
    mMaterialAnm = NULL;
    mSharedDLObj = NULL;
}

u32 J3DMaterial::countDLSize() {
    return (mColorBlock->countDLSize() + mTexGenBlock->countDLSize() + mTevBlock->countDLSize() + mIndBlock->countDLSize() + mPEBlock->countDLSize() +
            31) &
           ~0x1f;
}

void J3DMaterial::makeDisplayList_private(J3DDisplayListObj* pDLObj) {
    pDLObj->beginDL();
    mTevBlock->load();
    mIndBlock->load();
    mPEBlock->load();
    J3DGDSetGenMode(mTexGenBlock->getTexGenNum(), mColorBlock->getColorChanNum(), mTevBlock->getTevStageNum(), mIndBlock->getIndTexStageNum(),
                    (GXCullMode)(u8)mColorBlock->getCullMode());
    mTexGenBlock->load();
    mColorBlock->load();
    J3DGDSetNumChans(mColorBlock->getColorChanNum());
    J3DGDSetNumTexGens(mTexGenBlock->getTexGenNum());
    pDLObj->endDL();
}

void J3DMaterial::makeDisplayList() {
    if (!j3dSys.getMatPacket()->isLocked()) {
        j3dSys.getMatPacket()->mDiffFlag = mDiffFlag;
        makeDisplayList_private(j3dSys.getMatPacket()->getDisplayListObj());
    }
}

void J3DMaterial::makeSharedDisplayList() {
    makeDisplayList_private(mSharedDLObj);
}

void J3DMaterial::load() {
    j3dSys.setMaterialMode(mMaterialMode);
    if (!j3dSys.checkFlag(2)) {
        loadNBTScale(*mTexGenBlock->getNBTScale());
    }
}

void J3DMaterial::loadSharedDL() {
    j3dSys.setMaterialMode(mMaterialMode);
    if (!j3dSys.checkFlag(2)) {
        mSharedDLObj->callDL();
        loadNBTScale(*mTexGenBlock->getNBTScale());
    }
}

void J3DMaterial::patch() {
    j3dSys.getMatPacket()->mDiffFlag = mDiffFlag;
    j3dSys.getMatPacket()->beginPatch();
    mTevBlock->patch();
    mColorBlock->patch();
    mTexGenBlock->patch();
    j3dSys.getMatPacket()->endPatch();
}

void J3DMaterial::diff(u32 diffFlags) {
    if (j3dSys.getMatPacket()->isEnabled_Diff()) {
        j3dSys.getMatPacket()->beginDiff();

        mTevBlock->diff(diffFlags);
        mIndBlock->diff(diffFlags);
        mPEBlock->diff(diffFlags);
        if (diffFlags & J3DDiffFlag_KonstColor) {
            J3DGDSetGenMode_3Param(mTexGenBlock->getTexGenNum(), mTevBlock->getTevStageNum(), mIndBlock->getIndTexStageNum());
            J3DGDSetNumTexGens(mTexGenBlock->getTexGenNum());
        }
        mTexGenBlock->diff(diffFlags);
        mColorBlock->diff(diffFlags);

        j3dSys.getMatPacket()->endDiff();
    }
}

void J3DMaterial::calc(f32 const (*param_0)[4]) {
    if (j3dSys.checkFlag(0x40000000)) {
        mTexGenBlock->calcPostTexMtx(param_0);
    } else {
        mTexGenBlock->calc(param_0);
    }

    calcCurrentMtx();
    setCurrentMtx();
}

void J3DMaterial::calcDiffTexMtx(f32 const (*param_0)[4]) {
    if (j3dSys.checkFlag(0x40000000)) {
        mTexGenBlock->calcPostTexMtxWithoutViewMtx(param_0);
    } else {
        mTexGenBlock->calcWithoutViewMtx(param_0);
    }
}

void J3DMaterial::setCurrentMtx() {
    mShape->setCurrentMtx(mCurrentMtx);
}

void J3DMaterial::calcCurrentMtx() {
    if (!j3dSys.checkFlag(0x40000000)) {
        mCurrentMtx.setCurrentTexMtx(getTexCoord(0)->getTexGenMtx(), getTexCoord(1)->getTexGenMtx(), getTexCoord(2)->getTexGenMtx(),
                                     getTexCoord(3)->getTexGenMtx(), getTexCoord(4)->getTexGenMtx(), getTexCoord(5)->getTexGenMtx(),
                                     getTexCoord(6)->getTexGenMtx(), getTexCoord(7)->getTexGenMtx());
    } else {
        mCurrentMtx.setCurrentTexMtx(getTexCoord(0)->getTexMtxReg(), getTexCoord(1)->getTexMtxReg(), getTexCoord(2)->getTexMtxReg(),
                                     getTexCoord(3)->getTexMtxReg(), getTexCoord(4)->getTexMtxReg(), getTexCoord(5)->getTexMtxReg(),
                                     getTexCoord(6)->getTexMtxReg(), getTexCoord(7)->getTexMtxReg());
    }
}

void J3DMaterial::copy(J3DMaterial* pOther) {
    mColorBlock->reset(pOther->mColorBlock);
    mTexGenBlock->reset(pOther->mTexGenBlock);
    mTevBlock->reset(pOther->mTevBlock);
    mIndBlock->reset(pOther->mIndBlock);
    mPEBlock->reset(pOther->mPEBlock);
}

void J3DMaterial::reset() {
    if ((~mDiffFlag & J3DDiffFlag_Changed) == 0) {
        mDiffFlag &= ~J3DDiffFlag_Changed;
        mMaterialMode = mpOrigMaterial->mMaterialMode;
        mInvalid = mpOrigMaterial->mInvalid;
        mMaterialAnm = NULL;
        copy(mpOrigMaterial);
    }
}

void J3DMaterial::change() {
    if ((mDiffFlag & (J3DDiffFlag_Changed | J3DDiffFlag_Unk40000000)) == 0) {
        mDiffFlag |= J3DDiffFlag_Changed;
        mMaterialAnm = NULL;
    }
}

s32 J3DMaterial::newSharedDisplayList(u32 dlSize) {
    if (mSharedDLObj == NULL) {
        mSharedDLObj = new J3DDisplayListObj();
        if (mSharedDLObj == NULL) {
            return kJ3DError_Alloc;
        }

        s32 ret = mSharedDLObj->newDisplayList(dlSize);
        if (ret != kJ3DError_Success) {
            return ret;
        }
    }

    return kJ3DError_Success;
}

s32 J3DMaterial::newSingleSharedDisplayList(u32 dlSize) {
    if (mSharedDLObj == NULL) {
        mSharedDLObj = new J3DDisplayListObj();
        if (mSharedDLObj == NULL) {
            return kJ3DError_Alloc;
        }

        s32 ret = mSharedDLObj->newSingleDisplayList(dlSize);
        if (ret != kJ3DError_Success) {
            return ret;
        }
    }

    return kJ3DError_Success;
}

// Original load helper from src/JSystem/J3DGraphBase/J3DTevs.cpp.
void loadNBTScale(J3DNBTScale& NBTScale) {
    if (NBTScale.mbHasScale == true) {
        j3dSys.setNBTScale(&NBTScale.mScale);
    } else {
        j3dSys.setNBTScale(NULL);
    }
}
