#include "TplTexture.hpp"

#include <array>
#include <optional>
#include <stdexcept>

namespace smgpc::game {
    namespace {

        enum class TlutFormat : std::uint32_t {
            IA8 = 0x0,
            RGB565 = 0x1,
            RGB5A3 = 0x2,
        };

        struct TplDescriptor {
            std::uint32_t texture_header_offset = 0U;
            std::uint32_t clut_header_offset = 0U;
        };

        struct TplHeader {
            std::uint16_t height = 0U;
            std::uint16_t width = 0U;
            TplTextureFormat format = TplTextureFormat::I4;
            std::uint32_t data_offset = 0U;
        };

        struct TplClutHeader {
            std::uint16_t entries = 0U;
            TlutFormat format = TlutFormat::IA8;
            std::uint32_t data_offset = 0U;
        };

        struct Color {
            std::uint8_t r = 0U;
            std::uint8_t g = 0U;
            std::uint8_t b = 0U;
            std::uint8_t a = 0U;
        };

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("TPL read past end of buffer");
            }

            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8U) | static_cast<std::uint16_t>(data[offset + 1U]));
        }

        [[nodiscard]] std::int16_t read_sbe16(std::span<const std::uint8_t> data, std::size_t offset) {
            const auto value = read_be16(data, offset);
            return value < 0x8000U ? static_cast<std::int16_t>(value) : static_cast<std::int16_t>(static_cast<int>(value) - 0x10000);
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("TPL read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) | (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
        }

        [[nodiscard]] std::uint8_t expand3(std::uint8_t value) {
            return static_cast<std::uint8_t>((value << 5U) | (value << 2U) | (value >> 1U));
        }

        [[nodiscard]] std::uint8_t expand4(std::uint8_t value) {
            return static_cast<std::uint8_t>((value << 4U) | value);
        }

        [[nodiscard]] std::uint8_t expand5(std::uint8_t value) {
            return static_cast<std::uint8_t>((value << 3U) | (value >> 2U));
        }

        [[nodiscard]] std::uint8_t expand6(std::uint8_t value) {
            return static_cast<std::uint8_t>((value << 2U) | (value >> 4U));
        }

        [[nodiscard]] Color rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
            return {.r = r, .g = g, .b = b, .a = a};
        }

        [[nodiscard]] Color decode_ia8(std::uint16_t value) {
            const auto intensity = static_cast<std::uint8_t>(value >> 8U);
            const auto alpha = static_cast<std::uint8_t>(value & 0xFFU);
            return rgba(intensity, intensity, intensity, alpha);
        }

        [[nodiscard]] Color decode_rgb565(std::uint16_t value) {
            return rgba(expand5(static_cast<std::uint8_t>((value >> 11U) & 0x1FU)), expand6(static_cast<std::uint8_t>((value >> 5U) & 0x3FU)), expand5(static_cast<std::uint8_t>(value & 0x1FU)), 0xFFU);
        }

        [[nodiscard]] Color decode_rgb5a3(std::uint16_t value) {
            if ((value & 0x8000U) != 0U) {
                return rgba(expand5(static_cast<std::uint8_t>((value >> 10U) & 0x1FU)), expand5(static_cast<std::uint8_t>((value >> 5U) & 0x1FU)), expand5(static_cast<std::uint8_t>(value & 0x1FU)), 0xFFU);
            }

            return rgba(expand4(static_cast<std::uint8_t>((value >> 8U) & 0xFU)), expand4(static_cast<std::uint8_t>((value >> 4U) & 0xFU)), expand4(static_cast<std::uint8_t>(value & 0xFU)), expand3(static_cast<std::uint8_t>((value >> 12U) & 0x7U)));
        }

        [[nodiscard]] std::uint32_t width_blocks(std::uint16_t width, std::uint8_t block_width);

        [[nodiscard]] Color blend_rgb(const Color &a, const Color &b, std::uint8_t a_weight, std::uint8_t b_weight, std::uint8_t divisor) {
            return rgba(
                static_cast<std::uint8_t>((static_cast<std::uint16_t>(a.r) * a_weight + static_cast<std::uint16_t>(b.r) * b_weight) / divisor),
                static_cast<std::uint8_t>((static_cast<std::uint16_t>(a.g) * a_weight + static_cast<std::uint16_t>(b.g) * b_weight) / divisor),
                static_cast<std::uint8_t>((static_cast<std::uint16_t>(a.b) * a_weight + static_cast<std::uint16_t>(b.b) * b_weight) / divisor),
                0xFFU);
        }

        [[nodiscard]] Color read_cmpr_texel(std::span<const std::uint8_t> texture, std::uint16_t width, std::uint16_t x, std::uint16_t y) {
            const auto block_x = x / 8U;
            const auto block_y = y / 8U;
            const auto sub_block_x = (x % 8U) / 4U;
            const auto sub_block_y = (y % 8U) / 4U;
            const auto in_block_x = x % 4U;
            const auto in_block_y = y % 4U;
            const auto block_offset = (block_y * width_blocks(width, 8U) + block_x) * 32U;
            const auto sub_block_offset = (sub_block_y * 2U + sub_block_x) * 8U;
            const auto offset = static_cast<std::size_t>(block_offset + sub_block_offset);

            const auto color0_value = read_be16(texture, offset);
            const auto color1_value = read_be16(texture, offset + 2U);
            const auto indices = read_be32(texture, offset + 4U);
            const auto color0 = decode_rgb565(color0_value);
            const auto color1 = decode_rgb565(color1_value);

            std::array<Color, 4U> colors{};
            colors[0] = color0;
            colors[1] = color1;
            if (color0_value > color1_value) {
                colors[2] = blend_rgb(color0, color1, 2U, 1U, 3U);
                colors[3] = blend_rgb(color0, color1, 1U, 2U, 3U);
            } else {
                colors[2] = blend_rgb(color0, color1, 1U, 1U, 2U);
                colors[3] = rgba(0U, 0U, 0U, 0U);
            }

            const auto pixel_index = in_block_y * 4U + in_block_x;
            const auto selector = static_cast<std::uint8_t>((indices >> (30U - pixel_index * 2U)) & 0x3U);
            return colors[selector];
        }

        [[nodiscard]] Color decode_palette(std::span<const std::uint8_t> texture_data, const TplClutHeader &clut, std::uint16_t index) {
            if (index >= clut.entries) {
                throw std::runtime_error("TPL palette index outside CLUT");
            }

            const auto value = read_be16(texture_data, clut.data_offset + static_cast<std::size_t>(index) * 2U);
            switch (clut.format) {
            case TlutFormat::IA8:
                return decode_ia8(value);
            case TlutFormat::RGB565:
                return decode_rgb565(value);
            case TlutFormat::RGB5A3:
                return decode_rgb5a3(value);
            }

            throw std::runtime_error("Unsupported TPL palette format");
        }

        [[nodiscard]] TplDescriptor read_descriptor(std::span<const std::uint8_t> data, std::uint32_t descriptor_index) {
            if (read_be32(data, 0U) != 0x0020AF30U) {
                throw std::runtime_error("TPL has unexpected version magic");
            }

            const auto descriptor_count = read_be32(data, 4U);
            const auto descriptor_array_offset = read_be32(data, 8U);
            if (descriptor_index >= descriptor_count) {
                throw std::runtime_error("TPL descriptor index outside palette");
            }

            const auto descriptor_offset = descriptor_array_offset + descriptor_index * 8U;
            return {
                .texture_header_offset = read_be32(data, descriptor_offset),
                .clut_header_offset = read_be32(data, descriptor_offset + 4U),
            };
        }

        [[nodiscard]] TplHeader read_texture_header(std::span<const std::uint8_t> data, std::uint32_t offset) {
            return {
                .height = read_be16(data, offset),
                .width = read_be16(data, offset + 2U),
                .format = static_cast<TplTextureFormat>(read_be32(data, offset + 4U)),
                .data_offset = read_be32(data, offset + 8U),
            };
        }

        [[nodiscard]] TplClutHeader read_clut_header(std::span<const std::uint8_t> data, std::uint32_t offset) {
            return {
                .entries = read_be16(data, offset),
                .format = static_cast<TlutFormat>(read_be32(data, offset + 4U)),
                .data_offset = read_be32(data, offset + 8U),
            };
        }

        [[nodiscard]] std::uint32_t width_blocks(std::uint16_t width, std::uint8_t block_width) {
            return (static_cast<std::uint32_t>(width - 1U) / block_width) + 1U;
        }

        [[nodiscard]] std::size_t tiled_block_offset(std::uint16_t width, std::uint16_t x, std::uint16_t y, std::uint8_t block_width, std::uint8_t block_height, std::uint8_t block_bytes) {
            const auto block_x = x / block_width;
            const auto block_y = y / block_height;
            return (block_y * width_blocks(width, block_width) + block_x) * block_bytes;
        }

        [[nodiscard]] std::size_t tiled_texel_offset(std::uint16_t x, std::uint16_t y, std::uint8_t block_width, std::uint8_t block_height) {
            const auto in_block_x = x % block_width;
            const auto in_block_y = y % block_height;
            return in_block_y * block_width + in_block_x;
        }

        [[nodiscard]] std::size_t tiled_byte_offset(std::uint16_t width, std::uint16_t x, std::uint16_t y, std::uint8_t block_width, std::uint8_t block_height, std::uint8_t block_bytes, std::uint8_t bytes_per_texel) {
            return tiled_block_offset(width, x, y, block_width, block_height, block_bytes) + tiled_texel_offset(x, y, block_width, block_height) * bytes_per_texel;
        }

        [[nodiscard]] bool is_palette_format(TplTextureFormat format) {
            return format == TplTextureFormat::C4 || format == TplTextureFormat::C8 || format == TplTextureFormat::C14X2;
        }

        [[nodiscard]] Color read_texel(std::span<const std::uint8_t> data, const TplHeader &header, const TplClutHeader *clut, std::uint16_t x, std::uint16_t y) {
            const auto texture = data.subspan(header.data_offset);

            switch (header.format) {
            case TplTextureFormat::I4: {
                const auto texel_offset = tiled_texel_offset(x, y, 8U, 8U);
                const auto byte_offset = tiled_block_offset(header.width, x, y, 8U, 8U, 32U) + texel_offset / 2U;
                const auto value = static_cast<std::uint8_t>((texture[byte_offset] >> ((texel_offset & 1U) == 0U ? 4U : 0U)) & 0xFU);
                const auto intensity = expand4(value);
                return rgba(intensity, intensity, intensity, intensity);
            }
            case TplTextureFormat::I8: {
                const auto offset = tiled_byte_offset(header.width, x, y, 8U, 4U, 32U, 1U);
                const auto intensity = texture[offset];
                return rgba(intensity, intensity, intensity, intensity);
            }
            case TplTextureFormat::IA4: {
                const auto offset = tiled_byte_offset(header.width, x, y, 8U, 4U, 32U, 1U);
                const auto value = texture[offset];
                const auto alpha = expand4(static_cast<std::uint8_t>(value >> 4U));
                const auto intensity = expand4(static_cast<std::uint8_t>(value & 0xFU));
                return rgba(intensity, intensity, intensity, alpha);
            }
            case TplTextureFormat::IA8: {
                const auto offset = tiled_byte_offset(header.width, x, y, 4U, 4U, 32U, 2U);
                return decode_ia8(read_be16(texture, offset));
            }
            case TplTextureFormat::RGB565: {
                const auto offset = tiled_byte_offset(header.width, x, y, 4U, 4U, 32U, 2U);
                return decode_rgb565(read_be16(texture, offset));
            }
            case TplTextureFormat::RGB5A3: {
                const auto offset = tiled_byte_offset(header.width, x, y, 4U, 4U, 32U, 2U);
                return decode_rgb5a3(read_be16(texture, offset));
            }
            case TplTextureFormat::RGBA8: {
                const auto base = (static_cast<std::size_t>(y / 4U) * width_blocks(header.width, 4U) + (x / 4U)) * 64U;
                const auto in_block = static_cast<std::size_t>(y % 4U) * 4U + (x % 4U);
                return rgba(texture[base + in_block * 2U + 1U], texture[base + 32U + in_block * 2U], texture[base + 32U + in_block * 2U + 1U], texture[base + in_block * 2U]);
            }
            case TplTextureFormat::C4: {
                if (clut == nullptr) {
                    throw std::runtime_error("TPL C4 texture missing CLUT");
                }
                const auto texel_offset = tiled_texel_offset(x, y, 8U, 8U);
                const auto byte_offset = tiled_block_offset(header.width, x, y, 8U, 8U, 32U) + texel_offset / 2U;
                const auto index = static_cast<std::uint16_t>((texture[byte_offset] >> ((texel_offset & 1U) == 0U ? 4U : 0U)) & 0xFU);
                return decode_palette(data, *clut, index);
            }
            case TplTextureFormat::C8: {
                if (clut == nullptr) {
                    throw std::runtime_error("TPL C8 texture missing CLUT");
                }
                const auto offset = tiled_byte_offset(header.width, x, y, 8U, 4U, 32U, 1U);
                return decode_palette(data, *clut, texture[offset]);
            }
            case TplTextureFormat::C14X2: {
                if (clut == nullptr) {
                    throw std::runtime_error("TPL C14X2 texture missing CLUT");
                }
                const auto offset = tiled_byte_offset(header.width, x, y, 4U, 4U, 32U, 2U);
                return decode_palette(data, *clut, static_cast<std::uint16_t>(read_be16(texture, offset) & 0x3FFFU));
            }
            case TplTextureFormat::CMPR:
                return read_cmpr_texel(texture, header.width, x, y);
            }

            throw std::runtime_error("Unsupported TPL texture format");
        }

        [[nodiscard]] DecodedTexture decode_texture(std::span<const std::uint8_t> data, const TplHeader &header, const TplClutHeader *clut) {
            auto texture = DecodedTexture{
                .width = header.width,
                .height = header.height,
                .format = header.format,
                .rgba = std::vector<std::uint8_t>(static_cast<std::size_t>(header.width) * header.height * 4U),
            };

            for (std::uint16_t y = 0U; y < header.height; ++y) {
                for (std::uint16_t x = 0U; x < header.width; ++x) {
                    const auto color = read_texel(data, header, clut, x, y);
                    const auto offset = (static_cast<std::size_t>(y) * header.width + x) * 4U;
                    texture.rgba[offset] = color.r;
                    texture.rgba[offset + 1U] = color.g;
                    texture.rgba[offset + 2U] = color.b;
                    texture.rgba[offset + 3U] = color.a;
                }
            }

            return texture;
        }

    }  // namespace

    DecodedTexture decode_tpl_texture(std::span<const std::uint8_t> data, std::uint32_t descriptor_index) {
        const auto descriptor = read_descriptor(data, descriptor_index);
        const auto header = read_texture_header(data, descriptor.texture_header_offset);
        const auto clut = descriptor.clut_header_offset == 0U ? std::optional<TplClutHeader>() : std::optional<TplClutHeader>(read_clut_header(data, descriptor.clut_header_offset));
        return decode_texture(data, header, clut.has_value() ? &*clut : nullptr);
    }

    BtiTexture decode_bti_texture(std::span<const std::uint8_t> data) {
        if (data.size() < 0x20U) {
            throw std::runtime_error("BTI texture header outside buffer");
        }

        auto texture = BtiTexture{
            .format = static_cast<TplTextureFormat>(data[0x00U]),
            .transparency = data[0x01U],
            .width = read_be16(data, 0x02U),
            .height = read_be16(data, 0x04U),
            .wrap_s = data[0x06U],
            .wrap_t = data[0x07U],
            .palette_format = data[0x08U],
            .palette_entry_count = read_be16(data, 0x0AU),
            .palette_data_offset = read_be32(data, 0x0CU),
            .mipmap = data[0x10U] != 0U,
            .do_edge_lod = data[0x11U] != 0U,
            .bias_clamp = data[0x12U] != 0U,
            .max_anisotropy = data[0x13U],
            .min_filter = data[0x14U],
            .mag_filter = data[0x15U],
            .min_lod = data[0x16U],
            .max_lod = data[0x17U],
            .image_count = data[0x18U],
            .lod_bias = read_sbe16(data, 0x1AU),
            .image_data_offset = read_be32(data, 0x1CU),
            .image = {},
        };

        if (texture.image_data_offset >= data.size()) {
            throw std::runtime_error("BTI image data outside buffer");
        }
        if (is_palette_format(texture.format) && texture.palette_entry_count == 0U) {
            throw std::runtime_error("BTI palette texture missing palette entries");
        }

        const auto header = TplHeader{
            .height = texture.height,
            .width = texture.width,
            .format = texture.format,
            .data_offset = texture.image_data_offset,
        };
        const auto clut = texture.palette_entry_count == 0U ?
                              std::optional<TplClutHeader>() :
                              std::optional<TplClutHeader>(TplClutHeader{
                                  .entries = texture.palette_entry_count,
                                  .format = static_cast<TlutFormat>(texture.palette_format),
                                  .data_offset = texture.palette_data_offset,
                              });
        texture.image = decode_texture(data, header, clut.has_value() ? &*clut : nullptr);
        return texture;
    }

    DecodedTexture decode_raw_gx_texture(std::span<const std::uint8_t> data, std::uint16_t width, std::uint16_t height, TplTextureFormat format) {
        const auto header = TplHeader{
            .height = height,
            .width = width,
            .format = format,
            .data_offset = 0U,
        };

        return decode_texture(data, header, nullptr);
    }

}  // namespace smgpc::game
