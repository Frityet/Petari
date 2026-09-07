#pragma once

#include "JSystem/JUtility/JUTTexture.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace smgpc::resource {
    // Shared BTI/TEX1 record boundary: native scalar fields, unchanged GX bytes.
    [[nodiscard]] ResTIMG decode_texture_image_header(std::span<const std::uint8_t>);
    void validate_texture_image(std::span<const std::uint8_t>, std::size_t record_offset, const ResTIMG&);
}
