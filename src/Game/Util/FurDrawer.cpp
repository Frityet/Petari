#include "Game/Util/FurDrawer.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/FurParam.hpp"
#include "Game/Util/MathUtil.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <cmath>
#include <aurora/endian.hpp>

FurDrawer::FurDrawer(u32 layerCount, ResTIMG* baseTexture, ResTIMG* indirectTexture)
    : mBaseTexture(nullptr), mFurTexture(nullptr), mIndirectTexture(nullptr), mLayerCount(layerCount) {
    mLength.mEnd = 3.0f;
    mLength.mExponent = 1.0f;
    mIndirect.mEnd = 0.2f;
    mIndirect.mExponent = 1.5f;
    mBrightness.mEnd = 40.0f;
    mBrightness.mExponent = 4.5f;
    mAlpha.mEnd = 210.0f;
    mAlpha.mExponent = 1.8f;
    mFurScale = 1.0f;
    mBaseScale = 1.0f;
    mOffset.mEnd = 0.0f;
    mOffset.mExponent = 1.0f;
    mCullMode = GX_CULL_BACK;
    mMaterial = 255;
    mAmbient = 50;
    mUseIndirect = 1;
    mZCompLoc = 0;
    mZUpdate = 0;
    mAdditive = 0;
    mAlphaRef = 32;
    mBaseTexture = new JUTTexture(baseTexture, 0);
    if (mIndirectTexture != nullptr) {
        mIndirectTexture = new JUTTexture(indirectTexture, 0);
        mUseIndirect = 1;
    } else {
        mUseIndirect = 0;
    }

    u32 textureSize = GXGetTexBufferSize(32, 32, GX_TF_IA8, GX_FALSE, 1);
    ResTIMG* image = reinterpret_cast<ResTIMG*>(new (32) u8[textureSize + sizeof(ResTIMG)]);
    image->mFormat = GX_TF_IA8;
    image->mWidth = 32;
    image->mHeight = 32;
    image->mWrapS = GX_REPEAT;
    image->mWrapT = GX_REPEAT;
    image->mPaletteName = 0;
    image->mPaletteFormat = 0;
    image->mPaletteNum = 0;
    image->mPaletteDataOffset = 0;
    image->mMipmap = false;
    image->mDoEdgeLod = false;
    image->mBiasClamp = false;
    image->mMaxAnisotropy = 0;
    image->mMinType = GX_LINEAR;
    image->mMagType = GX_LINEAR;
    image->mMinLod = 0;
    image->mMaxLod = 0;
    image->mImageNum = 1;
    image->mLodBias = 0;
    image->mImageDataOffset = sizeof(ResTIMG);
    mFurTexture = new JUTTexture(image, 0);
    _F8 = 0;
    PSMTXIdentity(mFurMtx);
    PSMTXIdentity(mIndirectMtx);
    mMixFog = 0;
    mFogPosition.z = 0.0f;
    mFogPosition.y = 0.0f;
    mFogPosition.x = 0.0f;
    mFogRadius = 0.0f;
    mColor.r = 0;
    mColor.g = 0;
    mColor.b = 0;
    mColor.a = 0;
    mDensity[0] = 0.8f;
    mIntensity[0] = 0.517f;
    mTransparency[0] = 26;
    mDensity[1] = 0.6f;
    mIntensity[1] = 0.386f;
    mTransparency[1] = 64;
    mDensity[2] = 0.3f;
    mIntensity[2] = 0.3645f;
    mTransparency[2] = 98;
    mDensity[3] = 0.09f;
    mIntensity[3] = 0.1713f;
    mTransparency[3] = 212;
    createFurMap();
    update();
}

void FurDrawer::update() {
    PSMTXScale(mFurMtx, mFurScale, mFurScale, 1.0f);
}

f32 FurDrawer::CLayerParam::calcValue(s32 layer, s32 layerCount) const {
    f32 t = pow(static_cast<f32>(layer + 1) / static_cast<f32>(layerCount), mExponent);
    return mStart + t * (mEnd - mStart);
}

void FurDrawer::setupMaterial(DynamicFurParam* dynamicParam) const {
    FurLightParam* light = dynamicParam->mLightParam;
    bool useLight1 = light != nullptr && light->mLight1Enabled != 0;
    GXSetCoPlanar(GX_FALSE);
    GXSetCullMode(static_cast<GXCullMode>(mCullMode));
    if (mAdditive) {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
    } else {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    }
    GXSetZMode(GX_TRUE, GX_LEQUAL, mZUpdate);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetAlphaCompare(GX_GREATER, mAlphaRef, GX_AOP_OR, GX_GREATER, mAlphaRef);
    GXSetZCompLoc(mZCompLoc);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetNumTevStages(2);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K3_R);
    GXSetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K3_G);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_KONST);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevKColorSel(GX_TEVSTAGE1, GX_TEV_KCSEL_K3);
    GXSetTevKAlphaSel(GX_TEVSTAGE1, GX_TEV_KASEL_K3_B);
    if (useLight1) {
        GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR1A1);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_C2, GX_CC_CPREV, GX_CC_TEXC, GX_CC_RASC);
    } else {
        GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_C2, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
    }
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    if (mUseIndirect) {
        GXSetNumIndStages(1);
        GXSetTevIndirect(GX_TEVSTAGE1, GX_INDTEXSTAGE0, GX_ITF_8, GX_ITB_STU, GX_ITM_0, GX_ITW_OFF, GX_ITW_OFF,
                         GX_FALSE, GX_FALSE, GX_ITBA_OFF);
    } else {
        GXSetNumIndStages(0);
        GXSetTevDirect(GX_TEVSTAGE1);
    }
    if (dynamicParam->mFogCtrl != nullptr) {
        const J3DFogInfo& fog = dynamicParam->mFogCtrl->mFogInfo;
        GXSetFog(static_cast<GXFogType>(fog.mType), fog.mStartZ, fog.mEndZ, fog.mNearZ, fog.mFarZ, fog.mColor);
    } else if (mMixFog) {
        TDDraw::mixFogColor(mFogPosition, mFogRadius, 0xFF000080);
    } else {
        GXColor color = {0, 0, 0, 0};
        GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, color);
        GXSetFogRangeAdj(GX_FALSE, 0, nullptr);
    }

    Mtx furMtx;
    Mtx indirectMtx;
    Mtx baseMtx;
    PSMTXCopy(mFurMtx, furMtx);
    if (mUseIndirect) {
        PSMTXCopy(mIndirectMtx, indirectMtx);
    } else {
        PSMTXIdentity(indirectMtx);
    }
    PSMTXScale(baseMtx, mBaseScale, mBaseScale, 1.0f);
    GXLoadTexMtxImm(baseMtx, GX_TEXMTX0, GX_MTX2x4);
    GXLoadTexMtxImm(furMtx, GX_TEXMTX1, GX_MTX2x4);
    if (mUseIndirect) {
        GXLoadTexMtxImm(indirectMtx, GX_TEXMTX2, GX_MTX2x4);
        GXSetNumTexGens(3);
        GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
        GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
        GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX2, GX_FALSE, GX_PTIDENTITY);
    } else {
        GXSetNumTexGens(2);
    }
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1, GX_FALSE, GX_PTIDENTITY);
    GXSetClipMode(GX_CLIP_DISABLE);
    if (light != nullptr) {
        u8 channelCount = 1;
        GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR0, GX_TRUE, static_cast<GXColorSrc>(light->mLightColorSource & 1),
                      static_cast<GXColorSrc>((light->mLightColorSource >> 1) & 1), light->mLight0Enabled, GX_DF_CLAMP, GX_AF_NONE);
        if (useLight1) {
            GXSetChanCtrl(GX_COLOR1, GX_TRUE, static_cast<GXColorSrc>((light->mLightColorSource >> 2) & 1),
                          static_cast<GXColorSrc>((light->mLightColorSource >> 3) & 1), light->mLight1Enabled, GX_DF_NONE, GX_AF_SPEC);
            GXSetChanCtrl(GX_ALPHA1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
            channelCount = 2;
        } else {
            GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        }
        GXColor material0 = {0, 0, 0, 255};
        GXColor ambient0 = {0, 0, 0, 255};
        material0.r = material0.g = material0.b = light->mLight0Material;
        ambient0.r = ambient0.g = ambient0.b = light->mLight0Ambient;
        GXSetChanMatColor(GX_COLOR0A0, material0);
        GXSetChanAmbColor(GX_COLOR0A0, ambient0);
        GXColor material1 = {0, 0, 0, 255};
        GXColor ambient1 = {0, 0, 0, 255};
        material1.r = material1.g = material1.b = light->mLight1Material;
        ambient1.r = ambient1.g = ambient1.b = light->mLight1Ambient;
        GXSetChanMatColor(GX_COLOR1A1, material1);
        GXSetChanAmbColor(GX_COLOR1A1, ambient1);
        GXSetNumChans(channelCount);
    } else {
        GXSetChanCtrl(GX_COLOR0, GX_TRUE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
        GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXColor material = {0, 0, 0, 255};
        GXColor ambient = {0, 0, 0, 255};
        material.r = material.g = material.b = mMaterial;
        ambient.r = ambient.g = ambient.b = mAmbient;
        GXSetChanMatColor(GX_COLOR0A0, material);
        GXSetChanAmbColor(GX_COLOR0A0, ambient);
        GXSetNumChans(1);
    }
    mBaseTexture->load(GX_TEXMAP0);
    mFurTexture->load(GX_TEXMAP1);
    if (mUseIndirect && mIndirectTexture != nullptr) {
        mIndirectTexture->load(GX_TEXMAP2);
    }
}

void FurDrawer::setupLayerMaterial(s32 layer) const {
    f32 brightness = mBrightness.calcValue(layer, mLayerCount);
    f32 alpha = mAlpha.calcValue(mLayerCount - layer - 1, mLayerCount);
    f32 offset = mOffset.calcValue(mLayerCount - layer - 1, mLayerCount);
    GXColor layerColor;
    layerColor.r = brightness;
    layerColor.g = alpha;
    layerColor.b = offset;
    layerColor.a = 255;
    GXSetTevKColor(GX_KCOLOR3, layerColor);
    GXColor color;
    color.r = mColor.r;
    color.g = mColor.g;
    color.b = mColor.b;
    color.a = 255;
    GXSetTevColor(GX_TEVREG2, color);
    if (mUseIndirect) {
        f32 amount = mIndirect.calcValue(layer, mLayerCount);
        f32 mtx[2][3];
        mtx[0][0] = amount;
        mtx[0][1] = 0.0f;
        mtx[0][2] = 0.0f;
        mtx[1][0] = 0.0f;
        mtx[1][1] = amount;
        mtx[1][2] = 0.0f;
        GXSetIndTexMtx(GX_ITM_0, mtx, 0);
    }
}

void FurDrawer::createFurMap() {
    u32 width = mFurTexture->getWidth();
    u32 height = mFurTexture->getHeight();
    auto* texels = reinterpret_cast<aurora::endian::BigEndian<u16>*>(mFurTexture->mImage);
    u32 texelCount = height * width;
    for (u32 i = 0; i < texelCount; i++) {
        texels[i] = 255;
    }
    for (u32 layer = 0; layer < 4; layer++) {
        for (u32 i = 0; i < static_cast<f32>(height) * static_cast<f32>(width) * mDensity[layer]; i++) {
            f32 randomY = MR::getRandom(0.0f, 1.0f);
            f32 randomX = MR::getRandom(0.0f, 1.0f);
            s32 y = height * randomY;
            s32 x = width * randomX;
            if (y == height) {
                y = height - 1;
            }
            if (x == width) {
                x = width - 1;
            }
            u16 intensity = 255.0f * mIntensity[layer];
            u16 alpha = 255 - mTransparency[layer];
            texels[width * y + x] = (intensity << 8) | alpha;
        }
    }
    DCStoreRange(texels, texelCount * sizeof(u16));
}
