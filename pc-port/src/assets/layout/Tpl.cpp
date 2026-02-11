#include "Tpl.hpp"

#include <algorithm>
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
                    set_pixel(out, bx + x + 0, by + y, 255U, 255U, 255U, hi);
                    set_pixel(out, bx + x + 1, by + y, 255U, 255U, 255U, lo);
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
                    set_pixel(out, bx + x, by + y, 255U, 255U, 255U, intensity);
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
                    const auto alpha = binary::read_u8(data, cursor++);
                    const auto intensity = binary::read_u8(data, cursor++);
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
