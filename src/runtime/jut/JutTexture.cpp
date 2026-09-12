#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JUtility/JUTPalette.hpp>

#include "compat/JutTextureAllocation.hpp"
#include "compat/JkrHeapFinalizer.hpp"
#include "compat/JutTextureConstruction.hpp"

namespace {
    void retire_jut_texture(void* object) noexcept {
        static_cast<JUTTexture*>(object)->~JUTTexture();
    }
}

JUTTexture::JUTTexture() {
    setCaptureFlag(false);
    try {
        smgpc::compat::register_jkr_heap_finalizer(this, retire_jut_texture);
        smgpc::compat::record_completed_jut_texture(*this);
    } catch (...) {
        smgpc::compat::unregister_jkr_heap_finalizer(this);
        throw;
    }
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
    try {
        smgpc::compat::register_jkr_heap_finalizer(this, retire_jut_texture);
        smgpc::compat::record_completed_jut_texture(*this);
    } catch (...) {
        smgpc::compat::unregister_jkr_heap_finalizer(this);
        throw;
    }
    allocation.commit();
}

JUTTexture::JUTTexture(const ResTIMG *p_timg, u8 param_1) {
    try {
        storeTIMG(p_timg, param_1);
        setCaptureFlag(false);
        smgpc::compat::register_jkr_heap_finalizer(this, retire_jut_texture);
        smgpc::compat::record_completed_jut_texture(*this);
    } catch (...) {
        smgpc::compat::unregister_jkr_heap_finalizer(this);
        GXDestroyTexObj(&mObj);
        if (getEmbPaletteDelFlag()) delete mEmbPalette;
        throw;
    }
}

JUTTexture::~JUTTexture() {
    smgpc::compat::forget_completed_jut_texture(*this);
    smgpc::compat::unregister_jkr_heap_finalizer(this);
    GXDestroyTexObj(&mObj);
    if (getCaptureFlag()) {
        smgpc::compat::release_owned_jut_texture(*this);
    }
    if (getEmbPaletteDelFlag()) delete mEmbPalette;
}

void JUTTexture::storeTIMG(const ResTIMG* pTIMG, u8 a1) {
    if (pTIMG != nullptr && a1 < 0x10) {
        mTIMG = pTIMG;
        mImage = reinterpret_cast< u8* >(const_cast< ResTIMG* >(pTIMG)) + pTIMG->mImageDataOffset;
        if (pTIMG->mImageDataOffset == 0)
            mImage = reinterpret_cast< u8* >(const_cast< ResTIMG* >(pTIMG)) + sizeof(ResTIMG);

        mPalette = nullptr;
        mTlutName = 0;
        mWrapS = getTexInfo()->mWrapS;
        mWrapT = getTexInfo()->mWrapT;
        mMinType = getTexInfo()->mMinType;
        mMagType = getTexInfo()->mMagType;
        mMinLod = static_cast< s8 >(getTexInfo()->mMinLod);
        mMaxLod = static_cast< s8 >(getTexInfo()->mMaxLod);
        mLodBias = static_cast< s16 >(getTexInfo()->mLodBias);

        if (getTexInfo()->mPaletteNum == 0) {
            initTexObj();
            return;
        }

        s32 lut;
        if (getTexInfo()->mPaletteNum > 256) {
            lut = (a1 % 4) + GX_BIGTLUT0;
        } else {
            lut = a1;
        }

        if (mEmbPalette == nullptr || (mFlag & 2) == 0) {
            mEmbPalette = new JUTPalette(static_cast< GXTlut >(lut), static_cast< GXTlutFmt >(getTexInfo()->mPaletteFormat),
                                         static_cast< JUTTransparency >(getTexInfo()->mTransparency), getTexInfo()->mPaletteNum,
                                         reinterpret_cast< u8* >(const_cast< ResTIMG* >(getTexInfo())) + getTexInfo()->mPaletteDataOffset);
            setEmbPaletteDelFlag(true);
        } else {
            mEmbPalette->storeTLUT(static_cast< GXTlut >(lut), static_cast< GXTlutFmt >(getTexInfo()->mPaletteFormat),
                                   static_cast< JUTTransparency >(getTexInfo()->mTransparency), getTexInfo()->mPaletteNum,
                                   reinterpret_cast< u8* >(const_cast< ResTIMG* >(getTexInfo())) + getTexInfo()->mPaletteDataOffset);
        }
        attachPalette(mEmbPalette);
    }
}

void JUTTexture::storeTIMG(const ResTIMG* pTIMG, JUTPalette* pPalette) {
    GXTlut tlut;

    if (pPalette != nullptr) {
        tlut = static_cast< GXTlut >(pPalette->mName);
    } else {
        tlut = GX_TLUT0;
    }

    storeTIMG(pTIMG, pPalette, tlut);
}

void JUTTexture::storeTIMG(ResTIMG const* pTIMG, JUTPalette* pPalette, GXTlut a1) {
    if (pTIMG != nullptr) {
        mTIMG = pTIMG;
        mImage = reinterpret_cast< u8* >(const_cast< ResTIMG* >(pTIMG)) + pTIMG->mImageDataOffset;
        if (pTIMG->mImageDataOffset == 0)
            mImage = reinterpret_cast< u8* >(const_cast< ResTIMG* >(pTIMG)) + sizeof(ResTIMG);

        if ((mFlag & 2) != 0) {
            delete mEmbPalette;
        }
        mEmbPalette = pPalette;
        setEmbPaletteDelFlag(false);
        mPalette = nullptr;
        if (pPalette != nullptr) {
            mTlutName = a1;
            if (a1 != pPalette->mName) {
                pPalette->storeTLUT(a1, static_cast< GXTlutFmt >(pPalette->mFormat), static_cast< JUTTransparency >(pPalette->mTransparency),
                                    pPalette->mLutNum, pPalette->mColorTable);
            }
        }

        mWrapS = getTexInfo()->mWrapS;
        mWrapT = getTexInfo()->mWrapT;
        mMinType = getTexInfo()->mMinType;
        mMagType = getTexInfo()->mMagType;
        mMinLod = static_cast< s8 >(getTexInfo()->mMinLod);
        mMaxLod = static_cast< s8 >(getTexInfo()->mMaxLod);
        mLodBias = static_cast< s16 >(getTexInfo()->mLodBias);
        init();
    }
}

void JUTTexture::attachPalette(JUTPalette* pPalette) {
    if (mTIMG->mPaletteName == GX_TLUT0) {
        return;
    }

    if (pPalette == nullptr && mEmbPalette != nullptr) {
        mPalette = mEmbPalette;
    } else {
        mPalette = pPalette;
    }

    initTexObj(static_cast< GXTlut >(mPalette->mName));
}

void JUTTexture::init() {
    if (mTIMG->mPaletteNum == 0) {
        initTexObj();
    } else if (mEmbPalette != nullptr) {
        mPalette = mEmbPalette;

        initTexObj(static_cast< GXTlut >(mPalette->mName));
    }
}

void JUTTexture::initTexObj() {
    GXBool mipmapEnabled;
    if (mTIMG->mMipmap != 0) {
        mipmapEnabled = 1;
    } else {
        mipmapEnabled = 0;
    }
    u8* image = reinterpret_cast< u8* >(const_cast< ResTIMG* >(mTIMG));
    image += (mTIMG->mImageDataOffset ? mTIMG->mImageDataOffset : 0x20);
    GXInitTexObj(&mObj, image, mTIMG->mWidth, mTIMG->mHeight, static_cast< GXTexFmt >(mTIMG->mFormat), static_cast< GXTexWrapMode >(mWrapS),
                 static_cast< GXTexWrapMode >(mWrapT), mipmapEnabled);

    GXInitTexObjLOD(&mObj, static_cast< GXTexFilter >(mMinType), static_cast< GXTexFilter >(mMagType), mMinLod / 8.0f, mMaxLod / 8.0f,
                    mLodBias / 100.0f, mTIMG->mBiasClamp, mTIMG->mDoEdgeLod, static_cast< GXAnisotropy >(mTIMG->mMaxAnisotropy));
}

void JUTTexture::initTexObj(GXTlut lut) {
    GXBool mipmapEnabled;
    if (mTIMG->mMipmap != 0) {
        mipmapEnabled = 1;
    } else {
        mipmapEnabled = 0;
    }
    mTlutName = lut;
    u8* image = reinterpret_cast< u8* >(const_cast< ResTIMG* >(mTIMG));
    image += (mTIMG->mImageDataOffset ? mTIMG->mImageDataOffset : 0x20);
    GXInitTexObjCI(&mObj, image, mTIMG->mWidth, mTIMG->mHeight, static_cast< GXCITexFmt >(mTIMG->mFormat), static_cast< GXTexWrapMode >(mWrapS),
                   static_cast< GXTexWrapMode >(mWrapT), mipmapEnabled, lut);

    GXInitTexObjLOD(&mObj, static_cast< GXTexFilter >(mMinType), static_cast< GXTexFilter >(mMagType), mMinLod / 8.0f, mMaxLod / 8.0f,
                    mLodBias / 100.0f, mTIMG->mBiasClamp, mTIMG->mDoEdgeLod, static_cast< GXAnisotropy >(mTIMG->mMaxAnisotropy));
}

void JUTTexture::load(GXTexMapID texMapID) {
    if (mPalette != nullptr) {
        mPalette->load();
    }

    GXLoadTexObj(&mObj, texMapID);
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
