#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "AssetServices.hpp"

namespace smgpc::assets::layout::tpl {

struct DecodedImage {
    std::uint16_t width {};
    std::uint16_t height {};
    std::vector<std::uint8_t> rgba8 {};

    [[nodiscard]] bool empty() const {
        return width == 0U || height == 0U || rgba8.empty();
    }
};

[[nodiscard]] AssetResult<DecodedImage> decode_tpl_first_image(std::span<const std::byte> bytes);

[[nodiscard]] AssetResult<DecodedImage> decode_gx_tiled_texture(
    std::span<const std::byte> bytes,
    std::uint16_t width,
    std::uint16_t height,
    std::uint32_t format);

}  // namespace smgpc::assets::layout::tpl
