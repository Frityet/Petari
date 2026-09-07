#include "Game/Util/FurCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/FurMulti.hpp"
#include "Game/Util/FurParam.hpp"
#include "Game/Util/FurDrawer.hpp"
#include "Game/Util/FurShader.hpp"
#include "Game/Util.hpp"
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3DGraphBase/J3DShape.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>

FurMulti* FurBank::check(J3DModelData* pModelData, u32 layer) {
    for (u32 i = 0; i < mCount; i++) {
        if (mFurMulti[i]->mModel->getModelData() == pModelData && (mLayerMask[i] & (1 << layer))) {
            return mFurMulti[i];
        }
    }
    return nullptr;
}

void FurDrawManager::add(FurCtrl* pCtrl, u8 idx) {
    if (mCapacity != mNumFurCtrls[idx]) {
        mFurCtrls[idx][mNumFurCtrls[idx]] = pCtrl;
        mNumFurCtrls[idx]++;
    }
}

void FurDrawManager::draw() const {
    for (u32 i = 0; i < mNumFurCtrls[1]; i++) {
        if (!MR::isClipped(mFurCtrls[1][i]->_0) && !MR::isDead(mFurCtrls[1][i]->_0) && !MR::isHiddenModel(mFurCtrls[1][i]->_0)) {
            if (mFurCtrls[1][i]->_1C == 1) {
                mFurCtrls[1][i]->drawFur();
            }
        }
    }

    GXSetClipMode(GX_CLIP_ENABLE);
}

FurCtrl::FurCtrl(LiveActor* pActor, FurParam* pParam, bool add, u8 idx) : _18(pParam), _1C(0), _20(1.0f) {
    if (add) {
        MR::getSceneObj< FurDrawManager >(SceneObj_FurDrawManager)->add(this, idx);
    }
    _0 = pActor;
    _10 = nullptr;
    _14 = 0xFFFF;
    _24 = 0;
    _28 = nullptr;
    _2C = nullptr;
    _30 = nullptr;
    _34 = nullptr;
    _38 = nullptr;
    _3C = nullptr;
    _C = 0;
    mDynamicParam.mFogCtrl = nullptr;
    mDynamicParam.mLightParam = nullptr;
}

namespace MR {
    FurDrawManager* getFurDrawManager() {
        return getSceneObj< FurDrawManager >(SceneObj_FurDrawManager);
    }
}

FurDrawManager::FurDrawManager(u8 capacity) : NameObj("ファー描画マネージャ") {
    mNumFurCtrls[0] = 0;
    mNumFurCtrls[1] = 0;
    mFurCtrls[0] = new FurCtrl*[capacity];
    mFurCtrls[1] = new FurCtrl*[capacity];
    mCapacity = capacity;
    mBank = new FurBank;
    MR::connectToScene(this, -1, -1, -1, 49);
}

FurDrawManager::~FurDrawManager() {
}

void FurCtrl::calcLayerForm() {
    _28->mFurScale = _18->_38;
    _28->mBaseScale = _18->_3C;
    _28->mLength.mEnd = _18->_4;
    _28->mLength.mStart = 0.0f;
    _28->mLength.mExponent = _18->_8;
    _28->mLayerCount = _18->mLayerCount;
    if (static_cast< s32 >(_28->mLayerCount) > _24) {
        _28->mLayerCount = _24;
    }
    for (s32 i = 0; i < static_cast< s32 >(_28->mLayerCount); i++) {
        f32 length = _28->mLength.calcValue(i, _28->mLayerCount);
        _2C->_8 = pow(1.0f * i / static_cast< s32 >(_28->mLayerCount), 4.0);
        _30[i]->getVertexBuffer()->frameInit();
        _2C->_1C = length;
        _2C->calc(_30[i]);
    }
}

void FurCtrl::createFurMap() {
    if (_C != 0) {
        return;
    }
    for (u32 i = 0; i < 4; i++) {
        _28->mDensity[i] = _18->_44[i];
        _28->mIntensity[i] = _18->_54[i];
        _28->mTransparency[i] = reinterpret_cast< u8* >(&_18->_64)[i];
    }
    _28->createFurMap();
}

J3DModel2::J3DModel2(J3DModel* pModel) : J3DModel() {
    mModelData = pModel->mModelData;
    mMtxBuffer = pModel->mMtxBuffer;
    mShapePacket = pModel->mShapePacket;
    mMatPacket = pModel->mMatPacket;
    mVertexBuffer.setVertexData(&mModelData->mVertexData);
    mFlags |= J3DMdlFlag_UseDefaultJ3D;
}

J3DModel2::~J3DModel2() {
}

void FurCtrl::setupFur(J3DModel* pModel, ResTIMG* pLength, ResTIMG* pIndirect, ResTIMG* pBody, u16 shapeIndex, u8 count) {
    J3DModelData* modelData = pModel->getModelData();
    _10 = pModel;
    _34 = pLength;
    _38 = pBody;
    _3C = pIndirect;
    _14 = shapeIndex;
    _2C = new CShader(modelData, pLength);
    _2C->makeIndexData(modelData->getShapeNodePointer(shapeIndex));
    _2C->checkBorderVtx(modelData, shapeIndex);
    _28 = new FurDrawer(count, pBody, pIndirect);
    u32 componentSize;
    switch (_2C->_24) {
    case GX_S16:
        componentSize = 2;
        break;
    case GX_F32:
        componentSize = 4;
        break;
    }
    _24 = count;
    _30 = new J3DModel*[count];
    for (s32 i = 0; i < _24; i++) {
        char name[32];
        sprintf(name, "レイヤ %d", i);
        J3DModel2* model = new J3DModel2(_10);
        model->mUnkCalc1 = _2C;
        if (model->mUnkCalc1 != nullptr) {
            model->mUnkCalc1->setup(model->getModelData());
            J3DVertexBuffer* buffer = model->getVertexBuffer();
            if (buffer->mTransformedVtxPosArray[0] == nullptr || buffer->mTransformedVtxPosArray[1] == nullptr) {
                void* positions = new (32) u8[(componentSize * 3 * buffer->mVtxData->mVtxNum + 31) & ~31];
                buffer->mTransformedVtxPosArray[0] = positions;
                if (positions != nullptr) {
                    buffer->mTransformedVtxPosArray[1] = positions;
                }
            }
            for (s32 j = 0; j < 2; j++) {
                memcpy(buffer->mTransformedVtxPosArray[j], buffer->mVtxData->mVtxPosArray, componentSize * buffer->mVtxData->mVtxNum * 3);
                DCStoreRange(buffer->mTransformedVtxPosArray[j], componentSize * 3 * buffer->mVtxData->mVtxNum);
            }
        }
        _30[i] = model;
    }
    _1C = 1;
}

void FurCtrl::setupFurClone(J3DModel* pModel, FurCtrl* pOriginal) {
    _10 = pModel;
    _C = 1;
    _34 = pOriginal->_34;
    _38 = pOriginal->_38;
    _3C = pOriginal->_3C;
    _14 = pOriginal->_14;
    _2C = nullptr;
    _28 = pOriginal->_28;
    _24 = pOriginal->_24;
    _30 = new J3DModel*[_24];
    for (u32 i = 0; i < _24; i++) {
        char name[32];
        sprintf(name, "レイヤ(コピー) %d", i);
        J3DModel2* model = new J3DModel2(_10);
        J3DVertexBuffer& destination = model->mVertexBuffer;
        const J3DVertexBuffer& source = pOriginal->_30[i]->mVertexBuffer;
        destination.mVtxData = source.mVtxData;
        for (u32 j = 0; j < 2; j++) {
            destination.mVtxPosArray[j] = source.mVtxPosArray[j];
            destination.mVtxNrmArray[j] = source.mVtxNrmArray[j];
            destination.mVtxColArray[j] = source.mVtxColArray[j];
            destination.mTransformedVtxPosArray[j] = source.mTransformedVtxPosArray[j];
            destination.mTransformedVtxNrmArray[j] = source.mTransformedVtxNrmArray[j];
        }
        destination.mCurrentVtxPos = source.mCurrentVtxPos;
        destination.mCurrentVtxNrm = source.mCurrentVtxNrm;
        destination.mCurrentVtxCol = source.mCurrentVtxCol;
        _30[i] = model;
    }
    _1C = 1;
}

void FurCtrl::drawFur() {
    J3DModelData* modelData = _10->getModelData();
    u16 shapeIndex = _14;
    if (MR::getJ3DModel(_0)->getShapePacket(shapeIndex)->checkFlag(0x10)) {
        return;
    }
    if (modelData->getShapeNodePointer(_14)->checkFlag(1)) {
        return;
    }
    if (mDynamicParam.mLightParam->mLightType == -1) {
        if (MR::getLightCtrl(_0) != nullptr) {
            MR::loadActorLight(_0);
        }
    } else {
        MR::loadLight(mDynamicParam.mLightParam->mLightType);
    }
    _28->mFurScale = _18->_38 * _20;
    _28->mBaseScale = _18->_3C;
    _28->mColor = _18->_40;
    _28->mIndirect.mEnd = _18->_C;
    _28->mIndirect.mStart = 0.0f;
    _28->mIndirect.mExponent = _18->_10;
    _28->mBrightness.mEnd = _18->_14;
    _28->mBrightness.mStart = _18->_18;
    _28->mBrightness.mExponent = _18->_1C;
    _28->mAlpha.mEnd = _18->_20;
    _28->mAlpha.mStart = _18->_24;
    _28->mAlpha.mExponent = _18->_28;
    _28->mOffset.mEnd = _18->_2C;
    _28->mOffset.mStart = _18->_30;
    _28->mOffset.mExponent = _18->_34;
    _28->update();
    _28->setupMaterial(&mDynamicParam);
    if (_28->mMixFog != 0) {
        _28->mMixFog--;
    }
    modelData->getShapeNodePointer(_14)->mCurrentMtx.setCurrentTexMtx(
        GX_TEXMTX0, GX_TEXMTX1, GX_TEXMTX2, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY);
    J3DShape::resetVcdVatCache();
    for (s32 i = 0; i < static_cast< s32 >(_28->mLayerCount); i++) {
        _28->setupLayerMaterial(i);
        J3DModel* model = _30[i];
        J3DShapePacket* packet = model->getShapePacket(_14);
        packet->setModel(model);
        packet->prepareDraw();
        packet->getShape()->draw();
    }
    _10->getShapePacket(_14)->setModel(_10);
    GXColor color = {0, 0, 0, 0};
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, color);
    J3DShape::resetVcdVatCache();
    GXSetClipMode(GX_CLIP_DISABLE);
}

namespace MR {
    void initFurParamFromDvd(FurParam* pParam, DynamicFurParam* pDynamic, char* pData, u32 size) {
        FurLightParam* light = pDynamic->mLightParam;
        s32 light0Enabled = 0;
        s32 light0Material = 0;
        s32 light0Ambient = 0;
        s32 light1Enabled = 0;
        s32 light1Material = 0;
        s32 light1Ambient = 0;
        s32 lightColorSource = 0;
        u32 offset = 0;
        while (offset < size) {
            char line[256];
            u32 length = 0;
            while (true) {
                char character = *pData;
                if (character == '\n' || character == '\r') {
                    break;
                }
                offset++;
                line[length] = character;
                length++;
                pData++;
                if (offset >= size) {
                    break;
                }
            }
            line[length] = '\0';
            while (offset < size) {
                if (*pData != '\n' && *pData != '\r') {
                    break;
                }
                pData++;
                offset++;
            }
            scan32(line, "レイヤ数", &pParam->mLayerCount);
            scanf32(line, "毛長さ", &pParam->_4);
            scanf32(line, "長さ偏差", &pParam->_8);
            scanf32(line, "ズレ(indirect)", &pParam->_C);
            scanf32(line, "ズレ偏差", &pParam->_10);
            scanf32(line, "明るさ(毛先)", &pParam->_14);
            scanf32(line, "明るさ(毛元)", &pParam->_18);
            scanf32(line, "明るさ偏差", &pParam->_1C);
            scanf32(line, "透明度(毛先)", &pParam->_20);
            scanf32(line, "透明度(毛元)", &pParam->_24);
            scanf32(line, "透明度偏差", &pParam->_28);
            scanf32(line, "透明度・地肌(毛先)", &pParam->_2C);
            scanf32(line, "透明度・地肌(毛元)", &pParam->_30);
            scanf32(line, "透明度・地肌偏差", &pParam->_34);
            scanf32(line, "密度マップスケール", &pParam->_38);
            scanf32(line, "ベースマップスケール", &pParam->_3C);
            scanu8x4(line, "混合カラー", &pParam->_40.r);
            scanf32x4(line, "植毛密度", pParam->_44);
            scanf32x4(line, "植毛太さ", pParam->_54);
            scanu8x4(line, "混合比", &pParam->_64.r);
            scan32(line, "ライト0スイッチ", &light0Enabled);
            scan32(line, "ライト0マテリアル", &light0Material);
            scan32(line, "ライト0アンビエント", &light0Ambient);
            scan32(line, "ライト1スイッチ", &light1Enabled);
            scan32(line, "ライト1マテリアル", &light1Material);
            scan32(line, "ライト1アンビエント", &light1Ambient);
            scan32(line, "ライトカラーソース", &lightColorSource);
            light->mLight0Enabled = light0Enabled;
            light->mLight0Material = light0Material;
            light->mLight0Ambient = light0Ambient;
            light->mLight1Enabled = light1Enabled;
            light->mLight1Material = light1Material;
            light->mLight1Ambient = light1Ambient;
            light->mLightColorSource = lightColorSource;
        }
    }
}
