#include "JSystem/J3DGraphBase/J3DTevs.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"
#include "compat/BigEndian.hpp"

void J3DLightObj::load(u32 index) const {
    GDOverflowCheck(0x48);
    GXLightID id = static_cast<GXLightID>(1 << index);
    J3DGDSetLightPos(id, mInfo.mLightPosition.x, mInfo.mLightPosition.y, mInfo.mLightPosition.z);
    J3DGDSetLightAttn(id, mInfo.mCosAtten.x, mInfo.mCosAtten.y, mInfo.mCosAtten.z,
                     mInfo.mDistAtten.x, mInfo.mDistAtten.y, mInfo.mDistAtten.z);
    J3DGDSetLightColor(id, mInfo.mColor);
    J3DGDSetLightDir(id, mInfo.mLightDirection.x, mInfo.mLightDirection.y, mInfo.mLightDirection.z);
}

const GXColor j3dDefaultColInfo = {255, 255, 255, 255};
const GXColor j3dDefaultAmbInfo = {50, 50, 50, 50};
const u8 j3dDefaultNumChans = 1;
const J3DTevOrderInfo j3dDefaultTevOrderInfoNull = {255, 255, 255, 0};
const J3DIndTexOrderInfo j3dDefaultIndTexOrderNull = {255, 255, 0, 0};
const GXColor j3dDefaultTevKColor = {255, 255, 255, 255};
const GXColorS10 j3dDefaultTevColor = {255, 255, 255, 255};
const J3DTevSwapModeTableInfo j3dDefaultTevSwapModeTable = {0, 1, 2, 3};
const J3DBlendInfo j3dDefaultBlendInfo = {1, 4, 5, 5};
const u8 j3dDefaultTevSwapTableID = 0x1B;
const u16 j3dDefaultAlphaCmpID = 0xE7;
const u16 j3dDefaultZModeID = 0x17;
const J3DColorChanInfo j3dDefaultColorChanInfo = {0, 0, 0, 2, 2, 0, {255, 255}};
const J3DIndTexCoordScaleInfo j3dDefaultIndTexCoordScaleInfo = {};
const J3DTevSwapModeInfo j3dDefaultTevSwapMode = {};

void loadTexCoordGens(u32 texGenNum, J3DTexCoord* texCoords) {
    u32 var_r28;
    GDOverflowCheck(texGenNum * 4 * 2 + 10);
    J3DGDWriteXFCmdHdr(0x1040, texGenNum);

    for (int i = 0; i < texGenNum; i++) {
        J3DGDSetTexCoordGen(GXTexGenType(texCoords[i].getTexGenType()), GXTexGenSrc(texCoords[i].getTexGenSrc()));
    }

    var_r28 = 61;
    J3DGDWriteXFCmdHdr(0x1050, texGenNum);

    if (j3dSys.checkFlag(0x40000000)) {
        for (int i = 0; i < texGenNum; i++) {
            if (texCoords[i].getTexGenMtx() != 60) {
                var_r28 = i * 3;
            } else {
                var_r28 = 61;
            }
            J3DGDWrite_u32(var_r28);
        }
    } else {
        for (int i = 0; i < texGenNum; i++) {
            J3DGDWrite_u32(var_r28);
        }
    }
}


bool isTexNoReg(void* pDL) {
    u8 r31 = ((u8*)pDL)[1];
    if (r31 >= 0x80 && r31 <= 0xbb) {
        return true;
    }
    return false;
}

u16 getTexNoReg(void* pDL) {
    u32 var_r31 = smgpc::compat::read_be_u32((u8*)pDL + 1);
    return var_r31 & 0xFFFFFF;
}

void loadTexNo(u32 param_0, const u16& texNo) {
    ResTIMG* resTIMG = j3dSys.getTexture()->getResTIMG(texNo);
    J3DSys::sTexCoordScaleTable[param_0].field_0x00 = (u16)resTIMG->mWidth;
    J3DSys::sTexCoordScaleTable[param_0].field_0x02 = (u16)resTIMG->mHeight;

    GDOverflowCheck(0x14);
    J3DGDSetTexImgPtr(GXTexMapID(param_0), (u8*)resTIMG + resTIMG->mImageDataOffset);
    J3DGDSetTexImgAttr(GXTexMapID(param_0), resTIMG->mWidth, resTIMG->mHeight, GXTexFmt(resTIMG->mFormat & 0x0f));
    J3DGDSetTexLookupMode(GXTexMapID(param_0), GXTexWrapMode(resTIMG->mWrapS), GXTexWrapMode(resTIMG->mWrapT), GXTexFilter(resTIMG->mMinType),
                          GXTexFilter(resTIMG->mMagType), resTIMG->mMinLod * 0.125f, resTIMG->mMaxLod * 0.125f, resTIMG->mLodBias * 0.01f,
                          resTIMG->mBiasClamp, resTIMG->mDoEdgeLod, GXAnisotropy(resTIMG->mMaxAnisotropy));

    if (resTIMG->mPaletteName == true) {
        GXTlutSize tlutSize = resTIMG->mPaletteNum > 16 ? GX_TLUT_256 : GX_TLUT_16;
        GDOverflowCheck(0x14);
        J3DGDLoadTlut((u8*)resTIMG + resTIMG->mPaletteDataOffset, (param_0 << 13) + 0xf0000, tlutSize);
        J3DGDSetTexTlut(GXTexMapID(param_0), (param_0 << 13) + 0xf0000, GXTlutFmt(resTIMG->mPaletteFormat));
    }
}

void patchTexNo_PtrToIdx(u32 texID, const u16& idx) {
    ResTIMG* timg = j3dSys.getTexture()->getResTIMG(idx);
    J3DGDSetTexImgPtrRaw(GXTexMapID(texID), idx);
}


const J3DLightInfo j3dDefaultLightInfo = {
    0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0xff, 0xff, 0xff, 0xff, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
};

J3DTexCoordInfo const j3dDefaultTexCoordInfo[8] = {
    {GX_MTX2x4, GX_TG_TEX0, GX_IDENTITY, 0}, {GX_MTX2x4, GX_TG_TEX1, GX_IDENTITY, 0}, {GX_MTX2x4, GX_TG_TEX2, GX_IDENTITY, 0},
    {GX_MTX2x4, GX_TG_TEX3, GX_IDENTITY, 0}, {GX_MTX2x4, GX_TG_TEX4, GX_IDENTITY, 0}, {GX_MTX2x4, GX_TG_TEX5, GX_IDENTITY, 0},
    {GX_MTX2x4, GX_TG_TEX6, GX_IDENTITY, 0}, {GX_MTX2x4, GX_TG_TEX7, GX_IDENTITY, 0},
};


J3DIndTexMtxInfo const j3dDefaultIndTexMtxInfo = {0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1};

J3DTevStageInfo const j3dDefaultTevStageInfo = {
    0x04, 0x0A, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
};

J3DIndTevStageInfo const j3dDefaultIndTevStageInfo = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

J3DFogInfo const j3dDefaultFogInfo = {
    0x00,   0x00,   0x0140, 0.0f,   0.0f,   0.1f,   10000.0f, 0xFF,   0xFF,   0xFF,   0x00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   0x0000, 0x0000, 0x0000,
};

J3DNBTScaleInfo const j3dDefaultNBTScaleInfo = {
    0x00,
    1.0f,
    1.0f,
    1.0f,
};
