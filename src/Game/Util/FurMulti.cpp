#include "Game/Util/FurMulti.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/FurCtrl.hpp"
#include "Game/Util/FurDrawer.hpp"
#include "Game/Util/FurParam.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JKernel/JKRSolidHeap.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <cstring>

namespace {
    FurParam sFurParam = {
        6, 10.0f, 0.778809011f, 0.0f, 1.0f, 0.0f, 0.0f, 0.6f,
        255.0f, 0.0f, 1.0f, 80.0f, 0.0f, 5.419921875f, 16.2109375f, 1.0f,
        {54, 48, 44, 0},
        {0.8f, 0.6f, 0.3f, 0.09f},
        {0.517f, 0.386f, 0.3645f, 0.1713f},
        {26, 64, 98, 212},
    };
}

FurMulti::FurMulti(LiveActor* actor, u32 count) {
    mActor = actor;
    mModel = MR::getJ3DModel(actor);
    mLayerCount = count;
    _4 = new u8[count];
    _8 = new u8[count];
    mFurCtrls = new FurCtrl*[count];
    for (u32 i = 0; i < count; i++) {
        _8[i] = 0xFF;
    }
    _0 = 0;
    _1 = 1;
}

void FurMulti::setLayerDirect(u32 layer, u32 drawType, u32 shapeIndex, u32 count, FurParam* pParam,
                             ResTIMG* pBody, ResTIMG* pLength, ResTIMG* pIndirect, ResTIMG* pDensity) {
    _4[layer] = shapeIndex;
    _8[layer] = drawType;
    mFurCtrls[layer] = new FurCtrl(mActor, pParam, false, 1);
    J3DModelData* modelData = mModel->getModelData();
    FurMulti* original = MR::getFurDrawManager()->mBank->check(modelData, layer);
    if (original != nullptr) {
        mFurCtrls[layer]->setupFurClone(mModel, original->mFurCtrls[layer]);
    } else {
        mFurCtrls[layer]->setupFur(mModel, pLength, pIndirect, pBody, shapeIndex, count);
        MR::getFurDrawManager()->mBank->regist(this, layer);
        mFurCtrls[layer]->calcLayerForm();
        if (pDensity != nullptr) {
            pDensity->mFormat = GX_TF_IA8;
            pDensity->mWidth = 32;
            pDensity->mHeight = 32;
            pDensity->mWrapS = GX_REPEAT;
            pDensity->mWrapT = GX_REPEAT;
            pDensity->mPaletteName = 0;
            pDensity->mPaletteFormat = 0;
            pDensity->mPaletteNum = 0;
            pDensity->mPaletteDataOffset = 0;
            pDensity->mMipmap = false;
            pDensity->mDoEdgeLod = false;
            pDensity->mBiasClamp = false;
            pDensity->mMaxAnisotropy = 0;
            pDensity->mMinType = GX_LINEAR;
            pDensity->mMagType = GX_LINEAR;
            pDensity->mMinLod = 0;
            pDensity->mMaxLod = 0;
            pDensity->mImageNum = 1;
            pDensity->mLodBias = 0;
            pDensity->mImageDataOffset = 32;
            JUTTexture* texture = new JUTTexture(pDensity, 0);
            FurDrawer* drawer = mFurCtrls[layer]->_28;
            drawer->mFurTexture = texture;
            drawer->_F8 = 1;
        } else {
            mFurCtrls[layer]->createFurMap();
        }
        mFurCtrls[layer]->_28->update();
    }
}

void FurMulti::offDraw(u32 mask) {
    for (u32 i = 0; i < mLayerCount; i++) {
        if (mask & (1 << i)) {
            mFurCtrls[i]->_1C = 0;
        }
    }
}

void FurMulti::onDraw(u32 mask) {
    for (u32 i = 0; i < mLayerCount; i++) {
        if (mask & (1 << i)) {
            mFurCtrls[i]->_1C = 1;
        }
    }
}

void FurMulti::addToManager() {
    for (u32 i = 0; i < mLayerCount; i++) {
        MR::getFurDrawManager()->add(mFurCtrls[i], 1);
    }
    _0 = 1;
}

void FurBank::regist(FurMulti* pFur, u32 layer) {
    mFurMulti[mCount] = pFur;
    mLayerMask[mCount] |= 1 << layer;
    mCount++;
}

namespace MR {
    FurMulti* initMultiFur(LiveActor* pActor, s32 lightType) {
        CurrentHeapRestorer heapRestorer(getSceneHeapGDDR3());
        J3DModelData* modelData = getJ3DModelData(pActor);
        u16 furCount = 0;
        for (u16 i = 0; i < modelData->getMaterialNum(); i++) {
            if (strstr(modelData->getMaterialName()->getName(i), "Fur") != nullptr) {
                furCount++;
            }
        }
        if (furCount == 0) {
            return nullptr;
        }

        FurMulti* fur = new FurMulti(pActor, furCount);
        DynamicFurParam dynamicParam;
        dynamicParam.mFogCtrl = new FogCtrl(modelData, true);
        dynamicParam.mLightParam = new FurLightParam;
        dynamicParam.mLightParam->mLightType = lightType;

        u32 layer = 0;
        for (u16 i = 0; i < modelData->getMaterialNum(); i++) {
            if (strstr(modelData->getMaterialName()->getName(i), "Fur") == nullptr) {
                continue;
            }

            u16 shapeIndex = modelData->getMaterialNodePointer(i)->getShape()->getIndex();
            char resourceName[256];
            strcpy(resourceName, modelData->getMaterialName()->getName(i));
            strcat(resourceName, "Body");
            ResTIMG* body;
            if (isExistTexture(pActor, resourceName)) {
                body = getTexFromModel(resourceName, pActor);
            } else {
                body = getResTIMG(pActor, 0);
            }

            strcpy(resourceName, modelData->getMaterialName()->getName(i));
            strcat(resourceName, "Length.bti");
            ResTIMG* length = nullptr;
            if (getResourceHolder(pActor)->mFileInfoTable->isExistRes(resourceName)) {
                length = static_cast< ResTIMG* >(getResourceHolder(pActor)->mFileInfoTable->getRes(resourceName));
            }

            strcpy(resourceName, modelData->getMaterialName()->getName(i));
            strcat(resourceName, "Indirect.bti");
            ResTIMG* indirect = nullptr;
            if (getResourceHolder(pActor)->mFileInfoTable->isExistRes(resourceName)) {
                indirect = static_cast< ResTIMG* >(getResourceHolder(pActor)->mFileInfoTable->getRes(resourceName));
            }

            strcpy(resourceName, modelData->getMaterialName()->getName(i));
            strcat(resourceName, "Density.bti");
            ResTIMG* density = nullptr;
            if (getResourceHolder(pActor)->mFileInfoTable->isExistRes(resourceName)) {
                density = static_cast< ResTIMG* >(getResourceHolder(pActor)->mFileInfoTable->getRes(resourceName));
            }

            FurParam* param = new FurParam;
            *param = sFurParam;
            strcpy(resourceName, modelData->getMaterialName()->getName(i));
            strcat(resourceName, ".fur.txt");
            if (getResourceHolder(pActor)->mFileInfoTable->isExistRes(resourceName)) {
                char* data = static_cast< char* >(getResourceHolder(pActor)->mFileInfoTable->getRes(resourceName));
                u32 size = getResourceHolder(pActor)->mFileInfoTable->findFileInfo(resourceName)->_4;
                initFurParamFromDvd(param, &dynamicParam, data, size);
            }

            fur->setLayerDirect(layer, 0, shapeIndex, param->mLayerCount, param, body, length, indirect, density);
            fur->mFurCtrls[layer]->mDynamicParam.mLightParam = dynamicParam.mLightParam;
            fur->mFurCtrls[layer]->mDynamicParam.mFogCtrl = dynamicParam.mFogCtrl;
            layer++;
        }
        fur->addToManager();
        return fur;
    }
}
