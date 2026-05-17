#include <JSystem/JUtility/JUTTexture.hpp>

#include <algorithm>

JUTTexture::JUTTexture() {
    setCaptureFlag(false);
}

JUTTexture::JUTTexture(int width, int height, GXTexFmt format) {
    setCaptureFlag(true);

    const auto clamped_width = static_cast<u16>(std::max(width, 1));
    const auto clamped_height = static_cast<u16>(std::max(height, 1));
    const auto buffer_size = GXGetTexBufferSize(clamped_width, clamped_height, format, GX_FALSE, 1U);
    auto *bytes = new u8[sizeof(ResTIMG) + buffer_size]{};
    auto *tex_buf = reinterpret_cast<ResTIMG *>(bytes);
    _3C = tex_buf;

    tex_buf->mFormat = static_cast<u8>(format);
    tex_buf->mTransparency = 0U;
    tex_buf->mWidth = clamped_width;
    tex_buf->mHeight = clamped_height;
    tex_buf->mWrapS = GX_CLAMP;
    tex_buf->mWrapT = GX_CLAMP;
    tex_buf->mPaletteName = GX_TLUT0;
    tex_buf->mPaletteFormat = GX_TL_IA8;
    tex_buf->mPaletteNum = 0U;
    tex_buf->mPaletteDataOffset = 0U;
    tex_buf->mMipmap = false;
    tex_buf->mDoEdgeLod = false;
    tex_buf->mBiasClamp = false;
    tex_buf->mMaxAnisotropy = GX_ANISO_1;
    tex_buf->mMinType = GX_LINEAR;
    tex_buf->mMagType = GX_LINEAR;
    tex_buf->mMinLod = 0U;
    tex_buf->mMaxLod = 0U;
    tex_buf->mImageNum = 1U;
    tex_buf->mLodBias = 0;
    tex_buf->mImageDataOffset = sizeof(ResTIMG);

    storeTIMG(tex_buf, static_cast<u8>(0U));
    DCFlushRange(mImage, buffer_size);
}

JUTTexture::JUTTexture(const ResTIMG *p_timg, u8 param_1) {
    storeTIMG(p_timg, param_1);
    setCaptureFlag(false);
}

JUTTexture::~JUTTexture() {
    if (getCaptureFlag()) {
        delete[] reinterpret_cast<u8 *>(_3C);
    }
}

void JUTTexture::storeTIMG(const ResTIMG *timg, u8 tlut_name) {
    if (timg == nullptr || tlut_name >= 0x10U) {
        return;
    }

    mTIMG = timg;
    mImage = const_cast<u8 *>(reinterpret_cast<const u8 *>(timg) + (timg->mImageDataOffset != 0U ? timg->mImageDataOffset : sizeof(ResTIMG)));
    mPalette = nullptr;
    mTlutName = tlut_name;
    mWrapS = timg->mWrapS;
    mWrapT = timg->mWrapT;
    mMinType = timg->mMinType;
    mMagType = timg->mMagType;
    mMinLod = timg->mMinLod;
    mMaxLod = timg->mMaxLod;
    mLodBias = timg->mLodBias;
    init();
}

void JUTTexture::storeTIMG(const ResTIMG *timg, JUTPalette *palette) {
    storeTIMG(timg, palette, palette != nullptr ? static_cast<GXTlut>(mTlutName) : GX_TLUT0);
}

void JUTTexture::storeTIMG(const ResTIMG *timg, JUTPalette *palette, GXTlut tlut) {
    storeTIMG(timg, static_cast<u8>(tlut));
    mEmbPalette = palette;
    mPalette = palette;
}

void JUTTexture::attachPalette(JUTPalette *palette) {
    mPalette = palette;
}

void JUTTexture::init() {
    initTexObj();
}

void JUTTexture::initTexObj() {
    if (mTIMG == nullptr) {
        return;
    }

    GXInitTexObj(&mObj, mImage, mTIMG->mWidth, mTIMG->mHeight, static_cast<GXTexFmt>(mTIMG->mFormat), static_cast<GXTexWrapMode>(mWrapS),
                 static_cast<GXTexWrapMode>(mWrapT), mTIMG->mMipmap ? GX_TRUE : GX_FALSE);
    GXInitTexObjLOD(&mObj, static_cast<GXTexFilter>(mMinType), static_cast<GXTexFilter>(mMagType), static_cast<f32>(mMinLod) / 8.0F,
                    static_cast<f32>(mMaxLod) / 8.0F, static_cast<f32>(mLodBias) / 100.0F, mTIMG->mBiasClamp ? GX_TRUE : GX_FALSE,
                    mTIMG->mDoEdgeLod ? GX_TRUE : GX_FALSE, static_cast<GXAnisotropy>(mTIMG->mMaxAnisotropy));
}

void JUTTexture::initTexObj(GXTlut tlut) {
    mTlutName = static_cast<u8>(tlut);
    initTexObj();
}

void JUTTexture::load(GXTexMapID tex_map_id) {
    GXLoadTexObj(&mObj, tex_map_id);
}

void JUTTexture::capture(int width, int height, GXTexFmt format, bool mipmap, u8 clear) {
    if (!getCaptureFlag() || mTIMG == nullptr) {
        return;
    }

    if (mipmap) {
        GXSetTexCopySrc(static_cast<u16>(width), static_cast<u16>(height), static_cast<u16>(mTIMG->mWidth * 2U),
                        static_cast<u16>(mTIMG->mHeight * 2U));
    } else {
        GXSetTexCopySrc(static_cast<u16>(width), static_cast<u16>(height), mTIMG->mWidth, mTIMG->mHeight);
    }

    GXSetTexCopyDst(mTIMG->mWidth, mTIMG->mHeight, format, mipmap ? GX_TRUE : GX_FALSE);
    GXCopyTex(mImage, clear != 0U ? GX_TRUE : GX_FALSE);
    GXPixModeSync();
}
