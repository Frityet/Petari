#include "BtiTextureData.hpp"
#include <aurora/exception.hpp>

#include <cstring>
#include <stdexcept>

namespace smgpc::resource {
    BtiTextureData::BtiTextureData(std::span<const std::uint8_t> bytes, std::shared_ptr<Mem1ResourceHeap> heap) {
        if (!heap) aurora::throw_host_exception<std::invalid_argument>("BTI texture requires a retained MEM1 heap");
        const auto header = decode_texture_image_header(bytes);
        validate_texture_image(bytes, 0, header);
        _allocation = heap->allocate(bytes.size());
        auto* storage = _allocation.bytes().data();
        std::memcpy(storage, bytes.data(), bytes.size());
        // The original relative offsets continue to address identical palette,
        // mip and GX tiled image bytes in this one aligned retained allocation.
        std::construct_at(reinterpret_cast<ResTIMG*>(storage), header);
    }

    const ResTIMG* BtiTextureData::image() const noexcept {
        return reinterpret_cast<const ResTIMG*>(_allocation.bytes().data());
    }
}
