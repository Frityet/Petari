#include "Tpl.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "Binary.hpp"

namespace smgpc::assets::layout::tpl {
namespace {

enum : std::uint32_t {
    FORMAT_I4 = 0,
    FORMAT_I8 = 1,
    FORMAT_IA4 = 2,
    FORMAT_IA8 = 3,
    FORMAT_RGB565 = 4,
    FORMAT_RGB5A3 = 5,
    FORMAT_CMPR = 14,
};

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

[[nodiscard]] std::uint8_t expand4(std::uint8_t value) {
    return static_cast<std::uint8_t>((value << 4U) | value);
}

[[nodiscard]] std::uint8_t expand3(std::uint8_t value) {
    return static_cast<std::uint8_t>((value << 5U) | (value << 2U) | (value >> 1U));
}

[[nodiscard]] std::uint8_t expand5(std::uint8_t value) {
    return static_cast<std::uint8_t>((value << 3U) | (value >> 2U));
}

[[nodiscard]] std::uint8_t expand6(std::uint8_t value) {
    return static_cast<std::uint8_t>((value << 2U) | (value >> 4U));
}

void set_pixel(DecodedImage *image, int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    if (image == nullptr) {
        return;
    }

    if (x < 0 || y < 0 || x >= static_cast<int>(image->width) || y >= static_cast<int>(image->height)) {
        return;
    }

    const auto index = (static_cast<std::size_t>(y) * image->width + static_cast<std::size_t>(x)) * 4U;
    image->rgba8[index + 0U] = r;
    image->rgba8[index + 1U] = g;
    image->rgba8[index + 2U] = b;
    image->rgba8[index + 3U] = a;
}

void decode_i4(std::span<const std::byte> data, std::size_t data_offset, DecodedImage *out) {
    std::size_t cursor = data_offset;
    for (int by = 0; by < static_cast<int>(out->height); by += 8) {
        for (int bx = 0; bx < static_cast<int>(out->width); bx += 8) {
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; x += 2) {
                    const auto packed = binary::read_u8(data, cursor++);
                    const auto hi = expand4(static_cast<std::uint8_t>((packed >> 4U) & 0x0FU));
                    const auto lo = expand4(static_cast<std::uint8_t>(packed & 0x0FU));
                    set_pixel(out, bx + x + 0, by + y, hi, hi, hi, hi);
                    set_pixel(out, bx + x + 1, by + y, lo, lo, lo, lo);
                }
            }
        }
    }
}

void decode_i8(std::span<const std::byte> data, std::size_t data_offset, DecodedImage *out) {
    std::size_t cursor = data_offset;
    for (int by = 0; by < static_cast<int>(out->height); by += 4) {
        for (int bx = 0; bx < static_cast<int>(out->width); bx += 8) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 8; ++x) {
                    const auto intensity = binary::read_u8(data, cursor++);
                    set_pixel(out, bx + x, by + y, intensity, intensity, intensity, intensity);
                }
            }
        }
    }
}

void decode_ia4(std::span<const std::byte> data, std::size_t data_offset, DecodedImage *out) {
    std::size_t cursor = data_offset;
    for (int by = 0; by < static_cast<int>(out->height); by += 4) {
        for (int bx = 0; bx < static_cast<int>(out->width); bx += 8) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 8; ++x) {
                    const auto packed = binary::read_u8(data, cursor++);
                    const auto alpha = expand4(static_cast<std::uint8_t>((packed >> 4U) & 0x0FU));
                    const auto intensity = expand4(static_cast<std::uint8_t>(packed & 0x0FU));
                    set_pixel(out, bx + x, by + y, intensity, intensity, intensity, alpha);
                }
            }
        }
    }
}

void decode_ia8(std::span<const std::byte> data, std::size_t data_offset, DecodedImage *out) {
    std::size_t cursor = data_offset;
    for (int by = 0; by < static_cast<int>(out->height); by += 4) {
        for (int bx = 0; bx < static_cast<int>(out->width); bx += 4) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    const auto intensity = binary::read_u8(data, cursor++);
                    const auto alpha = binary::read_u8(data, cursor++);
                    set_pixel(out, bx + x, by + y, intensity, intensity, intensity, alpha);
                }
            }
        }
    }
}

void decode_rgb565(std::span<const std::byte> data, std::size_t data_offset, DecodedImage *out) {
    std::size_t cursor = data_offset;
    for (int by = 0; by < static_cast<int>(out->height); by += 4) {
        for (int bx = 0; bx < static_cast<int>(out->width); bx += 4) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    const auto packed = static_cast<std::uint16_t>((binary::read_u8(data, cursor) << 8U) |
                                                                    binary::read_u8(data, cursor + 1U));
                    cursor += 2;
                    const auto r = expand5(static_cast<std::uint8_t>((packed >> 11U) & 0x1FU));
                    const auto g = expand6(static_cast<std::uint8_t>((packed >> 5U) & 0x3FU));
                    const auto b = expand5(static_cast<std::uint8_t>(packed & 0x1FU));
                    set_pixel(out, bx + x, by + y, r, g, b, 255U);
                }
            }
        }
    }
}

void decode_rgb5a3(std::span<const std::byte> data, std::size_t data_offset, DecodedImage *out) {
    std::size_t cursor = data_offset;
    for (int by = 0; by < static_cast<int>(out->height); by += 4) {
        for (int bx = 0; bx < static_cast<int>(out->width); bx += 4) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    const auto packed = static_cast<std::uint16_t>((binary::read_u8(data, cursor) << 8U) |
                                                                    binary::read_u8(data, cursor + 1U));
                    cursor += 2;

                    std::uint8_t r = 0U;
                    std::uint8_t g = 0U;
                    std::uint8_t b = 0U;
                    std::uint8_t a = 255U;
                    if ((packed & 0x8000U) != 0U) {
                        r = expand5(static_cast<std::uint8_t>((packed >> 10U) & 0x1FU));
                        g = expand5(static_cast<std::uint8_t>((packed >> 5U) & 0x1FU));
                        b = expand5(static_cast<std::uint8_t>(packed & 0x1FU));
                    } else {
                        a = expand3(static_cast<std::uint8_t>((packed >> 12U) & 0x07U));
                        r = expand4(static_cast<std::uint8_t>((packed >> 8U) & 0x0FU));
                        g = expand4(static_cast<std::uint8_t>((packed >> 4U) & 0x0FU));
                        b = expand4(static_cast<std::uint8_t>(packed & 0x0FU));
                    }
                    set_pixel(out, bx + x, by + y, r, g, b, a);
                }
            }
        }
    }
}

struct RgbaColor {
    std::uint8_t r {};
    std::uint8_t g {};
    std::uint8_t b {};
    std::uint8_t a {255U};
};

[[nodiscard]] RgbaColor decode_rgb565_color(std::uint16_t packed) {
    return RgbaColor {
        .r = expand5(static_cast<std::uint8_t>((packed >> 11U) & 0x1FU)),
        .g = expand6(static_cast<std::uint8_t>((packed >> 5U) & 0x3FU)),
        .b = expand5(static_cast<std::uint8_t>(packed & 0x1FU)),
        .a = 255U,
    };
}

[[nodiscard]] RgbaColor lerp_color(RgbaColor a, RgbaColor b, int weight_a, int weight_b, int denominator) {
    return RgbaColor {
        .r = static_cast<std::uint8_t>((static_cast<int>(a.r) * weight_a + static_cast<int>(b.r) * weight_b) / denominator),
        .g = static_cast<std::uint8_t>((static_cast<int>(a.g) * weight_a + static_cast<int>(b.g) * weight_b) / denominator),
        .b = static_cast<std::uint8_t>((static_cast<int>(a.b) * weight_a + static_cast<int>(b.b) * weight_b) / denominator),
        .a = static_cast<std::uint8_t>((static_cast<int>(a.a) * weight_a + static_cast<int>(b.a) * weight_b) / denominator),
    };
}

void decode_cmpr_subblock(std::span<const std::byte> data, std::size_t cursor, int base_x, int base_y, DecodedImage *out) {
    const auto color0_raw = static_cast<std::uint16_t>((binary::read_u8(data, cursor) << 8U) | binary::read_u8(data, cursor + 1U));
    const auto color1_raw = static_cast<std::uint16_t>((binary::read_u8(data, cursor + 2U) << 8U) | binary::read_u8(data, cursor + 3U));
    const auto selectors = binary::read_u32_be(data, cursor + 4U);

    std::array<RgbaColor, 4> palette {};
    palette[0] = decode_rgb565_color(color0_raw);
    palette[1] = decode_rgb565_color(color1_raw);
    if (color0_raw > color1_raw) {
        palette[2] = lerp_color(palette[0], palette[1], 2, 1, 3);
        palette[3] = lerp_color(palette[0], palette[1], 1, 2, 3);
    } else {
        palette[2] = lerp_color(palette[0], palette[1], 1, 1, 2);
        palette[3] = RgbaColor {0U, 0U, 0U, 0U};
    }

    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            const auto selector_shift = static_cast<std::uint32_t>(30 - 2 * (y * 4 + x));
            const auto selector = static_cast<std::size_t>((selectors >> selector_shift) & 0x3U);
            const auto color = palette[selector];
            set_pixel(out, base_x + x, base_y + y, color.r, color.g, color.b, color.a);
        }
    }
}

void decode_cmpr(std::span<const std::byte> data, std::size_t data_offset, DecodedImage *out) {
    std::size_t cursor = data_offset;
    for (int by = 0; by < static_cast<int>(out->height); by += 8) {
        for (int bx = 0; bx < static_cast<int>(out->width); bx += 8) {
            decode_cmpr_subblock(data, cursor, bx + 0, by + 0, out);
            cursor += 8U;
            decode_cmpr_subblock(data, cursor, bx + 4, by + 0, out);
            cursor += 8U;
            decode_cmpr_subblock(data, cursor, bx + 0, by + 4, out);
            cursor += 8U;
            decode_cmpr_subblock(data, cursor, bx + 4, by + 4, out);
            cursor += 8U;
        }
    }
}

}  // namespace

AssetResult<DecodedImage> decode_gx_tiled_texture(
    std::span<const std::byte> bytes,
    std::uint16_t width,
    std::uint16_t height,
    std::uint32_t format) {
    DecodedImage image {};
    image.width = width;
    image.height = height;
    image.rgba8.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0U);

    const std::size_t data_offset = 0U;

    switch (format) {
    case FORMAT_I4:
        decode_i4(bytes, data_offset, &image);
        break;
    case FORMAT_I8:
        decode_i8(bytes, data_offset, &image);
        break;
    case FORMAT_IA4:
        decode_ia4(bytes, data_offset, &image);
        break;
    case FORMAT_IA8:
        decode_ia8(bytes, data_offset, &image);
        break;
    case FORMAT_RGB565:
        decode_rgb565(bytes, data_offset, &image);
        break;
    case FORMAT_RGB5A3:
        decode_rgb5a3(bytes, data_offset, &image);
        break;
    case FORMAT_CMPR:
        decode_cmpr(bytes, data_offset, &image);
        break;
    default:
        return make_error("Unsupported GX texture format.");
    }

    return image;
}

AssetResult<DecodedImage> decode_tpl_first_image(std::span<const std::byte> bytes) {
    using namespace binary;

    if (bytes.size() < 0x0CU) {
        return make_error("TPL file is too small.");
    }

    const auto magic = read_u32_be(bytes, 0U);
    if (magic != 0x0020AF30U) {
        return make_error("TPL magic mismatch.");
    }

    const auto texture_count = read_u32_be(bytes, 4U);
    const auto table_offset = static_cast<std::size_t>(read_u32_be(bytes, 8U));
    if (texture_count == 0U) {
        return make_error("TPL contains no textures.");
    }
    if (not has_bytes(bytes, table_offset, 8U)) {
        return make_error("TPL descriptor table exceeds file bounds.");
    }

    const auto image_header_offset = static_cast<std::size_t>(read_u32_be(bytes, table_offset + 0U));
    if (not has_bytes(bytes, image_header_offset, 0x20U)) {
        return make_error("TPL image header exceeds file bounds.");
    }

    const auto height = read_u16_be(bytes, image_header_offset + 0U);
    const auto width = read_u16_be(bytes, image_header_offset + 2U);
    const auto format = read_u32_be(bytes, image_header_offset + 4U);
    const auto image_data_offset = static_cast<std::size_t>(read_u32_be(bytes, image_header_offset + 8U));

    if (image_data_offset >= bytes.size()) {
        return make_error("TPL image data offset exceeds file bounds.");
    }

    const auto image_data = bytes.subspan(image_data_offset);
    return decode_gx_tiled_texture(image_data, width, height, format);
}

}  // namespace smgpc::assets::layout::tpl
