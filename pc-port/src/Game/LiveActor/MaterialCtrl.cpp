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

FogCtrl::FogCtrl(J3DModelData* pModelData, bool a3) : MaterialCtrl(pModelData, nullptr) {
    mNumMaterials = 0;
    mMaterials = nullptr;

    J3DMaterial* nextMat = nullptr;
    for (u16 i = 0; i < pModelData->getMaterialNum(); i++) {
        J3DMaterial* mat = pModelData->getMaterialNodePointer(i);
        if (a3 || mat->mPEBlock->getFog()->mType) {
            if (nextMat == nullptr) {
                nextMat = mat;
            }

            mNumMaterials++;
        }
    }

    if (nextMat == nullptr) {
        nextMat = pModelData->getMaterialNodePointer(0);
    }

    mFogInfo = *nextMat->mPEBlock->getFog();

    if (mNumMaterials > 0) {
        mMaterials = new J3DMaterial*[mNumMaterials];
        s32 curMaterial = 0;

        for (u16 i = 0; i < pModelData->getMaterialNum(); i++) {
            J3DMaterial* mat = pModelData->getMaterialNodePointer(i);
            if (a3 || mat->mPEBlock->getFog()->mType) {
                mMaterials[curMaterial++] = mat;
            }
        }
    }
}
void FogCtrl::update() {
    for (s32 i = 0; i < mNumMaterials; i++) {
        mMaterials[i]->mPEBlock->getFog()->setFogInfo(mFogInfo);
    }
}

MatColorCtrl::MatColorCtrl(J3DModelData* pModelData, const char* pName, u32 color, const J3DGXColor* pColor) : MaterialCtrl(pModelData, pName) {
    mColorChoice = color;
    mColor = pColor;
}

void MatColorCtrl::updateMaterial(J3DMaterial* pMaterial) {
    pMaterial->mColorBlock->setMatColor(mColorChoice, mColor);
}

ViewProjmapEffectMtxSetter::ViewProjmapEffectMtxSetter(J3DModelData* pModelData)
    : MaterialCtrl(nullptr, nullptr), mMatricies(nullptr), mNumMatricies(0) {
    J3DTexMtxInfo* matrices[64];
    for (u16 i = 0; i < pModelData->getMaterialNum(); i++) {
        J3DMaterial* material = pModelData->getMaterialNodePointer(i);
        for (u32 j = 0; j < 8; j++) {
            J3DTexMtx* texMtx = material->mTexGenBlock->getTexMtx(j);
            if (texMtx != nullptr && (texMtx->getTexMtxInfo().mInfo & 0x3F) == static_cast< u32 >(J3DTexMtxMode_ViewProjmap) &&
                MR::isUseTexMtx(material, j)) {
                matrices[mNumMatricies] = &texMtx->getTexMtxInfo();
                mNumMatricies++;
            }
        }
    }
    mMatricies = new J3DTexMtxInfo*[mNumMatricies];
    MR::copyMemory(mMatricies, matrices, mNumMatricies * sizeof(J3DTexMtxInfo*));
}

void ViewProjmapEffectMtxSetter::update() {
    TProj3f projection;
    projection = MR::getCameraProjectionMtx();
    projection.mMtx[2][0] = 0.0f;
    projection.mMtx[2][1] = 0.0f;
    projection.mMtx[2][2] = -1.0f;
    projection.mMtx[2][3] = 0.0f;
    projection.mMtx[3][0] = 0.0f;
    projection.mMtx[3][1] = 0.0f;
    projection.mMtx[3][2] = 0.0f;
    projection.mMtx[3][3] = 1.0f;
    for (s32 i = 0; i < mNumMatricies; i++) {
        mMatricies[i]->setEffectMtx(projection.mMtx);
    }
}

ProjmapEffectMtxSetter::ProjmapEffectMtxSetter(J3DModel* pModel, const ResourceHolder* pResourceHolder)
    : MaterialCtrl(nullptr, nullptr), mUpdateEffectMtxInfo(nullptr), mNumUpdateEffectMtxInfo(0), mModel(pModel) {
    J3DModelData* modelData = pModel->mModelData;
    mBaseMtx.identity();
    for (u16 i = 0; i < modelData->mMaterialTable.getMaterialNum(); i++) {
        J3DMaterial* material = modelData->mMaterialTable.getMaterialNodePointer(i);
        for (u32 j = 0; j < 8; j++) {
            J3DTexMtx* texMtx = material->mTexGenBlock->getTexMtx(j);
            if (texMtx && (texMtx->getTexMtxInfo().mInfo & 0x3F) == static_cast< u32 >(J3DTexMtxMode_Projmap) &&
                MR::isUseTexMtx(material, j)) {
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
            if (texMtx && (texMtx->getTexMtxInfo().mInfo & 0x3F) == static_cast< u32 >(J3DTexMtxMode_Projmap) &&
                MR::isUseTexMtx(material, j)) {
                mUpdateEffectMtxInfo[updateIndex].mTexMtxInfo = &texMtx->getTexMtxInfo();
                mUpdateEffectMtxInfo[updateIndex].mEffectMtx.set(pResourceHolder->getInitEffectMtx(i, j));
                updateIndex++;
            }
        }
    }
}

void ProjmapEffectMtxSetter::update() {
    for (s32 i = 0; i < mNumUpdateEffectMtxInfo; i++) {
        UpdateEffectMtxInfo* info = &mUpdateEffectMtxInfo[i];
        TPos3f effectMtx;
        effectMtx.concat(info->mEffectMtx, mBaseMtx);
        info->mTexMtxInfo->setEffectMtx(effectMtx);
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

MarioShadowProjmapMtxSetter::MarioShadowProjmapMtxSetter(J3DModel* pModel, const ResourceHolder* pResourceHolder)
    : MaterialCtrl(nullptr, nullptr), mProjmapEffectMtxSetter(nullptr) {
    mProjmapEffectMtxSetter = new ProjmapEffectMtxSetter(pModel, pResourceHolder);
}

void MarioShadowProjmapMtxSetter::update() {
    TVec3f playerPos(*MR::getPlayerPos());
    TVec3f baseTrans;
    mProjmapEffectMtxSetter->getBaseTrans(&baseTrans);
    TVec3f offset(baseTrans);
    offset.sub(playerPos);
    TVec3f shadowVec(MR::getMarioShadowVec());
    f32 distance = -1.0f;
    if (MR::isNearZero(shadowVec.length() - 1.0f)) {
        distance = MR::vecKillElement(offset, shadowVec, &offset);
    }

    Mtx projection;
    Mtx translation;
    if (distance > 0.0f) {
        PSMTXTrans(translation, -playerPos.x, -playerPos.y, -playerPos.z);
    } else {
        PSMTXTrans(translation, 1000000.0f, 1000000.0f, 1000000.0f);
    }

    Mtx rotation;
    MR::makeMtxRotate(rotation, -*MR::getPlayerShadowRotate());
    PSMTXConcat(rotation, translation, projection);
    mProjmapEffectMtxSetter->mBaseMtx.set(projection);
    mProjmapEffectMtxSetter->update();
}

TexMtxCtrl::TexMtxCtrl(J3DModelData* pModelData, const char* pMaterialName) : MaterialCtrl(pModelData, pMaterialName) {
    for (u32 i = 0; i < 8; i++) {
        mMatricies[i] = nullptr;
    }
}

void TexMtxCtrl::setTexMtx(u32 index, J3DTexMtx* pTexMtx) {
    mMatricies[index] = pTexMtx;
}

void TexMtxCtrl::updateMaterial(J3DMaterial* pMaterial) {
    for (u32 i = 0; i < 8; i++) {
        if (mMatricies[i] != nullptr) {
            pMaterial->mTexGenBlock->setTexMtx(i, mMatricies[i]);
        }
    }
}

void MaterialCtrl::updateMaterial(J3DMaterial*) {
}

ProjmapEffectMtxSetter::UpdateEffectMtxInfo::UpdateEffectMtxInfo() {
}
