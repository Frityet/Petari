#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/Animation/MaterialAnmBuffer.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util.hpp"
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <algorithm>

DisplayListMaker::DisplayListMaker(J3DModel* pModel, const ResourceHolder* pResHolder)
    : mMaterialCtrl(), mModel(pModel), mFogCtrl(nullptr), mResHolder(pResHolder) {
    u16 materialNum = mModel->mModelData->getMaterialNum();
    mPrgFlag = new u32[materialNum];
    mCurFlag = new u32[materialNum];
    MR::zeroMemory(mPrgFlag, materialNum * sizeof(u32));
    MR::zeroMemory(mCurFlag, materialNum * sizeof(u32));
    mMaterialCtrl.init(materialNum * 4);
}

void DisplayListMaker::update() {
    std::for_each(mMaterialCtrl.begin(), mMaterialCtrl.end(), std::mem_func(&MaterialCtrl::update));
}

void DisplayListMaker::diff() {
    for (u16 i = 0; i < mModel->mModelData->getMaterialNum(); i++) {
        if (mCurFlag[i] != 0) {
            MR::ProhibitSchedulerAndInterrupts prohibit(false);
            j3dSys.setMatPacket(mModel->getMatPacket(i));
            J3DMaterial* material = mModel->mModelData->getMaterialNodePointer(i);
            material->diff(getDiffFlag(i));
        }
    }
}

void DisplayListMaker::newDifferedDisplayList() {
    checkMaterial();

    for (u16 i = 0; i < mModel->mModelData->getMaterialNum(); i++) {
        u32 flag = getDiffFlag(i);
        if (flag != 0) {
            mModel->getMatPacket(i)->getShapePacket()->newDifferedDisplayList(flag);
        }
    }
}

bool DisplayListMaker::isValidDiff() {
    return true;
}

void DisplayListMaker::onPrgFlag(u16 index, u32 flag) {
    mPrgFlag[index] |= flag;
}

void DisplayListMaker::onCurFlag(u16 index, u32 flag) {
    mCurFlag[index] |= flag;
}

u32 DisplayListMaker::getDiffFlag(s32 index) const {
    u32 flag = mPrgFlag[index];
    if (mResHolder->mMaterialBuf != nullptr) {
        flag |= mResHolder->mMaterialBuf->getDiffFlag(index);
    }
    return flag;
}

FogCtrl* DisplayListMaker::addFogCtrl(bool a1) {
    FogCtrl* fog = new FogCtrl(mModel->mModelData, a1);
    push(fog);
    mFogCtrl = fog;
    onBothFlagAll(0x10000000);
    return fog;
}

MatColorCtrl* DisplayListMaker::addMatColorCtrl(const char* a1, u32 a2, const J3DGXColor* a3) {
    MatColorCtrl* ctrl = new MatColorCtrl(mModel->mModelData, a1, a2, a3);
    push(ctrl);
    s16 materialNo = MR::getMaterialNo(mModel->mModelData, a1);
    onBothFlag(materialNo, 1);
    return ctrl;
}

TexMtxCtrl* DisplayListMaker::addTexMtxCtrl(const char* a1) {
    TexMtxCtrl* ctrl = new TexMtxCtrl(mModel->mModelData, a1);
    push(ctrl);
    s16 materialNo = MR::getMaterialNo(mModel->mModelData, a1);
    onBothFlag(materialNo, 0x200);
    return ctrl;
}

ProjmapEffectMtxSetter* DisplayListMaker::addProjmapEffectMtxSetter() {
    ProjmapEffectMtxSetter* setter = new ProjmapEffectMtxSetter(mModel, mResHolder);
    push(setter);
    return setter;
}

// DisplayListMaker::addMirrorReflectionMtxSetter

void DisplayListMaker::offCurFlagBpk(const J3DAnmBase* pAnimation) {
    MR::offDiffFlagBpk(mCurFlag, static_cast< const J3DAnmColorKey* >(pAnimation), "");
}

void DisplayListMaker::onCurFlagBtp(const J3DAnmBase* pAnimation) {
    MR::onDiffFlagBtp(mCurFlag, static_cast< const J3DAnmTexPattern* >(pAnimation), "");
}

void DisplayListMaker::offCurFlagBtp(const J3DAnmBase* pAnimation) {
    MR::offDiffFlagBtp(mCurFlag, static_cast< const J3DAnmTexPattern* >(pAnimation), "");
}

void DisplayListMaker::onCurFlagBtk(const J3DAnmBase* pAnimation) {
    MR::onDiffFlagBtk(mCurFlag, static_cast< const J3DAnmTextureSRTKey* >(pAnimation), "");
}

void DisplayListMaker::offCurFlagBtk(const J3DAnmBase* pAnimation) {
    MR::offDiffFlagBtk(mCurFlag, static_cast< const J3DAnmTextureSRTKey* >(pAnimation), "");
}

void DisplayListMaker::onCurFlagBrk(const J3DAnmBase* pAnimation) {
    MR::onDiffFlagBrk(mCurFlag, static_cast< const J3DAnmTevRegKey* >(pAnimation), "");
}

void DisplayListMaker::offCurFlagBrk(const J3DAnmBase* pAnimation) {
    MR::offDiffFlagBrk(mCurFlag, static_cast< const J3DAnmTevRegKey* >(pAnimation), "");
}

void DisplayListMaker::push(MaterialCtrl* pCtrl) {
    mMaterialCtrl.push_back(pCtrl);
}

void DisplayListMaker::onBothFlag(u16 index, u32 flag) {
    mPrgFlag[index] |= flag;
    mCurFlag[index] |= flag;
}

void DisplayListMaker::onBothFlagAll(u32 flag) {
    for (u16 i = 0; i < mModel->mModelData->getMaterialNum(); i++) {
        onBothFlag(i, flag);
    }
}

void DisplayListMaker::checkMaterial() {
    checkTexture();

    for (u16 i = 0; i < mModel->mModelData->getMaterialNum(); i++) {
        if (!MR::isNormalTexMtx(mModel->mModelData->getMaterialNodePointer(i))) {
            onBothFlag(i, 0x200);
        }
    }

    checkViewProjmapEffectMtx();
}

void DisplayListMaker::checkViewProjmapEffectMtx() {
    for (u16 i = 0; i < mModel->mModelData->getMaterialNum(); i++) {
        J3DMaterial* material = mModel->mModelData->getMaterialNodePointer(i);
        for (u32 j = 0; j < 8; j++) {
            J3DTexMtx* texMtx = material->mTexGenBlock->getTexMtx(j);
            if (texMtx != nullptr && (texMtx->getTexMtxInfo().mInfo & 0x3F) == static_cast< u32 >(J3DTexMtxMode_ViewProjmap) &&
                MR::isUseTexMtx(material, j)) {
                addViewProjmapEffectMtxSetter();
                return;
            }
        }
    }
}

ViewProjmapEffectMtxSetter* DisplayListMaker::addViewProjmapEffectMtxSetter() {
    ViewProjmapEffectMtxSetter* setter = new ViewProjmapEffectMtxSetter(mModel->mModelData);
    push(setter);
    return setter;
}

MarioShadowProjmapMtxSetter* DisplayListMaker::addMarioShadowProjmapMtxSetter() {
    MarioShadowProjmapMtxSetter* setter = new MarioShadowProjmapMtxSetter(mModel, mResHolder);
    push(setter);
    return setter;
}

void DisplayListMaker::onCurFlagBpk(const J3DAnmBase* pAnimation) {
    MR::onDiffFlagBpk(mCurFlag, static_cast< const J3DAnmColorKey* >(pAnimation), "");
}

void DisplayListMaker::checkTexture() {
    bool hasShadowSetter = false;
    for (u16 i = 0; i < mModel->mModelData->getTexture()->getNum(); i++) {
        const char* name = mModel->mModelData->getTextureName()->getName(i);
        if (MR::isEqualString(name, "IndDummy")) {
            for (u16 j = 0; j < mModel->mModelData->getMaterialNum(); j++) {
                if (MR::isUseTex(mModel->mModelData->getMaterialNodePointer(j), i)) {
                    onBothFlag(j, 0x04020000);
                }
            }
            mModel->mModelData->getTexture()->setResTIMG(i, *MR::getScreenResTIMG());
        }
        if (MR::isEqualString(name, "ShadowProjDummy")) {
            for (u16 j = 0; j < mModel->mModelData->getMaterialNum(); j++) {
                if (MR::isUseTex(mModel->mModelData->getMaterialNodePointer(j), i)) {
                    onBothFlag(j, 0x04020000);
                }
            }
            mModel->mModelData->getTexture()->setResTIMG(i, *MR::getMarioShadowTex()->getTexInfo());
            if (!hasShadowSetter) {
                addMarioShadowProjmapMtxSetter();
                hasShadowSetter = true;
            }
        }
    }
}

bool DisplayListMaker::isExistDiffMaterial(const J3DModelData* pModelData) {
    if (pModelData->getTextureName()->getIndex("IndDummy") != -1) {
        return true;
    }
    if (pModelData->getTextureName()->getIndex("ShadowDummy") != -1) {
        return true;
    }
    for (u16 i = 0; i < pModelData->getMaterialNum(); i++) {
        if (!MR::isNormalTexMtx(pModelData->getMaterialNodePointer(i))) {
            return true;
        }
    }
    return false;
}
