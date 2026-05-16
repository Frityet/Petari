#include "compat/LayoutTextureCompat.hpp"

#include <span>
#include <utility>

namespace {

[[nodiscard]] std::size_t round_up(std::size_t value, std::size_t alignment) {
    return (value + alignment - 1U) & ~(alignment - 1U);
}

[[nodiscard]] std::size_t gx_texture_data_size(std::uint16_t width, std::uint16_t height, std::uint32_t format) {
    switch (format) {
    case GX_TF_I4:
        return round_up(width, 8U) * round_up(height, 8U) / 2U;
    case GX_TF_I8:
    case GX_TF_IA4:
        return round_up(width, 8U) * round_up(height, 4U);
    case GX_TF_IA8:
    case GX_TF_RGB565:
    case GX_TF_RGB5A3:
        return round_up(width, 4U) * round_up(height, 4U) * 2U;
    case GX_TF_CMPR:
        return round_up(width, 8U) * round_up(height, 8U) / 2U;
    default:
        return 0U;
    }
}

[[nodiscard]] std::uint64_t fnv1a_hash(const std::uint8_t *data, std::size_t size, std::uint64_t seed = 1469598103934665603ULL) {
    std::uint64_t hash = seed;
    for (std::size_t i = 0U; i < size; ++i) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

[[nodiscard]] std::uint64_t hash_u64(std::uint64_t value, std::uint64_t seed) {
    std::uint8_t bytes[8] {};
    for (std::size_t i = 0U; i < 8U; ++i) {
        bytes[i] = static_cast<std::uint8_t>((value >> (i * 8U)) & 0xFFU);
    }
    return fnv1a_hash(bytes, sizeof(bytes), seed);
}

}  // namespace

namespace nw4r::lyt {

TexMap::TexMap() = default;

TexMap::TexMap(const _GXTexObj &rTexObj) {
    Set(rTexObj);
}

TexMap::TexMap(const smgpc::assets::layout::tpl::DecodedImage *pImage, u8 wrapS, u8 wrapT) : mStaticImage(pImage), mWrapS(wrapS), mWrapT(wrapT) {
    if (pImage != nullptr) {
        mWidth = pImage->width;
        mHeight = pImage->height;
    }
}

TexMap::TexMap(smgpc::assets::layout::tpl::DecodedImage image, u8 wrapS, u8 wrapT)
    : mOwnedImage(std::make_shared< smgpc::assets::layout::tpl::DecodedImage >(std::move(image))), mWrapS(wrapS), mWrapT(wrapT) {
    mStaticImage = mOwnedImage.get();
    if (mStaticImage != nullptr) {
        mWidth = mStaticImage->width;
        mHeight = mStaticImage->height;
    }
}

TexMap::TexMap(const ResTIMG *pImage) : mResTIMG(pImage) {
    if (pImage != nullptr) {
        mWidth = pImage->mWidth;
        mHeight = pImage->mHeight;
        mFormat = pImage->mFormat;
        mWrapS = pImage->mWrapS;
        mWrapT = pImage->mWrapT;
        mMipMap = pImage->mMipmap;
        mMinFilter = pImage->mMinType;
        mMagFilter = pImage->mMagType;
    }
}

void TexMap::SetImage(void *pImage) {
    mStaticImage = nullptr;
    mOwnedImage.reset();
    mResTIMG = nullptr;
    (void)pImage;
}

void TexMap::SetSize(u16 width, u16 height) {
    mWidth = width;
    mHeight = height;
}

void TexMap::SetTexelFormat(GXTexFmt value) {
    mFormat = value;
}

void TexMap::SetWrapMode(GXTexWrapMode wrapS, GXTexWrapMode wrapT) {
    mWrapS = wrapS;
    mWrapT = wrapT;
}

void TexMap::SetMipMap(bool mipmap) {
    mMipMap = mipmap;
}

void TexMap::SetFilter(GXTexFilter minFilter, GXTexFilter magFilter) {
    mMinFilter = minFilter;
    mMagFilter = magFilter;
}

void TexMap::SetLOD(f32 minLOD, f32 maxLOD) {
    mMinLOD = minLOD;
    mMaxLOD = maxLOD;
}

void TexMap::SetLODBias(f32 bias) {
    mLODBias = bias;
}

void TexMap::SetBiasClampEnable(bool enable) {
    mBiasClamp = enable;
}

void TexMap::SetEdgeLODEnable(bool enable) {
    mEdgeLOD = enable;
}

void TexMap::SetAnisotropy(GXAnisotropy anisotropy) {
    mAnisotropy = anisotropy;
}

void TexMap::SetPalette(void *pPalette) {
    mPalette = pPalette;
}

void TexMap::SetPaletteFormat(GXTlutFmt format) {
    mPaletteFormat = format;
}

void TexMap::SetPaletteEntryNum(u16 count) {
    mPaletteEntryNum = count;
}

void TexMap::Set(const _GXTexObj &rTexObj) {
    mStaticImage = rTexObj.decoded_image;
    mOwnedImage.reset();
    mResTIMG = nullptr;
    mWidth = rTexObj.width;
    mHeight = rTexObj.height;
    mFormat = rTexObj.format;
    mWrapS = rTexObj.wrap_s;
    mWrapT = rTexObj.wrap_t;
    mMipMap = rTexObj.mipmap;
}

void TexMap::Set(const TexMap &rTexMap) {
    *this = rTexMap;
}

void TexMap::Get(_GXTexObj *pTexObj) const {
    if (pTexObj == nullptr) {
        return;
    }

    const auto *image = decodedImage();
    *pTexObj = _GXTexObj {
        .decoded_image = image,
        .image = image != nullptr && !image->rgba8.empty() ? image->rgba8.data() : nullptr,
        .width = image != nullptr ? image->width : mWidth,
        .height = image != nullptr ? image->height : mHeight,
        .format = mFormat,
        .wrap_s = mWrapS,
        .wrap_t = mWrapT,
        .mipmap = mMipMap,
    };
}

void TexMap::Get(_GXTlutObj *pTlutObj) const {
    if (pTlutObj != nullptr) {
        *pTlutObj = _GXTlutObj {};
    }
}

const smgpc::assets::layout::tpl::DecodedImage *TexMap::decodedImage() const {
    if (mResTIMG != nullptr) {
        return decodeResTIMG();
    }
    if (mOwnedImage != nullptr) {
        return mOwnedImage.get();
    }
    return mStaticImage;
}

std::uint64_t TexMap::textureId() const {
    const auto *image = decodedImage();
    if (mResTIMG != nullptr) {
        return hash_u64(reinterpret_cast< std::uintptr_t >(mResTIMG), mDecodedResTIMGHash);
    }
    return image != nullptr ? static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(image)) : 0U;
}

u8 TexMap::wrapS() const {
    return mWrapS;
}

u8 TexMap::wrapT() const {
    return mWrapT;
}

const smgpc::assets::layout::tpl::DecodedImage *TexMap::decodeResTIMG() const {
    if (mResTIMG == nullptr) {
        return nullptr;
    }

    const auto data_size = gx_texture_data_size(mResTIMG->mWidth, mResTIMG->mHeight, mResTIMG->mFormat);
    if (data_size == 0U) {
        mDecodedResTIMG = {};
        mDecodedResTIMGHash = 0U;
        return &mDecodedResTIMG;
    }

    const auto image_offset = mResTIMG->mImageDataOffset != 0U ? mResTIMG->mImageDataOffset : static_cast< u32 >(sizeof(ResTIMG));
    const auto *base = reinterpret_cast<const std::byte *>(mResTIMG);
    const auto image_data = std::span<const std::byte>(base + image_offset, data_size);
    auto decoded = smgpc::assets::layout::tpl::decode_gx_tiled_texture(image_data, mResTIMG->mWidth, mResTIMG->mHeight, mResTIMG->mFormat);
    if (!decoded) {
        mDecodedResTIMG = {};
        mDecodedResTIMGHash = 0U;
        return &mDecodedResTIMG;
    }

    mDecodedResTIMG = std::move(*decoded);
    mDecodedResTIMGHash = fnv1a_hash(mDecodedResTIMG.rgba8.data(), mDecodedResTIMG.rgba8.size());
    mDecodedResTIMGHash = hash_u64(mDecodedResTIMG.width, mDecodedResTIMGHash);
    mDecodedResTIMGHash = hash_u64(mDecodedResTIMG.height, mDecodedResTIMGHash);
    return &mDecodedResTIMG;
}

}  // namespace nw4r::lyt
