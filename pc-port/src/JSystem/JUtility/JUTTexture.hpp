#pragma once

#include <revolution.h>

class JUTPalette;

struct ResTIMG {
    /* 0x00 */ u8 mFormat = 0U;
    /* 0x01 */ u8 mTransparency = 0U;
    /* 0x02 */ u16 mWidth = 0U;
    /* 0x04 */ u16 mHeight = 0U;
    /* 0x06 */ u8 mWrapS = GX_CLAMP;
    /* 0x07 */ u8 mWrapT = GX_CLAMP;
    /* 0x08 */ u8 mPaletteName = GX_TLUT0;
    /* 0x09 */ u8 mPaletteFormat = GX_TL_IA8;
    /* 0x0A */ u16 mPaletteNum = 0U;
    /* 0x0C */ u32 mPaletteDataOffset = 0U;
    /* 0x10 */ bool mMipmap = false;
    /* 0x11 */ bool mDoEdgeLod = false;
    /* 0x12 */ bool mBiasClamp = false;
    /* 0x13 */ u8 mMaxAnisotropy = GX_ANISO_1;
    /* 0x14 */ u8 mMinType = GX_LINEAR;
    /* 0x15 */ u8 mMagType = GX_LINEAR;
    /* 0x16 */ u8 mMinLod = 0U;
    /* 0x17 */ u8 mMaxLod = 0U;
    /* 0x18 */ u8 mImageNum = 1U;
    /* 0x19 */ u8 _19 = 0U;
    /* 0x1A */ s16 mLodBias = 0;
    /* 0x1C */ u32 mImageDataOffset = sizeof(ResTIMG);
};

class JUTTexture {
public:
    JUTTexture();
    JUTTexture(int width, int height, GXTexFmt format);
    JUTTexture(const ResTIMG* p_timg, u8 param_1);
    ~JUTTexture();

    void storeTIMG(const ResTIMG* timg, u8 tlut_name);
    void storeTIMG(const ResTIMG* timg, JUTPalette* palette);
    void storeTIMG(const ResTIMG* timg, JUTPalette* palette, GXTlut tlut);
    void attachPalette(JUTPalette* palette);
    void init();
    void initTexObj();
    void initTexObj(GXTlut tlut);
    void load(GXTexMapID tex_map_id);
    void capture(int width, int height, GXTexFmt format, bool mipmap, u8 clear);

    [[nodiscard]] const ResTIMG* getTexInfo() const { return mTIMG; }
    [[nodiscard]] s32 getFormat() const { return mTIMG != nullptr ? mTIMG->mFormat : 0; }
    [[nodiscard]] s32 getTransparency() const { return mTIMG != nullptr ? mTIMG->mTransparency : 0; }
    [[nodiscard]] s32 getWidth() const { return mTIMG != nullptr ? mTIMG->mWidth : 0; }
    [[nodiscard]] s32 getHeight() const { return mTIMG != nullptr ? mTIMG->mHeight : 0; }
    void setCaptureFlag(bool flag) { mFlag = static_cast<u8>((mFlag & 0x2U) | (flag ? 0x1U : 0U)); }
    [[nodiscard]] bool getCaptureFlag() const { return (mFlag & 0x1U) != 0U; }
    [[nodiscard]] bool getEmbPaletteDelFlag() const { return (mFlag & 0x2U) != 0U; }
    void setEmbPaletteDelFlag(bool flag) { mFlag = static_cast<u8>((mFlag & 0x1U) | (flag ? 0x2U : 0U)); }
    [[nodiscard]] int getTlutName() const { return mTlutName; }

    /* 0x00 */ GXTexObj mObj{};
    /* 0x20 */ const ResTIMG* mTIMG = nullptr;
    /* 0x24 */ u8* mImage = nullptr;
    /* 0x28 */ JUTPalette* mEmbPalette = nullptr;
    /* 0x2C */ JUTPalette* mPalette = nullptr;
    /* 0x30 */ u8 mWrapS = GX_CLAMP;
    /* 0x31 */ u8 mWrapT = GX_CLAMP;
    /* 0x32 */ u8 mMinType = GX_LINEAR;
    /* 0x33 */ u8 mMagType = GX_LINEAR;
    /* 0x34 */ u16 mMinLod = 0U;
    /* 0x36 */ u16 mMaxLod = 0U;
    /* 0x38 */ s16 mLodBias = 0;
    /* 0x3A */ u8 mTlutName = GX_TLUT0;
    /* 0x3B */ u8 mFlag = 0U;
    /* 0x3C */ ResTIMG* _3C = nullptr;
};
