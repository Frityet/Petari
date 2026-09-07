#include <aurora/exception.hpp>
#include "J3dTextureData.hpp"
#include "J3dNameData.hpp"
#include "Mem1ResourceHeap.hpp"
#include "NativeTextureImage.hpp"

#include "JSystem/J3DGraphAnimator/J3DMaterialAttach.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"

#include <algorithm>
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
                aurora::throw_host_exception<std::runtime_error>("J3D texture range exceeds its TEX1 block");
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

    struct J3dTextureData::Storage {
        Mem1ResourceHeap::Allocation allocation;
        J3dNameData names;
        std::unique_ptr<J3DTexture> texture;
        Bytes source;
        bool attached = false;

        Storage(Bytes block, std::shared_ptr<Mem1ResourceHeap> heap) {
            if (!heap) aurora::throw_host_exception<std::invalid_argument>("J3D texture data requires a retained MEM1 heap");
            check_range(block, 0, 0x14);
            if (u32_at(block, 0) != 0x54455831U || u32_at(block, 4) != block.size()) {
                aurora::throw_host_exception<std::runtime_error>("J3D texture data requires one complete TEX1 block");
            }
            const auto count = u16_at(block, 8);
            const auto headers = u32_at(block, 0xC);
            if (count != 0 && headers == 0) aurora::throw_host_exception<std::runtime_error>("J3D TEX1 is missing its texture records");
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
                auto image = decode_texture_image_header(block.subspan(offset, sizeof(ResTIMG)));
                validate_texture_image(block, offset, image);
                images.push_back(image);
            }
            const std::size_t payload_start = count * sizeof(ResTIMG);
            if (block.size() > std::numeric_limits<std::uint32_t>::max() - payload_start) {
                aurora::throw_host_exception<std::length_error>("J3D TEX1 relative offsets exceed the original record range");
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
            aurora::throw_host_exception<std::logic_error>("J3D texture data can only attach once to an empty texture table");
        }
        table.mTexture = _storage->texture.get();
        table.mTextureName = _storage->names.table();
        _storage->attached = true;
    }
}
