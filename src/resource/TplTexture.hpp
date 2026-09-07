#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace smgpc::resource {

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

    struct GxTextureImageLevel {
        std::uint8_t level = 0U;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        std::uint32_t data_offset = 0U;
        std::uint32_t data_size = 0U;
    };

    struct TplTextureDescriptor {
        std::uint32_t index = 0U;
        std::uint32_t texture_header_offset = 0U;
        std::uint32_t clut_header_offset = 0U;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        TplTextureFormat format = TplTextureFormat::I4;
        std::uint32_t image_data_offset = 0U;
        std::uint32_t image_data_size = 0U;
        std::uint32_t wrap_s = 0U;
        std::uint32_t wrap_t = 0U;
        std::uint32_t min_filter = 0U;
        std::uint32_t mag_filter = 0U;
        float lod_bias = 0.0F;
        bool edge_lod_enable = false;
        std::uint8_t min_lod = 0U;
        std::uint8_t max_lod = 0U;
        bool texture_header_unpacked = false;
        bool has_palette = false;
        std::uint16_t palette_entry_count = 0U;
        std::uint32_t palette_format = 0U;
        std::uint32_t palette_data_offset = 0U;
        std::uint32_t palette_data_size = 0U;
        bool palette_header_unpacked = false;
        std::vector<GxTextureImageLevel> image_levels;
    };

    struct TplPalette {
        std::uint32_t version = 0U;
        std::uint32_t descriptor_count = 0U;
        std::uint32_t descriptor_array_offset = 0U;
        std::vector<TplTextureDescriptor> descriptors;
    };

    struct BtiTexture {
        TplTextureFormat format = TplTextureFormat::I4;
        std::uint8_t transparency = 0U;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        std::uint8_t wrap_s = 0U;
        std::uint8_t wrap_t = 0U;
        std::uint8_t palette_format = 0U;
        std::uint16_t palette_entry_count = 0U;
        std::uint32_t palette_data_offset = 0U;
        bool mipmap = false;
        bool do_edge_lod = false;
        bool bias_clamp = false;
        std::uint8_t max_anisotropy = 0U;
        std::uint8_t min_filter = 0U;
        std::uint8_t mag_filter = 0U;
        std::uint8_t min_lod = 0U;
        std::uint8_t max_lod = 0U;
        std::uint8_t image_count = 1U;
        std::int16_t lod_bias = 0;
        std::uint32_t image_data_offset = 0U;
        std::uint32_t image_data_size = 0U;
        std::vector<GxTextureImageLevel> image_levels;
        std::vector<std::uint8_t> image_data;
        DecodedTexture image;
    };

    [[nodiscard]] TplPalette read_tpl_palette(std::span<const std::uint8_t> data);
    [[nodiscard]] std::vector<TplTextureDescriptor> read_tpl_texture_descriptors(std::span<const std::uint8_t> data);
    [[nodiscard]] DecodedTexture decode_tpl_texture(std::span<const std::uint8_t> data, std::uint32_t descriptor_index = 0U);
    [[nodiscard]] BtiTexture decode_bti_texture(std::span<const std::uint8_t> data);
    [[nodiscard]] std::uint32_t gx_texture_data_size(std::uint16_t width, std::uint16_t height, TplTextureFormat format);
    [[nodiscard]] DecodedTexture decode_raw_gx_texture(std::span<const std::uint8_t> data, std::uint16_t width, std::uint16_t height, TplTextureFormat format);

}  // namespace smgpc::resource
