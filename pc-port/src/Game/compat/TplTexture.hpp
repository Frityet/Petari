#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace smgpc::game {

enum class TplTextureFormat : std::uint32_t {
    I4 = 0x0,
    I8 = 0x1,
    IA4 = 0x2,
    IA8 = 0x3,
    RGB565 = 0x4,
    RGB5A3 = 0x5,
    RGBA8 = 0x6,
    C4 = 0x8,
    C8 = 0x9,
    C14X2 = 0xA,
    CMPR = 0xE,
};

struct DecodedTexture {
    std::uint16_t width = 0U;
    std::uint16_t height = 0U;
    TplTextureFormat format = TplTextureFormat::I4;
    std::vector<std::uint8_t> rgba;
};

[[nodiscard]] DecodedTexture decode_tpl_texture(std::span<const std::uint8_t> data, std::uint32_t descriptor_index = 0U);
[[nodiscard]] DecodedTexture decode_raw_gx_texture(std::span<const std::uint8_t> data, std::uint16_t width, std::uint16_t height, TplTextureFormat format);

}  // namespace smgpc::game
