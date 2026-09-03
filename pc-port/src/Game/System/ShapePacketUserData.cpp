#include "Game/System/ShapePacketUserData.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/SchedulerUtil.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <revolution/gd/GDTransform.h>

ShapePacketUserData::ShapePacketUserData() : mTexGenNum(0), mDisplayListSize(0), mDisplayList(nullptr) {
}

void ShapePacketUserData::init(J3DMaterial* pMaterial) {
    MR::ProhibitSchedulerAndInterrupts prohibit(false);
    bool isEnvMap = MR::isUseTexMtxEnvMap(pMaterial);
    u32 bufferSize = isEnvMap ? 0x20 : 0;
    mTexGenNum = pMaterial->mTexGenBlock->getTexGenNum();

    for (u32 i = 0; i < mTexGenNum; i++) {
        J3DTexCoord* texCoord = pMaterial->mTexGenBlock->getTexCoord(i);
        if (texCoord->getTexGenSrc() == GX_TG_POS) {
            mTexMtxInfo[i].mType = 1;
            mTexMtxInfo[i].mPostTexMtx = GX_PTTEXMTX0 + i * 3;
            bufferSize += 0x20;
        } else if (texCoord->getTexGenSrc() == GX_TG_NRM) {
            mTexMtxInfo[i].mType = 2;
            mTexMtxInfo[i].mPostTexMtx = GX_PTTEXMTX0 + i * 3;
            bufferSize += 0x20;
        } else if (isEnvMap && texCoord->getTexGenMtx() != GX_IDENTITY) {
            mTexMtxInfo[i].mType = 3;
            mTexMtxInfo[i].mPostTexMtx = GX_PTTEXMTX0 + i * 3;
            bufferSize += 0x20;
        } else {
            mTexMtxInfo[i].mType = 0;
            mTexMtxInfo[i].mPostTexMtx = GX_PTIDENTITY;
        }
    }

    mDisplayList = new (0x20) u8[bufferSize + 0x40];
    DCInvalidateRange(mDisplayList, bufferSize + 0x40);
    GDLObj displayList;
    GDInitGDLObj(&displayList, mDisplayList, bufferSize + 0x40);
    GDSetCurrent(&displayList);

    if (isEnvMap) {
        GDSetCurrentMtx(0, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY, GX_IDENTITY);
    }

    for (u32 i = 0; i < mTexGenNum; i++) {
        if (mTexMtxInfo[i].mType != 0) {
            J3DTexCoord* texCoord = pMaterial->mTexGenBlock->getTexCoord(i);
            GDSetTexCoordGen(static_cast< GXTexCoordID >(i), static_cast< GXTexGenType >(texCoord->getTexGenType()),
                            static_cast< GXTexGenSrc >(texCoord->getTexGenSrc()), false, mTexMtxInfo[i].mPostTexMtx);
        }
    }

    GDPadCurr32();
    mDisplayListSize = GDGetGDLObjOffset(&displayList);
    DCStoreRange(mDisplayList, bufferSize + 0x40);
}

void ShapePacketUserData::callDL() const {
    GXCallDisplayList(mDisplayList, mDisplayListSize);
}

void ShapePacketUserData::loadTexMtx(J3DMaterial* pMaterial, int slot, u16 index) const {
    TPos3f matrix;
    for (u32 i = 0; i < mTexGenNum; i++) {
        if (mTexMtxInfo[i].mType != 0) {
            if (mTexMtxInfo[i].mType == 2) {
                matrix.setInline(j3dSys.getModelDrawMtx(index));
                matrix.zeroTrans();
                GXLoadTexMtxImm(matrix, GX_TEXMTX0 + slot * 3,
                               static_cast< GXTexMtxType >(pMaterial->mTexGenBlock->getTexCoord(i)->getTexGenType()));
            }

            J3DTexMtx* texMtx = pMaterial->mTexGenBlock->getTexMtx(i);
            texMtx->calcPostTexMtx(MR::getCameraInvViewMtx());
            GXLoadTexMtxImm(texMtx->getMtx(), mTexMtxInfo[i].mPostTexMtx, GX_MTX3x4);
        }
    }
}

namespace MR {
    ShapePacketUserData* getJ3DShapePacketUserData(const J3DShapePacket* pPacket) {
        void* userData = pPacket->getUserArea();
        if (userData != nullptr) {
            return static_cast< ShapePacketUserData* >(userData);
        }

        return nullptr;
    }

    void initJ3DShapePacketUserData(J3DModel* pModel) {
        J3DModelData* modelData = pModel->getModelData();
        u16 materialNum = modelData->getMaterialNum();
        for (u16 i = 0; i < materialNum; i++) {
            J3DMaterial* material = modelData->getMaterialNodePointer(i);
            J3DShapePacket* packet = pModel->getShapePacket(material->getShape()->getIndex());
            if (isEnvelope(material) && (isUseTexMtxProjMap(material) || isUseTexMtxEnvMap(material))) {
                ShapePacketUserData* userData = new ShapePacketUserData();
                userData->init(material);
                if (userData != nullptr) {
                    packet->setUserArea(reinterpret_cast< uintptr_t >(userData));
                }
            }
        }
    }
}  // namespace MR
