#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util.hpp"
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3DGraphAnimator/J3DModelData.hpp>
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>

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

ProjmapEffectMtxSetter::ProjmapEffectMtxSetter(J3DModel* pModel, const ResourceHolder* pResourceHolder)
    : MaterialCtrl(nullptr, nullptr), mUpdateEffectMtxInfo(nullptr), mNumUpdateEffectMtxInfo(0), mModel(pModel) {
    mBaseMtx.identity();

    J3DModelData* modelData = pModel->mModelData;
    for (u16 i = 0; i < modelData->mMaterialTable.getMaterialNum(); i++) {
        J3DMaterial* material = modelData->mMaterialTable.getMaterialNodePointer(i);
        for (u32 j = 0; j < 8; j++) {
            J3DTexMtx* texMtx = material->mTexGenBlock->getTexMtx(j);
            if (texMtx && (texMtx->getTexMtxInfo().mInfo & 0x3F) == J3DTexMtxMode_Projmap && MR::isUseTexMtx(material, j)) {
                mNumUpdateEffectMtxInfo++;
            }
        }
    }

    mUpdateEffectMtxInfo = new UpdateEffectMtxInfo[mNumUpdateEffectMtxInfo];

    s32 updateIndex = 0;
    for (u16 i = 0; i < modelData->mMaterialTable.getMaterialNum(); i++) {
        J3DMaterial* material = modelData->mMaterialTable.getMaterialNodePointer(i);
        for (u32 j = 0; j < 8; j++) {
            J3DTexMtx* texMtx = material->mTexGenBlock->getTexMtx(j);
            if (texMtx && (texMtx->getTexMtxInfo().mInfo & 0x3F) == J3DTexMtxMode_Projmap && MR::isUseTexMtx(material, j)) {
                UpdateEffectMtxInfo* updateInfo = &mUpdateEffectMtxInfo[updateIndex];
                updateInfo->mTexMtxInfo = &texMtx->getTexMtxInfo();
                updateInfo->mEffectMtx.set(pResourceHolder->getInitEffectMtx(i, j));
                updateIndex++;
            }
        }
    }
}

void ProjmapEffectMtxSetter::update() {
    for (s32 i = 0; i < mNumUpdateEffectMtxInfo; i++) {
        TPos3f effectMtx;
        effectMtx.concat(mUpdateEffectMtxInfo[i].mEffectMtx, mBaseMtx);
        mUpdateEffectMtxInfo[i].mTexMtxInfo->setEffectMtx(effectMtx);
    }
}

void ProjmapEffectMtxSetter::getBaseTrans(TVec3f* pBaseTrans) const {
    pBaseTrans->set< f32 >(mModel->mBaseTransformMtx[0][3], mModel->mBaseTransformMtx[1][3], mModel->mBaseTransformMtx[2][3]);
}

void ProjmapEffectMtxSetter::updateMtxUseBaseMtx() {
    TPos3f baseMtx;
    baseMtx.set(mModel->mBaseTransformMtx);
    mBaseMtx.invert(baseMtx);
}

void ProjmapEffectMtxSetter::updateMtxUseBaseMtxWithLocalOffset(const TVec3f& rLocalOffset) {
    TPos3f baseMtx(mModel->mBaseTransformMtx);

    TPos3f localOffsetMtx;
    localOffsetMtx.makeTrans(rLocalOffset.x, rLocalOffset.y, rLocalOffset.z);
    baseMtx.concat(localOffsetMtx);
    mBaseMtx.invert(baseMtx);
}

ProjmapEffectMtxSetter::UpdateEffectMtxInfo::UpdateEffectMtxInfo() {
}
