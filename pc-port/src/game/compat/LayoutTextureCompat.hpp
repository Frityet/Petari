#pragma once

#include "compat/Types.hpp"
#include "layout/Tpl.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

struct ResTIMG {
    /* 0x00 */ u8 mFormat {};
    /* 0x01 */ u8 mTransparency {};
    /* 0x02 */ u16 mWidth {};
    /* 0x04 */ u16 mHeight {};
    /* 0x06 */ u8 mWrapS {};
    /* 0x07 */ u8 mWrapT {};
    /* 0x08 */ u8 mPaletteName {};
    /* 0x09 */ u8 mPaletteFormat {};
    /* 0x0A */ u16 mPaletteNum {};
    /* 0x0C */ u32 mPaletteDataOffset {};
    /* 0x10 */ bool mMipmap {};
    /* 0x11 */ bool mDoEdgeLod {};
    /* 0x12 */ bool mBiasClamp {};
    /* 0x13 */ u8 mMaxAnisotropy {};
    /* 0x14 */ u8 mMinType {};
    /* 0x15 */ u8 mMagType {};
    /* 0x16 */ u8 mMinLod {};
    /* 0x17 */ u8 mMaxLod {};
    /* 0x18 */ u8 mImageNum {};
    /* 0x19 */ u8 _19 {};
    /* 0x1A */ s16 mLodBias {};
    /* 0x1C */ u32 mImageDataOffset {};
};

enum GXTexFmt : u32 {
    GX_TF_I4 = 0,
    GX_TF_I8 = 1,
    GX_TF_IA4 = 2,
    GX_TF_IA8 = 3,
    GX_TF_RGB565 = 4,
    GX_TF_RGB5A3 = 5,
    GX_TF_CMPR = 14,
};

enum GXTexWrapMode : u8 {
    GX_CLAMP = 0,
    GX_REPEAT = 1,
    GX_MIRROR = 2,
};

enum GXTexFilter : u8 {
    GX_NEAR = 0,
    GX_LINEAR = 1,
};

enum GXAnisotropy : u8 {
    GX_ANISO_1 = 0,
};

enum GXTlutFmt : u8 {
    GX_TL_IA8 = 0,
};

struct _GXTexObj {
    const smgpc::assets::layout::tpl::DecodedImage *decoded_image {};
    const void *image {};
    u16 width {};
    u16 height {};
    u32 format {};
    u8 wrap_s {};
    u8 wrap_t {};
    bool mipmap {};
};

using GXTexObj = _GXTexObj;

struct _GXTlutObj {
    u32 dummy[3] {};
};

using GXTlutObj = _GXTlutObj;

namespace nw4r::lyt {

class TexMap {
public:
    TexMap();
    explicit TexMap(const _GXTexObj &rTexObj);
    explicit TexMap(const smgpc::assets::layout::tpl::DecodedImage *pImage, u8 wrapS = GX_CLAMP, u8 wrapT = GX_CLAMP);
    explicit TexMap(smgpc::assets::layout::tpl::DecodedImage image, u8 wrapS = GX_CLAMP, u8 wrapT = GX_CLAMP);
    explicit TexMap(const ResTIMG *pImage);

    void SetImage(void *pImage);
    void SetSize(u16 width, u16 height);
    void SetTexelFormat(GXTexFmt value);
    void SetWrapMode(GXTexWrapMode wrapS, GXTexWrapMode wrapT);
    void SetMipMap(bool mipmap);
    void SetFilter(GXTexFilter minFilter, GXTexFilter magFilter);
    void SetLOD(f32 minLOD, f32 maxLOD);
    void SetLODBias(f32 bias);
    void SetBiasClampEnable(bool enable);
    void SetEdgeLODEnable(bool enable);
    void SetAnisotropy(GXAnisotropy anisotropy);
    void SetPalette(void *pPalette);
    void SetPaletteFormat(GXTlutFmt format);
    void SetPaletteEntryNum(u16 count);
    void Set(const _GXTexObj &rTexObj);
    void Set(const TexMap &rTexMap);
    void Get(_GXTexObj *pTexObj) const;
    void Get(_GXTlutObj *pTlutObj) const;

    [[nodiscard]] const smgpc::assets::layout::tpl::DecodedImage *decodedImage() const;
    [[nodiscard]] std::uint64_t textureId() const;
    [[nodiscard]] u8 wrapS() const;
    [[nodiscard]] u8 wrapT() const;

private:
    [[nodiscard]] const smgpc::assets::layout::tpl::DecodedImage *decodeResTIMG() const;

    const smgpc::assets::layout::tpl::DecodedImage *mStaticImage {};
    std::shared_ptr<const smgpc::assets::layout::tpl::DecodedImage> mOwnedImage {};
    const ResTIMG *mResTIMG {};
    mutable smgpc::assets::layout::tpl::DecodedImage mDecodedResTIMG {};
    mutable std::uint64_t mDecodedResTIMGHash {};
    u16 mWidth {};
    u16 mHeight {};
    u32 mFormat {GX_TF_I4};
    u8 mWrapS {GX_CLAMP};
    u8 mWrapT {GX_CLAMP};
    u8 mMinFilter {GX_LINEAR};
    u8 mMagFilter {GX_LINEAR};
    bool mMipMap {};
    bool mBiasClamp {};
    bool mEdgeLOD {};
    u8 mAnisotropy {GX_ANISO_1};
    void *mPalette {};
    u8 mPaletteFormat {GX_TL_IA8};
    u16 mPaletteEntryNum {};
    f32 mMinLOD {};
    f32 mMaxLOD {};
    f32 mLODBias {};
};

}  // namespace nw4r::lyt
