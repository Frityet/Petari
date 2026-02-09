#include "render/TplDecoder.hpp"

#include "core/Logger.hpp"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string>

namespace pcport {
namespace {

std::uint16_t ReadU16BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data.at(offset)) << 8U) | static_cast<std::uint16_t>(data.at(offset + 1U)));
}

std::uint32_t ReadU32BE(const std::vector<std::uint8_t>& data, std::size_t offset) {
    return (static_cast<std::uint32_t>(data.at(offset)) << 24U) | (static_cast<std::uint32_t>(data.at(offset + 1U)) << 16U) |
           (static_cast<std::uint32_t>(data.at(offset + 2U)) << 8U) | static_cast<std::uint32_t>(data.at(offset + 3U));
}

void SetPixel(ImageRGBA& image, int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    if (x < 0 || y < 0 || x >= image.width || y >= image.height) {
        return;
    }
    const std::size_t idx = static_cast<std::size_t>(y * image.width + x) * 4U;
    image.pixels[idx + 0U] = r;
    image.pixels[idx + 1U] = g;
    image.pixels[idx + 2U] = b;
    image.pixels[idx + 3U] = a;
}

std::uint8_t Expand4(std::uint8_t v) {
    return static_cast<std::uint8_t>((v << 4U) | v);
}

std::uint8_t Expand3(std::uint8_t v) {
    return static_cast<std::uint8_t>((v << 5U) | (v << 2U) | (v >> 1U));
}

std::uint8_t Expand5(std::uint8_t v) {
    return static_cast<std::uint8_t>((v << 3U) | (v >> 2U));
}

std::uint8_t Expand6(std::uint8_t v) {
    return static_cast<std::uint8_t>((v << 2U) | (v >> 4U));
}

void DecodeI4(const std::vector<std::uint8_t>& data, std::size_t dataOffset, ImageRGBA& out) {
    std::size_t cursor = dataOffset;
    for (int by = 0; by < out.height; by += 8) {
        for (int bx = 0; bx < out.width; bx += 8) {
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; x += 2) {
                    const std::uint8_t packed = data.at(cursor++);
                    const std::uint8_t hi = Expand4(static_cast<std::uint8_t>((packed >> 4U) & 0x0FU));
                    const std::uint8_t lo = Expand4(static_cast<std::uint8_t>(packed & 0x0FU));
                    SetPixel(out, bx + x + 0, by + y, 255, 255, 255, hi);
                    SetPixel(out, bx + x + 1, by + y, 255, 255, 255, lo);
                }
            }
        }
    }
}

void DecodeI8(const std::vector<std::uint8_t>& data, std::size_t dataOffset, ImageRGBA& out) {
    std::size_t cursor = dataOffset;
    for (int by = 0; by < out.height; by += 4) {
        for (int bx = 0; bx < out.width; bx += 8) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 8; ++x) {
                    const std::uint8_t i = data.at(cursor++);
                    SetPixel(out, bx + x, by + y, 255, 255, 255, i);
                }
            }
        }
    }
}

void DecodeIA4(const std::vector<std::uint8_t>& data, std::size_t dataOffset, ImageRGBA& out) {
    std::size_t cursor = dataOffset;
    for (int by = 0; by < out.height; by += 4) {
        for (int bx = 0; bx < out.width; bx += 8) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 8; ++x) {
                    const std::uint8_t v = data.at(cursor++);
                    const std::uint8_t a = Expand4(static_cast<std::uint8_t>((v >> 4U) & 0x0FU));
                    const std::uint8_t i = Expand4(static_cast<std::uint8_t>(v & 0x0FU));
                    SetPixel(out, bx + x, by + y, i, i, i, a);
                }
            }
        }
    }
}

void DecodeRGB565(const std::vector<std::uint8_t>& data, std::size_t dataOffset, ImageRGBA& out) {
    std::size_t cursor = dataOffset;
    for (int by = 0; by < out.height; by += 4) {
        for (int bx = 0; bx < out.width; bx += 4) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    const std::uint16_t p = static_cast<std::uint16_t>((data.at(cursor) << 8U) | data.at(cursor + 1U));
                    cursor += 2;
                    const std::uint8_t r = Expand5(static_cast<std::uint8_t>((p >> 11U) & 0x1FU));
                    const std::uint8_t g = Expand6(static_cast<std::uint8_t>((p >> 5U) & 0x3FU));
                    const std::uint8_t b = Expand5(static_cast<std::uint8_t>(p & 0x1FU));
                    SetPixel(out, bx + x, by + y, r, g, b, 255);
                }
            }
        }
    }
}

void DecodeRGB5A3(const std::vector<std::uint8_t>& data, std::size_t dataOffset, ImageRGBA& out) {
    std::size_t cursor = dataOffset;
    for (int by = 0; by < out.height; by += 4) {
        for (int bx = 0; bx < out.width; bx += 4) {
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    const std::uint16_t p = static_cast<std::uint16_t>((data.at(cursor) << 8U) | data.at(cursor + 1U));
                    cursor += 2;
                    std::uint8_t r = 0;
                    std::uint8_t g = 0;
                    std::uint8_t b = 0;
                    std::uint8_t a = 255;
                    if ((p & 0x8000U) != 0) {
                        r = Expand5(static_cast<std::uint8_t>((p >> 10U) & 0x1FU));
                        g = Expand5(static_cast<std::uint8_t>((p >> 5U) & 0x1FU));
                        b = Expand5(static_cast<std::uint8_t>(p & 0x1FU));
                    } else {
                        a = Expand3(static_cast<std::uint8_t>((p >> 12U) & 0x07U));
                        r = Expand4(static_cast<std::uint8_t>((p >> 8U) & 0x0FU));
                        g = Expand4(static_cast<std::uint8_t>((p >> 4U) & 0x0FU));
                        b = Expand4(static_cast<std::uint8_t>(p & 0x0FU));
                    }
                    SetPixel(out, bx + x, by + y, r, g, b, a);
                }
            }
        }
    }
}

}  // namespace

ImageRGBA TplDecoder::DecodeFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open TPL file: " + path.string());
    }

    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.size() < 12U) {
        throw std::runtime_error("TPL file too small: " + path.string());
    }

    const std::uint32_t magic = ReadU32BE(data, 0U);
    if (magic != 0x0020AF30U) {
        throw std::runtime_error("Invalid TPL magic: " + path.string());
    }

    const std::uint32_t textureCount = ReadU32BE(data, 4U);
    const std::uint32_t tableOffset = ReadU32BE(data, 8U);
    if (textureCount == 0U) {
        throw std::runtime_error("TPL has no textures: " + path.string());
    }

    const std::uint32_t imageHeaderOffset = ReadU32BE(data, tableOffset + 0U);
    const std::uint16_t height = ReadU16BE(data, imageHeaderOffset + 0U);
    const std::uint16_t width = ReadU16BE(data, imageHeaderOffset + 2U);
    const std::uint32_t format = ReadU32BE(data, imageHeaderOffset + 4U);
    const std::uint32_t imageDataOffset = ReadU32BE(data, imageHeaderOffset + 8U);

    ImageRGBA image;
    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.format = static_cast<TplTextureFormat>(format);
    image.pixels.assign(static_cast<std::size_t>(image.width * image.height * 4), 0);

    switch (format) {
    case 0:
        DecodeI4(data, imageDataOffset, image);
        break;
    case 1:
        DecodeI8(data, imageDataOffset, image);
        break;
    case 2:
        DecodeIA4(data, imageDataOffset, image);
        break;
    case 4:
        DecodeRGB565(data, imageDataOffset, image);
        break;
    case 5:
        DecodeRGB5A3(data, imageDataOffset, image);
        break;
    default:
        throw std::runtime_error("Unsupported TPL format " + std::to_string(format) + " in " + path.string());
    }

    Log(LogLevel::Debug, LogCategory::Assets,
        "Decoded TPL " + path.string() + " " + std::to_string(image.width) + "x" + std::to_string(image.height));

    return image;
}

}  // namespace pcport
