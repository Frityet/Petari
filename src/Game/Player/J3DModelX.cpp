#include "Game/Player/J3DModelX.hpp"
#include "Game/System/ShapePacketUserData.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/SchedulerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include <JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp>
#include <JSystem/J3DGraphAnimator/J3DModelData.hpp>
#include <JSystem/J3DGraphBase/J3DFifo.hpp>
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphBase/J3DShapeDraw.hpp>
#include <JSystem/J3DGraphBase/J3DShapeMtx.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JUtility/JUTVideo.hpp>
#include <revolution/gd/GDBase.h>
#include <revolution/gx.h>

#include <revolution/gd/GDGeometry.h>
#include <revolution/gd/GDTev.h>
#include <revolution/gd/GDPixel.h>
#include <revolution/gd/GDIndirect.h>
#include <revolution/gd/GDLight.h>
#include <revolution/gd/GDTexture.h>

class J3DMtxBuffer2 : public J3DMtxBuffer {
public:
    void calcNrmMtx2();
    void calcDrawMtx2(u32, const Vec&, const Mtx&, J3DMtxBuffer*);
    void calcDrawMtx3(u32, const Vec&, const Mtx&, J3DMtxBuffer*, const TVec3f&, const TVec3f&);
    void rotationMtx(MtxPtr);
};

class J3DShapeX : public J3DShape {
    friend class J3DModelX;
};

class J3DShapePacketX : public J3DShapePacket {
};

void J3DModelX::viewCalc2() {
    mMtxBuffer->swapDrawMtx();
    mMtxBuffer->swapNrmMtx();
    mMtxBuffer->calcDrawMtx(getMtxCalcMode(), mBaseScale, mBaseTransformMtx);
    static_cast< J3DMtxBuffer2* >(mMtxBuffer)->calcNrmMtx2();
    calcBBoardMtx();
    calcBumpMtx();
    DCStoreRangeNoSync(getDrawMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx));
    prepareShapePackets();
}

void J3DModelX::setDrawView(u32 view) {
    static_cast< J3DMtxBuffer2* >(mMtxBuffer)->rotationMtx(*mExtraMtxBuffer[_DC + view]);
}

void J3DModelX::setDrawViewBuffer(MtxPtr mtx) {
    mMtxBuffer->getDrawMtxPtrPtr()[mMtxBuffer->mCurrentViewNo] = reinterpret_cast< Mtx* >(mtx);
}

void J3DModelX::copyAnmMtxBuffer(const J3DModelX* model) {
    mMtxBuffer->mpAnmMtx = model->mMtxBuffer->mpAnmMtx;
}

void J3DModelX::viewCalc3(u32 view, MtxPtr mtx) {
    J3DMtxBuffer2* buffer = static_cast< J3DMtxBuffer2* >(mMtxBuffer);
    buffer->rotationMtx(*mExtraMtxBuffer[_DC + view]);

    if (mtx != nullptr) {
        buffer->calcDrawMtx(getMtxCalcMode(), mBaseScale, *reinterpret_cast< Mtx* >(&mtx));
    } else {
        buffer->calcDrawMtx(getMtxCalcMode(), mBaseScale, mBaseTransformMtx);
    }

    if ((view & 1) == 0) {
        buffer->swapNrmMtx();
        buffer->calcNrmMtx2();
    }

    calcBBoardMtx();
    calcBumpMtx();
    DCStoreRangeNoSync(getDrawMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx));
    prepareShapePackets();
}

void J3DModelX::viewCalcRef(u32 view, J3DModel* reference) {
    J3DMtxBuffer2* buffer = static_cast< J3DMtxBuffer2* >(mMtxBuffer);
    buffer->rotationMtx(*mExtraMtxBuffer[_DC + view]);
    buffer->calcDrawMtx2(getMtxCalcMode(), mBaseScale, mBaseTransformMtx, reference->mMtxBuffer);
    DCStoreRangeNoSync(getDrawMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx));
    prepareShapePackets();
}

void J3DModelX::viewCalcRefPos(u32 view, J3DModel* reference, const TVec3f& position, const TVec3f& up) {
    J3DMtxBuffer2* buffer = static_cast< J3DMtxBuffer2* >(mMtxBuffer);
    buffer->rotationMtx(*mExtraMtxBuffer[_DC + view]);
    buffer->calcDrawMtx3(getMtxCalcMode(), mBaseScale, mBaseTransformMtx, reference->mMtxBuffer, position, up);
    DCStoreRangeNoSync(getDrawMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx));
    prepareShapePackets();
}

void J3DMtxBuffer2::calcNrmMtx2() {
    const u16 drawMtxNum = mJointTree->getDrawMtxNum();

    for (u16 i = 0; i < drawMtxNum; i++) {
        J3DPSCalcInverseTranspose(*getDrawMtx(i), *getNrmMtx(i));
    }

    DCStoreRange(getNrmMtxPtr(), drawMtxNum * sizeof(Mtx33));
}

void J3DMtxBuffer2::calcDrawMtx2(u32, const Vec& scale, const Mtx& baseMtx, J3DMtxBuffer* reference) {
    Mtx viewBaseMtx;
    J3DCalcViewBaseMtx(j3dSys.getViewMtx(), scale, baseMtx, viewBaseMtx);

    const u16 fullWeightNum = mJointTree->getDrawFullWgtMtxNum();
    for (u16 i = 0; i < fullWeightNum; i++) {
        PSMTXConcat(viewBaseMtx, reference->getAnmMtx(mJointTree->getDrawMtxIndex(i)), *getDrawMtx(i));
    }

    if (mJointTree->getDrawMtxNum() > fullWeightNum) {
        J3DPSMtxArrayConcat(viewBaseMtx, reference->getWeightAnmMtx(0), *getDrawMtx(fullWeightNum), mJointTree->getWEvlpMtxNum());
    }
}

void J3DMtxBuffer2::calcDrawMtx3(u32, const Vec& scale, const Mtx& baseMtx, J3DMtxBuffer* reference, const TVec3f& position,
                                 const TVec3f& up) {
    Mtx viewBaseMtx;
    J3DCalcViewBaseMtx(j3dSys.getViewMtx(), scale, baseMtx, viewBaseMtx);

    const u16 fullWeightNum = mJointTree->getDrawFullWgtMtxNum();
    for (u16 i = 0; i < fullWeightNum; i++) {
        const MtxPtr referenceMtx = reference->getAnmMtx(mJointTree->getDrawMtxIndex(i));
        TVec3f referencePosition;
        MR::extractMtxTrans(referenceMtx, &referencePosition);

        TVec3f offset(referencePosition);
        offset -= position;

        TVec3f planeOffset;
        MR::vecKillElement(offset, up, &planeOffset);

        TVec3f adjustedPosition(position);
        adjustedPosition += planeOffset;

        Mtx adjustedMtx;
        PSMTXCopy(referenceMtx, adjustedMtx);
        MR::setMtxTrans(adjustedMtx, 0.0f, 0.0f, 0.0f);

        TVec3f zero(0.0f, 0.0f, 0.0f);
        TPos3f upMtx;
        MR::makeMtxUpNoSupportPos(&upMtx, up, zero);
        PSMTXConcat(upMtx, adjustedMtx, adjustedMtx);

        Mtx scaleMtx;
        PSMTXScale(scaleMtx, 1.0f, 0.1f, 1.0f);
        PSMTXConcat(scaleMtx, adjustedMtx, adjustedMtx);

        PSMTXInverse(upMtx, upMtx);
        PSMTXConcat(upMtx, adjustedMtx, adjustedMtx);
        MR::setMtxTrans(adjustedMtx, adjustedPosition.x, adjustedPosition.y, adjustedPosition.z);
        PSMTXConcat(viewBaseMtx, adjustedMtx, *getDrawMtx(i));
    }
}

void J3DMtxBuffer2::rotationMtx(MtxPtr mtx) {
    mpDrawMtxArr[1][mCurrentViewNo] = reinterpret_cast< Mtx* >(mtx);
}

void J3DModelX::directDraw(J3DModel* reference) {
    j3dSys.setModel(this);

    if (checkFlag(J3DMdlFlag_SkinPosCpu)) {
        j3dSys.onFlag(J3DSysFlag_SkinPosCpu);
    } else {
        j3dSys.offFlag(J3DSysFlag_SkinPosCpu);
    }

    if (checkFlag(J3DMdlFlag_SkinNrmCpu)) {
        j3dSys.onFlag(J3DSysFlag_SkinNrmCpu);
    } else {
        j3dSys.offFlag(J3DSysFlag_SkinNrmCpu);
    }

    mModelData->syncJ3DSysFlags();
    j3dSys.mTexture = mModelData->getTexture();

    bool mixFog = false;
    if (_1D0 != 0) {
        _1D0--;
        mixFog = true;
    }

    for (u16 i = 0; i < mModelData->getMaterialNum(); i++) {
        J3DMaterial* material = mModelData->getMaterialNodePointer(i);
        _1C0 = i;

        if (reference != nullptr) {
            drawIn(material, mixFog, reference->mBaseTransformMtx, reference);
        } else {
            drawIn(material, mixFog, mBaseTransformMtx, nullptr);
        }
    }

    J3DShape::resetVcdVatCache();
}

void J3DModelX::drawIn(J3DMaterial* material, bool mixFog, MtxPtr, J3DModel* reference) {
    if (material == nullptr || material->mShape == nullptr || material->mShape->checkFlag(J3DShpFlag_Visible)) {
        return;
    }

    J3DMatPacket* matPacket = &mMatPacket[material->mIndex];
    J3DShapePacketX* shapePacket;
    if (reference != nullptr) {
        shapePacket = reinterpret_cast< J3DShapePacketX* >(&reference->mShapePacket[material->mShape->mIndex]);
    } else {
        shapePacket = reinterpret_cast< J3DShapePacketX* >(&mShapePacket[material->mShape->mIndex]);
    }

    j3dSys.setMatPacket(matPacket);
    matPacket->getDisplayListObj()->callDL();

    if (mixFog) {
        TVec3f position;
        MR::extractMtxTrans(mBaseTransformMtx, &position);
        TDDraw::mixFogColor(position, _1D4, _1D8);
    }

    shapePacket->mpShape->loadPreDrawSetting();
    if (shapePacket->mpDisplayListObj != nullptr) {
        shapePacket->mpDisplayListObj->callDL();
    }

    ShapePacketUserData* userData = MR::getJ3DShapePacketUserData(shapePacket);
    if (userData != nullptr) {
        userData->callDL();
    }

    for (u32 i = 0; i < 16; i++) {
        if ((*(u32*)&mFlags & (1 << i)) != 0) {
            GXCallDisplayList(mDisplayLists[i], mDisplayListSizes[i]);
        }
    }

    if (_1B8 != nullptr) {
        _1B4 = _1B8;
        _1B8 = nullptr;
    }

    if (_1B4 != 0 && _1BC != 0) {
        GXCallDisplayList(_1B4, _1BC);
    }

    if (_1C8[_1C0] != nullptr) {
        _1C4[_1C0] = _1C8[_1C0];
        _1C8[_1C0] = nullptr;
    }

    if (_1C4[_1C0] != 0 && _1CC[_1C0] != 0) {
        GXCallDisplayList(_1C4[_1C0], _1CC[_1C0]);
    }

    if (mMaterialCallback != nullptr) {
        mMaterialCallback(_128, _1C0);
    }

    J3DShape* originalShape = nullptr;
    if (_12C != 0) {
        J3DModel* shapeModel = _12C;
        originalShape = shapePacket->mpShape;
        J3DShape* replacementShape = shapeModel->mShapePacket[material->mShape->mIndex].mpShape;
        shapePacket->mpShape = replacementShape;
        replacementShape->mDrawMtx = originalShape->mDrawMtx;
        replacementShape->mDrawMtxData = originalShape->mDrawMtxData;
        replacementShape->mCurrentMtx = material->mCurrentMtx;
    }

    shapePacketDrawFast(shapePacket);

    if (_12C != 0) {
        shapePacket->mpShape = originalShape;
    }
}

bool J3DModelX::simpleDrawSetup(J3DMaterial* material) {
    if (material == nullptr || material->mShape == nullptr || material->mShape->checkFlag(J3DShpFlag_Visible)) {
        return false;
    }

    J3DShape::resetVcdVatCache();
    J3DMatPacket* matPacket = &mMatPacket[material->mIndex];
    J3DShapePacket* shapePacket = &mShapePacket[material->mShape->mIndex];
    j3dSys.setMatPacket(matPacket);
    matPacket->getDisplayListObj()->callDL();
    shapePacket->mpShape->loadPreDrawSetting();
    if (shapePacket->mpDisplayListObj != nullptr) {
        shapePacket->mpDisplayListObj->callDL();
    }
    return true;
}

void J3DModelX::simpleDrawShape(J3DMaterial* material) {
    mShapePacket[material->mShape->mIndex].drawFast();
}

void J3DModelX::storeDisplayList(GDLObj* displayList, u32 index) {
    GDPadCurr32();
    const u32 usedSize = displayList->ptr - displayList->start;
    const u32 alignedSize = (usedSize + 31) & ~31;
    mDisplayLists[index] = new (0x20) u8[alignedSize];
    MR::copyMemory(mDisplayLists[index], displayList->start, alignedSize);
    DCStoreRange(mDisplayLists[index], alignedSize);
    mDisplayListSizes[index] = usedSize;
    GDInitGDLObj(displayList, displayList->start, displayList->length);
}

J3DModelX::J3DModelX(J3DModelData* modelData, u32 modelFlags, u32 mtxBufferFlags)
    : J3DModel(modelData, modelFlags, mtxBufferFlags) {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);

    _DC = 0;
    _DD = 0;
    mMaterialCallback = nullptr;
    _128 = 0;
    _12C = 0;
    mShapeCallback = nullptr;
    _1E4 = 0;
    _1E5 = 0;

    u8 displayListBuffer[0x200] ATTRIBUTE_ALIGN(32);
    GDLObj displayList;
    GDInitGDLObj(&displayList, displayListBuffer, sizeof(displayListBuffer));
    __GDCurrentDL = &displayList;

    GDSetCullMode(static_cast< GXCullMode >(1));
    storeDisplayList(&displayList, 0);

    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(1), static_cast< GXCompare >(7), 0);
    GDSetZMode(0, static_cast< GXCompare >(7), 0);
    GDSetGenMode2(0, 1, 1, 0, static_cast< GXCullMode >(2));
    GDSetTevDirect(static_cast< GXTevStageID >(0));
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetBlendModeEtc(static_cast< GXBlendMode >(0), static_cast< GXBlendFactor >(0), static_cast< GXBlendFactor >(5), static_cast< GXLogicOp >(0), 0, 1, 0);
    GDSetDstAlpha(1, 0xFF);
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(1), static_cast< GXTevScale >(3), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 1);

    GDSetCullMode(static_cast< GXCullMode >(1));
    GDSetZMode(1, static_cast< GXCompare >(3), 1);
    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(4), static_cast< GXBlendFactor >(1), static_cast< GXLogicOp >(0), 1, 0, 0);
    GDSetGenMode2(1, 0, 1, 0, static_cast< GXCullMode >(1));
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetChanCtrl(static_cast< GXChannelID >(5), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetTevOrder(static_cast< GXTevStageID >(0), static_cast< GXTexCoordID >(0), static_cast< GXTexMapID >(0), static_cast< GXChannelID >(0xFF), static_cast< GXTexCoordID >(1), static_cast< GXTexMapID >(1), static_cast< GXChannelID >(0xFF));
    GDSetTevColorCalc(static_cast< GXTevStageID >(0), static_cast< GXTevColorArg >(8), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0));
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(1), static_cast< GXTevScale >(3), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 2);

    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(1), static_cast< GXCompare >(7), 0);
    GDSetZMode(1, static_cast< GXCompare >(6), 0);
    GDSetGenMode2(0, 1, 1, 0, static_cast< GXCullMode >(2));
    GDSetTevDirect(static_cast< GXTevStageID >(0));
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetBlendModeEtc(static_cast< GXBlendMode >(0), static_cast< GXBlendFactor >(0), static_cast< GXBlendFactor >(5), static_cast< GXLogicOp >(0), 0, 1, 0);
    GDSetDstAlpha(1, 0x90);
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(1), static_cast< GXTevScale >(3), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 3);

    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(0), static_cast< GXCompare >(7), 0);
    GDSetZMode(1, static_cast< GXCompare >(3), 0);
    GDSetGenMode2(0, 1, 1, 0, static_cast< GXCullMode >(1));
    GDSetTevDirect(static_cast< GXTevStageID >(0));
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(4), static_cast< GXBlendFactor >(5), static_cast< GXLogicOp >(0), 1, 0, 0);
    GXColor yellow = {0xFF, 0xFF, 0x00, 0xFF};
    GDSetTevColor(static_cast< GXTevRegID >(1), yellow);
    GDSetTevColorCalc(static_cast< GXTevStageID >(0), static_cast< GXTevColorArg >(2), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0));
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(1), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 4);

    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(0), static_cast< GXCompare >(7), 0);
    GDSetZMode(1, static_cast< GXCompare >(7), 1);
    GDSetGenMode2(0, 1, 1, 0, static_cast< GXCullMode >(1));
    GDSetBlendModeEtc(static_cast< GXBlendMode >(0), static_cast< GXBlendFactor >(4), static_cast< GXBlendFactor >(1), static_cast< GXLogicOp >(0), 0, 0, 0);
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(1), static_cast< GXColorSrc >(1), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetTevColorCalc(static_cast< GXTevStageID >(0), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0));
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    GDSetTevOrder(static_cast< GXTevStageID >(0), static_cast< GXTexCoordID >(0xFF), static_cast< GXTexMapID >(0xFF), static_cast< GXChannelID >(4), static_cast< GXTexCoordID >(0xFF), static_cast< GXTexMapID >(0xFF), static_cast< GXChannelID >(4));
    storeDisplayList(&displayList, 5);

    GDSetTexLookupMode(static_cast< GXTexMapID >(0), static_cast< GXTexWrapMode >(1), static_cast< GXTexWrapMode >(1), static_cast< GXTexFilter >(1), static_cast< GXTexFilter >(1), 0.0f, 0.0f, 0.0f, 0, 0, static_cast< GXAnisotropy >(0));
    GDSetTexImgAttr(static_cast< GXTexMapID >(0), static_cast< u16 >(MR::getScreenWidth()), JUTVideo::getManager()->getEfbHeight(), static_cast< GXTexFmt >(4));
    const ResTIMG* screenImage = MR::getScreenResTIMG();
    GDSetTexImgPtr(static_cast< GXTexMapID >(0), const_cast< u8* >(reinterpret_cast< const u8* >(screenImage)) + screenImage->mImageDataOffset);
    GDSetTexCoordGen(static_cast< GXTexCoordID >(0), static_cast< GXTexGenType >(1), static_cast< GXTexGenSrc >(1), 1, 0x3C);
    storeDisplayList(&displayList, 6);

    GXColor black = {0, 0, 0, 0};
    GXColor opaqueBlack = {0, 0, 0, 0xFF};
    GDSetChanAmbColor(static_cast< GXChannelID >(2), black);
    GDSetChanMatColor(static_cast< GXChannelID >(2), opaqueBlack);
    GDSetBlendModeEtc(static_cast< GXBlendMode >(0), static_cast< GXBlendFactor >(1), static_cast< GXBlendFactor >(0), static_cast< GXLogicOp >(5), 0, 1, 0);
    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(0), static_cast< GXCompare >(7), 0);
    GDSetChanCtrl(static_cast< GXChannelID >(0), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetChanCtrl(static_cast< GXChannelID >(2), 1, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 1, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(6), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(5), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 7);

    GDSetCullMode(static_cast< GXCullMode >(1));
    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(0), static_cast< GXBlendFactor >(0), static_cast< GXLogicOp >(5), 0, 1, 0);
    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(1), static_cast< GXCompare >(7), 0);
    GDSetZMode(1, static_cast< GXCompare >(3), 0);
    GXColor tevColor8 = {0, 0, 0, 0xFC};
    GDSetTevColor(static_cast< GXTevRegID >(1), tevColor8);
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(1), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 8);

    GDSetCullMode(static_cast< GXCullMode >(2));
    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(1), static_cast< GXBlendFactor >(1), static_cast< GXLogicOp >(5), 0, 1, 0);
    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(1), static_cast< GXCompare >(7), 0);
    GDSetZMode(1, static_cast< GXCompare >(3), 0);
    GXColor tevColor9 = {0, 0, 0, 4};
    GDSetTevColor(static_cast< GXTevRegID >(1), tevColor9);
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(1), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 9);

    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(6), static_cast< GXBlendFactor >(7), static_cast< GXLogicOp >(5), 1, 1, 0);
    GDSetZMode(1, static_cast< GXCompare >(3), 0);
    GDSetDstAlpha(1, 0);
    storeDisplayList(&displayList, 10);

    GDSetZMode(1, static_cast< GXCompare >(3), 1);
    GDSetGenMode2(1, 0, 1, 0, static_cast< GXCullMode >(1));
    GDSetAlphaCompare(static_cast< GXCompare >(4), 0x20, static_cast< GXAlphaOp >(0), static_cast< GXCompare >(7), 0);
    GDSetBlendModeEtc(static_cast< GXBlendMode >(0), static_cast< GXBlendFactor >(0), static_cast< GXBlendFactor >(0), static_cast< GXLogicOp >(5), 1, 1, 0);
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetChanCtrl(static_cast< GXChannelID >(5), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetTevColorCalc(static_cast< GXTevStageID >(0), static_cast< GXTevColorArg >(8), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(3), 1, static_cast< GXTevRegID >(0));
    GDSetTevOrder(static_cast< GXTevStageID >(0), static_cast< GXTexCoordID >(0), static_cast< GXTexMapID >(0), static_cast< GXChannelID >(0xFF), static_cast< GXTexCoordID >(1), static_cast< GXTexMapID >(1), static_cast< GXChannelID >(0xFF));
    storeDisplayList(&displayList, 11);

    GDSetZMode(1, static_cast< GXCompare >(3), 0);
    GDSetGenMode2(1, 0, 1, 0, static_cast< GXCullMode >(2));
    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(0), static_cast< GXCompare >(7), 0);
    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(4), static_cast< GXBlendFactor >(1), static_cast< GXLogicOp >(5), 1, 0, 0);
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetChanCtrl(static_cast< GXChannelID >(5), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetTevColorCalc(static_cast< GXTevStageID >(0), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xC), static_cast< GXTevColorArg >(2), static_cast< GXTevColorArg >(0xF), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0));
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(1), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    GDSetTevOrder(static_cast< GXTevStageID >(0), static_cast< GXTexCoordID >(0), static_cast< GXTexMapID >(0), static_cast< GXChannelID >(0xFF), static_cast< GXTexCoordID >(1), static_cast< GXTexMapID >(1), static_cast< GXChannelID >(0xFF));
    storeDisplayList(&displayList, 12);

    GDSetAlphaCompare(static_cast< GXCompare >(7), 0, static_cast< GXAlphaOp >(1), static_cast< GXCompare >(7), 0);
    GDSetZMode(1, static_cast< GXCompare >(6), 0);
    GDSetGenMode2(0, 1, 1, 0, static_cast< GXCullMode >(2));
    GDSetTevDirect(static_cast< GXTevStageID >(0));
    GDSetChanCtrl(static_cast< GXChannelID >(4), 0, static_cast< GXColorSrc >(0), static_cast< GXColorSrc >(0), 0, static_cast< GXDiffuseFn >(2), static_cast< GXAttnFn >(2));
    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(1), static_cast< GXBlendFactor >(4), static_cast< GXLogicOp >(0), 1, 0, 0);
    GXColor tevColor13 = {0x23, 0x19, 0x19, 5};
    GDSetTevColor(static_cast< GXTevRegID >(1), tevColor13);
    GDSetTevColorCalc(static_cast< GXTevStageID >(0), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(0xF), static_cast< GXTevColorArg >(2), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0));
    GDSetTevAlphaCalcAndSwap(static_cast< GXTevStageID >(0), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(7), static_cast< GXTevAlphaArg >(1), static_cast< GXTevOp >(0), static_cast< GXTevBias >(0), static_cast< GXTevScale >(0), 1, static_cast< GXTevRegID >(0), static_cast< GXTevSwapSel >(0), static_cast< GXTevSwapSel >(0));
    storeDisplayList(&displayList, 13);

    GDSetDstAlpha(1, 0x40);
    storeDisplayList(&displayList, 14);

    GDSetBlendModeEtc(static_cast< GXBlendMode >(1), static_cast< GXBlendFactor >(7), static_cast< GXBlendFactor >(6), static_cast< GXLogicOp >(0), 1, 0, 0);
    storeDisplayList(&displayList, 15);

    _1E0 = -1;
    _1DC = 0x32323232;
    _1D8 = 0xFF0000FF;
    mFlags.clear();
    _1D4 = 0.0f;
    _1D0 = 0;
    _1B4 = 0;
    _1B8 = nullptr;
    _1BC = 0;

    const u16 materialCount = mModelData->getMaterialNum();
    _1C4 = new u8*[materialCount];
    _1C8 = new u8*[materialCount];
    _1CC = new u16[materialCount];

    for (u16 i = 0; i < materialCount; i++) {
        _1C4[i] = 0;
        _1C8[i] = nullptr;
        _1CC[i] = 0;
    }

    _DC = 0;
}

void J3DModelX::shapePacketDrawFast(J3DShapePacketX* packet) const {
    if (!packet->checkFlag(J3DShpFlag_Hidden) && packet->mpShape != nullptr) {
        packet->prepareDraw();

        if (packet->mpTexMtxObj != nullptr) {
            J3DDifferedTexMtx::sTexGenBlock = packet->mpShape->getMaterial()->getTexGenBlock();
            J3DDifferedTexMtx::sTexMtxObj = packet->mpTexMtxObj;
        } else {
            J3DDifferedTexMtx::sTexGenBlock = nullptr;
        }

        shapeDrawFast(reinterpret_cast< J3DShapeX* >(packet->mpShape));
    }
}

void J3DModelX::shapeDrawFast(J3DShapeX* shape) const {
    if (J3DShape::sOldVcdVatCmd != shape->mVcdVatCmd) {
        GXCallDisplayList(shape->mVcdVatCmd, J3DShape::kVcdVatDLSize);
        J3DShape::sOldVcdVatCmd = shape->mVcdVatCmd;
    }

    if (J3DShape::sEnvelopeFlag && !shape->mHasPNMTXIdx) {
        shape->mCurrentMtx.load();
    }

    shape->setArrayAndBindPipeline();
    if (!shape->checkFlag(J3DShpFlag_NoMtx)) {
        if (J3DShapeMtx::getLODFlag()) {
            J3DShapeMtx::resetMtxLoadCache();
        }

        const u16 groupNum = shape->mMtxGroupNum;
        for (u16 i = 0; i < groupNum; i++) {
            if (shape->mShapeMtx[i] != nullptr) {
                shape->mShapeMtx[i]->load();
            }
            if (mShapeCallback != nullptr) {
                mShapeCallback(shape);
            }
            if (shape->mShapeDraw[i] != nullptr) {
                shape->mShapeDraw[i]->draw();
            }
        }
    } else {
        MtxPtr baseMtx = *j3dSys.getShapePacket()->getBaseMtxPtr();
        J3DFifoLoadPosMtxImm(baseMtx, GX_PNMTX0);
        J3DFifoLoadNrmMtxImm(baseMtx, GX_PNMTX0);

        const u16 groupNum = shape->mMtxGroupNum;
        for (u16 i = 0; i < groupNum; i++) {
            if (shape->mShapeDraw[i] != nullptr) {
                shape->mShapeDraw[i]->draw();
            }
        }
    }
}

J3DModelX::~J3DModelX() {
}

#ifndef TARGET_PC
J3DModel::~J3DModel() {
}
#endif
