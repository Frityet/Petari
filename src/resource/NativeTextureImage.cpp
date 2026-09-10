#include "NativeTextureImage.hpp"
#include "TplTexture.hpp"
#include <aurora/exception.hpp>

#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace smgpc::resource {
    static_assert(sizeof(ResTIMG) == 0x20);
    static_assert(alignof(ResTIMG) <= 32);
    using Bytes = std::span<const std::uint8_t>;
    namespace {
        void check_range(Bytes bytes, std::size_t offset, std::size_t count) {
            if (offset > bytes.size() || count > bytes.size() - offset) {
                aurora::throw_host_exception<std::runtime_error>("Texture range exceeds its retained source");
            }
        }
        std::uint16_t u16_at(Bytes bytes, std::size_t offset) {
            check_range(bytes, offset, 2);
            return static_cast<std::uint16_t>((std::uint16_t{bytes[offset]} << 8U) | bytes[offset + 1]);
        }
        std::uint32_t u32_at(Bytes bytes, std::size_t offset) {
            return (std::uint32_t{u16_at(bytes, offset)} << 16U) | u16_at(bytes, offset + 2);
        }

    }
    ResTIMG decode_texture_image_header(Bytes bytes) {
        check_range(bytes, 0, sizeof(ResTIMG));
        for (std::size_t offset : {0x10U, 0x11U, 0x12U}) {
            if (bytes[offset] > 1) aurora::throw_host_exception<std::runtime_error>("Texture contains an invalid bool representation");
        }
        ResTIMG result;
        std::memcpy(&result, bytes.data(), sizeof(result));
        result.mWidth = u16_at(bytes, 2);
        result.mHeight = u16_at(bytes, 4);
        result.mPaletteNum = u16_at(bytes, 0xA);
        result.mPaletteDataOffset = u32_at(bytes, 0xC);
        result.mLodBias = std::bit_cast<std::int16_t>(u16_at(bytes, 0x1A));
        result.mImageDataOffset = u32_at(bytes, 0x1C);
        return result;
    }

    void validate_texture_image(Bytes block, std::size_t source, const ResTIMG& image) {
        // File offsets are forward unsigned words. Negative displacements
        // arise only after native J3DTexture::setResTIMG relocation.
        if (image.mImageDataOffset < 0 || image.mPaletteDataOffset < 0) {
            aurora::throw_host_exception<std::runtime_error>("Texture source offsets exceed the mapped graphics heap range");
        }
        if (image.mWidth == 0 || image.mWidth > 1024 || image.mHeight == 0 || image.mHeight > 1024 ||
            image.mWrapS > GX_MIRROR || image.mWrapT > GX_MIRROR || image.mMaxAnisotropy > GX_ANISO_4 ||
            image.mMinType > GX_LIN_MIP_LIN || image.mMagType > GX_LINEAR) {
            aurora::throw_host_exception<std::runtime_error>("Texture has an invalid GX dimension or sampler field");
        }
        u32 levels = 1;
        for (u32 dimension = std::max(image.mWidth, image.mHeight); dimension > 1; dimension >>= 1) ++levels;
        const u32 declared_levels = std::min(std::max<u32>(image.mImageNum, 1), levels);
        const bool mip_filter = image.mMinType >= GX_NEAR_MIP_NEAR;
        const u32 encoded_max_lod = (u32{image.mMaxLod} * 2U) & 0xFFU;
        const u32 sampled_levels = mip_filter ? std::min((encoded_max_lod + 15U) / 16U + 1U, levels) : 1U;
        // loadTexNo uses only the original low format nibble. Retain the
        // complete authored byte in ResTIMG while validating those reads.
        const auto format = image.mFormat & 0xFU;
        std::size_t image_bytes = 0;
        for (u32 level = 0; level < std::max(declared_levels, sampled_levels); ++level) {
            image_bytes += gx_texture_data_size(
                std::max<u16>(image.mWidth >> level, 1), std::max<u16>(image.mHeight >> level, 1),
                static_cast<TplTextureFormat>(format));
        }
        const auto image_offset = source + image.mImageDataOffset;
        check_range(block, image_offset, image_bytes);
        if ((image_offset & 31U) != 0) {
            aurora::throw_host_exception<std::runtime_error>("Texture payload is not aligned for an original BP address");
        }
        const bool indexed = format == GX_TF_C4 || format == GX_TF_C8 || format == GX_TF_C14X2;
        const bool loads_palette = image.mPaletteName == 1;
        const auto palette_offset = source + image.mPaletteDataOffset;
        std::size_t palette_bytes = image.mPaletteNum * 2U;
        if (loads_palette) {
            palette_bytes = std::max<std::size_t>(palette_bytes, image.mPaletteNum > 16 ? 512 : 32);
        }
        if ((indexed || loads_palette || image.mPaletteNum != 0) && image.mPaletteFormat > GX_TL_RGB5A3) {
            aurora::throw_host_exception<std::runtime_error>("Texture has an invalid GX palette format");
        }
        check_range(block, palette_offset, palette_bytes);
        if (palette_bytes != 0 && (palette_offset & 31U) != 0) {
            aurora::throw_host_exception<std::runtime_error>("Texture palette payload is not aligned for an original BP address");
        }
    }
}
