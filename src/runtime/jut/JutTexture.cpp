#include <JSystem/JUtility/JUTTexture.hpp>

#include "compat/JutTextureAllocation.hpp"
#include "compat/JkrHeapFinalizer.hpp"

namespace {
    void retire_jut_texture(void* object) noexcept {
        static_cast<JUTTexture*>(object)->~JUTTexture();
    }
}

JUTTexture::JUTTexture() {
    setCaptureFlag(false);
    smgpc::compat::register_jkr_heap_finalizer(this, retire_jut_texture);
}

JUTTexture::JUTTexture(int width, int height, GXTexFmt format) {
    mFlag = mFlag & 2 | 1;
    u32 bufSize = GXGetTexBufferSize(width, height, format, GX_FALSE, 1);

    auto allocation = smgpc::compat::allocate_owned_jut_texture(*this, static_cast<std::size_t>(bufSize) + sizeof(ResTIMG));
    ResTIMG* texBuf = static_cast<ResTIMG*>(allocation.data());
    _3C = texBuf;
    texBuf->mFormat = format;
    texBuf->mTransparency = 0;
    texBuf->mWidth = width;
    texBuf->mHeight = height;
    texBuf->mWrapS = GX_CLAMP;
    texBuf->mWrapT = GX_CLAMP;
    texBuf->mPaletteName = GX_TLUT0;
    texBuf->mPaletteFormat = GX_TL_IA8;
    texBuf->mPaletteNum = 0;
    texBuf->mPaletteDataOffset = 0;
    texBuf->mMipmap = false;
    texBuf->mDoEdgeLod = false;
    texBuf->mBiasClamp = false;
    texBuf->mMaxAnisotropy = GX_ANISO_1;
    texBuf->mMinType = GX_LINEAR;
    texBuf->mMagType = GX_LINEAR;
    texBuf->mMinLod = 0;
    texBuf->mMaxLod = 0;
    texBuf->mImageNum = 1;
    texBuf->mLodBias = 0;
    texBuf->mImageDataOffset = sizeof(ResTIMG);
    mEmbPalette = nullptr;

    // cast to u8 solves ambiguity
    storeTIMG(texBuf, static_cast< u8 >(0));
    DCFlushRange(mImage, bufSize);
    smgpc::compat::register_jkr_heap_finalizer(this, retire_jut_texture);
    allocation.commit();
}

JUTTexture::JUTTexture(const ResTIMG *p_timg, u8 param_1) {
    storeTIMG(p_timg, param_1);
    setCaptureFlag(false);
    try {
        smgpc::compat::register_jkr_heap_finalizer(this, retire_jut_texture);
    } catch (...) {
        GXDestroyTexObj(&mObj);
        throw;
    }
}

JUTTexture::~JUTTexture() {
    smgpc::compat::unregister_jkr_heap_finalizer(this);
    GXDestroyTexObj(&mObj);
    if (getCaptureFlag()) {
        smgpc::compat::release_owned_jut_texture(*this);
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
