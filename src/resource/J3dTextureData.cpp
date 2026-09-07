#include "J3dTextureData.hpp"
#include "J3dNameData.hpp"
#include "Mem1ResourceHeap.hpp"
#include "TplTexture.hpp"

#include "JSystem/J3DGraphAnimator/J3DMaterialAttach.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace smgpc::resource {
    namespace {
        using Bytes = std::span<const std::uint8_t>;
        static_assert(sizeof(ResTIMG) == 0x20);
        static_assert(alignof(ResTIMG) <= 32);

        void check_range(Bytes bytes, std::size_t offset, std::size_t count) {
            if (offset > bytes.size() || count > bytes.size() - offset) {
                throw std::runtime_error("J3D texture range exceeds its TEX1 block");
            }
        }
        std::uint16_t u16_at(Bytes bytes, std::size_t offset) {
            check_range(bytes, offset, 2);
            return static_cast<std::uint16_t>((std::uint16_t{bytes[offset]} << 8U) | bytes[offset + 1]);
        }
        std::uint32_t u32_at(Bytes bytes, std::size_t offset) {
            return (std::uint32_t{u16_at(bytes, offset)} << 16U) | u16_at(bytes, offset + 2);
        }

        ResTIMG decode_header(Bytes bytes) {
            check_range(bytes, 0, sizeof(ResTIMG));
            for (std::size_t offset : {0x10U, 0x11U, 0x12U}) {
                if (bytes[offset] > 1) throw std::runtime_error("J3D texture contains an invalid bool representation");
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

        void validate_header(Bytes block, std::size_t source, const ResTIMG& image) {
            if (image.mWidth == 0 || image.mWidth > 1024 || image.mHeight == 0 || image.mHeight > 1024 ||
                image.mWrapS > GX_MIRROR || image.mWrapT > GX_MIRROR || image.mMaxAnisotropy > GX_ANISO_4 ||
                image.mMinType > GX_LIN_MIP_LIN || image.mMagType > GX_LINEAR) {
                throw std::runtime_error("J3D texture has an invalid GX dimension or sampler field");
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
                throw std::runtime_error("J3D texture payload is not aligned for an original BP address");
            }
            const bool indexed = format == GX_TF_C4 || format == GX_TF_C8 || format == GX_TF_C14X2;
            const bool loads_palette = image.mPaletteName == 1;
            const auto palette_offset = source + image.mPaletteDataOffset;
            std::size_t palette_bytes = image.mPaletteNum * 2U;
            if (loads_palette) {
                palette_bytes = std::max<std::size_t>(palette_bytes, image.mPaletteNum > 16 ? 512 : 32);
            }
            if ((indexed || loads_palette || image.mPaletteNum != 0) && image.mPaletteFormat > GX_TL_RGB5A3) {
                throw std::runtime_error("J3D texture has an invalid GX palette format");
            }
            check_range(block, palette_offset, palette_bytes);
            if (palette_bytes != 0 && (palette_offset & 31U) != 0) {
                throw std::runtime_error("J3D palette payload is not aligned for an original BP address");
            }
        }
    }

    struct J3dTextureData::Storage {
        Mem1ResourceHeap::Allocation allocation;
        J3dNameData names;
        std::unique_ptr<J3DTexture> texture;
        Bytes source;
        bool attached = false;

        Storage(Bytes block, std::shared_ptr<Mem1ResourceHeap> heap) {
            if (!heap) throw std::invalid_argument("J3D texture data requires a retained MEM1 heap");
            check_range(block, 0, 0x14);
            if (u32_at(block, 0) != 0x54455831U || u32_at(block, 4) != block.size()) {
                throw std::runtime_error("J3D texture data requires one complete TEX1 block");
            }
            const auto count = u16_at(block, 8);
            const auto headers = u32_at(block, 0xC);
            if (count != 0 && headers == 0) throw std::runtime_error("J3D TEX1 is missing its texture records");
            check_range(block, headers, count * sizeof(ResTIMG));
            const auto name_offset = u32_at(block, 0x10);
            if (name_offset != 0) {
                check_range(block, name_offset, 4);
                names = J3dNameData(block.subspan(name_offset));
            }
            std::vector<ResTIMG> images;
            images.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                const auto offset = headers + i * sizeof(ResTIMG);
                auto image = decode_header(block.subspan(offset, sizeof(ResTIMG)));
                validate_header(block, offset, image);
                images.push_back(image);
            }
            const std::size_t payload_start = count * sizeof(ResTIMG);
            if (block.size() > std::numeric_limits<std::uint32_t>::max() - payload_start) {
                throw std::length_error("J3D TEX1 relative offsets exceed the original record range");
            }
            allocation = heap->allocate(payload_start + block.size());
            auto* storage = allocation.bytes().data();
            auto* records = reinterpret_cast<ResTIMG*>(storage);
            auto* raw = reinterpret_cast<std::uint8_t*>(storage + payload_start);
            std::memcpy(raw, block.data(), block.size());
            source = {raw, block.size()};
            for (std::size_t i = 0; i < count; ++i) {
                auto image = images[i];
                // Both arrays have the original 32-byte record stride. Every
                // rebase is forward into the one unchanged source copy, so no
                // 32-bit offset wraps and all authored aliases remain aliases.
                image.mImageDataOffset += static_cast<u32>(payload_start + headers);
                image.mPaletteDataOffset += static_cast<u32>(payload_start + headers);
                std::construct_at(records + i, image);
            }
            texture = std::make_unique<J3DTexture>(count, count != 0 ? records : nullptr);
        }
    };

    J3dTextureData::J3dTextureData(Bytes block, std::shared_ptr<Mem1ResourceHeap> heap)
        : _storage(std::make_unique<Storage>(block, std::move(heap))) {}
    J3dTextureData::~J3dTextureData() = default;
    J3dTextureData::J3dTextureData(J3dTextureData&&) noexcept = default;
    J3DTexture& J3dTextureData::texture() const noexcept { return *_storage->texture; }
    JUTNameTab* J3dTextureData::names() const noexcept { return _storage->names.table(); }
    Bytes J3dTextureData::source_bytes() const noexcept { return _storage->source; }
    void J3dTextureData::attach_to(J3DMaterialTable& table) {
        if (_storage->attached || table.mTexture != nullptr || table.mTextureName != nullptr) {
            throw std::logic_error("J3D texture data can only attach once to an empty texture table");
        }
        table.mTexture = _storage->texture.get();
        table.mTextureName = _storage->names.table();
        _storage->attached = true;
    }
}
