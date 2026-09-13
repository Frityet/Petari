#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/Util.hpp"
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>

MaterialCtrl::MaterialCtrl(J3DModelData* pModelData, const char* pMaterialName) {
    mModelData = pModelData;
    mMaterial = nullptr;

    if (pMaterialName) {
        mMaterial = MR::getMaterial(pModelData, pMaterialName);
    }
}

void MaterialCtrl::update() {
    if (mMaterial) {
        updateMaterial(mMaterial);
    } else {
        u16 i = 0;
        while (i < mModelData->mMaterialTable.getMaterialNum()) {
            updateMaterial(mModelData->mMaterialTable.getMaterialNodePointer(i));
            i++;
        }
    }
}

/*
FogCtrl::FogCtrl(J3DModelData* pModelData, bool a3) : MaterialCtrl(pModelData, nullptr) {
    mNumMaterials = 0;
    mMaterials = nullptr;

    J3DMaterial* nextMat = nullptr;
    for (u16 i = 0; i < pModelData->getMaterialCount(); i++) {
        J3DMaterial* mat = pModelData->getMaterial(i);
        if (a3 || mat->mPEBlock->getFog()->mType) {
            if (nextMat == nullptr) {
                nextMat = mat;
            }

            mNumMaterials++;
        }
    }

    if (nextMat == nullptr) {
        nextMat = *pModelData->mMaterialTable.mMaterials;
    }

    mFogInfo = nextMat->mPEBlock->getFog();

    if (mNumMaterials > 0) {
        mMaterials = new J3DMaterial*[mNumMaterials];
        s32 curMaterial = 0;

        for (u16 i = 0; i < pModelData->getMaterialCount(); i++) {
            J3DMaterial* mat = pModelData->getMaterial(i);
            if (a3 || mat->mPEBlock->getFog()->mType) {
                mMaterials[curMaterial++] = mat;
            }
        }
    }
}
*/

/*
void FogCtrl::update() {
    for (s32 i = 0; i < mNumMaterials; i++) {
        J3DPEBlockFull& block = *(J3DPEBlockFull*)mMaterials[i]->mPEBlock;
        mMaterials[i]->mPEBlock->getFog() = block.mFog;
    }
}
    */

MatColorCtrl::MatColorCtrl(J3DModelData* pModelData, const char* pName, u32 color, const J3DGXColor* pColor) : MaterialCtrl(pModelData, pName) {
    mColorChoice = color;
    mColor = pColor;
}

void MatColorCtrl::updateMaterial(J3DMaterial* pMaterial) {
    pMaterial->mColorBlock->setMatColor(mColorChoice, mColor);
}

MirrorReflectionMtxSetter::MirrorReflectionMtxSetter(J3DModel* pModel, const ResourceHolder* pResourceHolder)
    : MaterialCtrl(nullptr, nullptr), mNumMatrices(0) {
    for (u16 i = 0; i < 8; i++) {
        mMatrices[i] = nullptr;
    }
    addUpdatingTexMtxFromName(pModel->mModelData);
}

void MirrorReflectionMtxSetter::addUpdatingTexMtxFromTexNo(J3DModelData* pModelData, u16 textureNo) {
    u16 materialCount = pModelData->getMaterialNum();
    for (u16 i = 0; i < materialCount; i++) {
        if (MR::isUseTex(pModelData->getMaterialNodePointer(i), textureNo)) {
            addUpdatingTexMtxFromTexCoord(pModelData->getMaterialNodePointer(i));
        }
    }
}

void MirrorReflectionMtxSetter::addUpdatingTexMtxFromTexCoord(J3DMaterial* pMaterial) {
    for (u32 i = 0; i < 8; i++) {
        J3DTexMtx* pTexMtx = pMaterial->mTexGenBlock->getTexMtx(i);
        if (pTexMtx != nullptr && (pTexMtx->getTexMtxInfo().mInfo & 0x3F) == J3DTexMtxMode_Projmap && MR::isUseTexMtx(pMaterial, i)) {
            addUpdatingTexMtx(pTexMtx);
        }
    }
}

void MirrorReflectionMtxSetter::addUpdatingTexMtx(J3DTexMtx* pTexMtx) {
    mMatrices[mNumMatrices] = pTexMtx;
    mNumMatrices++;
}

void MirrorReflectionMtxSetter::update() {
    for (s32 i = 0; i < mNumMatrices; i++) {
        mMatrices[i]->getTexMtxInfo().setEffectMtx(const_cast< TPos3f& >(MR::getMirrorModelTexMtx()));
    }
}

void MirrorReflectionMtxSetter::addUpdatingTexMtxFromName(J3DModelData* pModelData) {
    u16 textureCount = pModelData->getTexture()->getNum();
    for (u16 i = 0; i < textureCount; i++) {
        if (MR::isEqualString(pModelData->getTextureName()->getName(i), "MirrorTex")) {
            addUpdatingTexMtxFromTexNo(pModelData, i);
        }
    }
}
