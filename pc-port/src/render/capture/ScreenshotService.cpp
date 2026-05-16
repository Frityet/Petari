#include "capture/ScreenshotService.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace smgpc::render::capture {
namespace {

constexpr std::array<std::uint8_t, 8U> kPngSignature {
    0x89U, 0x50U, 0x4eU, 0x47U, 0x0dU, 0x0aU, 0x1aU, 0x0aU,
};

void append_be32(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void append_le16(std::vector<std::uint8_t> &bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

[[nodiscard]] std::uint32_t update_crc32(std::uint32_t crc, std::span<const std::uint8_t> bytes) {
    auto value = crc;
    for (const auto byte : bytes) {
        value ^= byte;
        for (auto bit = 0U; bit < 8U; ++bit) {
            value = (value >> 1U) ^ (0xedb88320U & (0U - (value & 1U)));
        }
    }

    return value;
}

[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> type, std::span<const std::uint8_t> data) {
    auto value = 0xffffffffU;
    value = update_crc32(value, type);
    value = update_crc32(value, data);
    return value ^ 0xffffffffU;
}

[[nodiscard]] std::uint32_t adler32(std::span<const std::uint8_t> bytes) {
    constexpr auto kMod = 65521U;
    auto a = 1U;
    auto b = 0U;

    for (const auto byte : bytes) {
        a = (a + byte) % kMod;
        b = (b + a) % kMod;
    }

    return (b << 16U) | a;
}

void append_chunk(std::vector<std::uint8_t> &png, const std::array<std::uint8_t, 4U> &type, std::span<const std::uint8_t> data) {
    if (data.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("PNG chunk too large");
    }

    append_be32(png, static_cast<std::uint32_t>(data.size()));
    png.insert(png.end(), type.begin(), type.end());
    png.insert(png.end(), data.begin(), data.end());
    append_be32(png, crc32(type, data));
}

[[nodiscard]] std::vector<std::uint8_t> make_filtered_rgba_rows(const ScreenshotImageView &image) {
    if (image.width == 0U || image.height == 0U) {
        throw std::runtime_error("Cannot write a zero-sized PNG");
    }

    const auto row_bytes = image.width * 4U;
    const auto pitch = image.pitch == 0U ? row_bytes : image.pitch;
    if (pitch < row_bytes) {
        throw std::runtime_error("Screenshot pitch is shorter than one RGBA row");
    }

    const auto required_size = static_cast<std::size_t>(pitch) * static_cast<std::size_t>(image.height - 1U) + row_bytes;
    if (image.pixels.size() < required_size) {
        throw std::runtime_error("Screenshot pixel buffer is shorter than the declared dimensions");
    }

    auto filtered = std::vector<std::uint8_t> {};
    filtered.reserve((static_cast<std::size_t>(row_bytes) + 1U) * image.height);

    for (auto y = 0U; y < image.height; ++y) {
        const auto source_y = image.origin_bottom_left ? (image.height - 1U - y) : y;
        const auto row_offset = static_cast<std::size_t>(source_y) * pitch;
        filtered.push_back(0U);

        for (auto x = 0U; x < image.width; ++x) {
            const auto pixel_offset = row_offset + static_cast<std::size_t>(x) * 4U;
            switch (image.format) {
            case PixelFormat::RGBA8:
                filtered.push_back(image.pixels[pixel_offset]);
                filtered.push_back(image.pixels[pixel_offset + 1U]);
                filtered.push_back(image.pixels[pixel_offset + 2U]);
                filtered.push_back(image.pixels[pixel_offset + 3U]);
                break;
            case PixelFormat::BGRA8:
                filtered.push_back(image.pixels[pixel_offset + 2U]);
                filtered.push_back(image.pixels[pixel_offset + 1U]);
                filtered.push_back(image.pixels[pixel_offset]);
                filtered.push_back(image.pixels[pixel_offset + 3U]);
                break;
            }
        }
    }

    return filtered;
}

[[nodiscard]] std::vector<std::uint8_t> make_zlib_stored_stream(std::span<const std::uint8_t> payload) {
    auto stream = std::vector<std::uint8_t> {};
    stream.reserve(payload.size() + (payload.size() / 65535U + 1U) * 5U + 6U);

    stream.push_back(0x78U);
    stream.push_back(0x01U);

    auto offset = std::size_t {};
    while (offset < payload.size()) {
        const auto remaining = payload.size() - offset;
        const auto block_size = static_cast<std::uint16_t>(std::min<std::size_t>(remaining, 65535U));
        const auto final_block = (offset + block_size) == payload.size();

        stream.push_back(final_block ? 0x01U : 0x00U);
        append_le16(stream, block_size);
        append_le16(stream, static_cast<std::uint16_t>(~block_size));
        stream.insert(stream.end(), payload.begin() + static_cast<std::ptrdiff_t>(offset), payload.begin() + static_cast<std::ptrdiff_t>(offset + block_size));
        offset += block_size;
    }

    append_be32(stream, adler32(payload));
    return stream;
}

[[nodiscard]] std::vector<std::uint8_t> encode_png_rgba(const ScreenshotImageView &image) {
    auto ihdr = std::vector<std::uint8_t> {};
    ihdr.reserve(13U);
    append_be32(ihdr, image.width);
    append_be32(ihdr, image.height);
    ihdr.push_back(8U);
    ihdr.push_back(6U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);

    auto filtered = make_filtered_rgba_rows(image);
    auto idat = make_zlib_stored_stream(filtered);

    auto png = std::vector<std::uint8_t> {};
    png.reserve(kPngSignature.size() + ihdr.size() + idat.size() + 64U);
    png.insert(png.end(), kPngSignature.begin(), kPngSignature.end());
    append_chunk(png, {'I', 'H', 'D', 'R'}, ihdr);
    append_chunk(png, {'I', 'D', 'A', 'T'}, idat);
    append_chunk(png, {'I', 'E', 'N', 'D'}, {});
    return png;
}

class PngScreenshotService final : public IScreenshotService {
public:
    void write_png(const std::filesystem::path &path, const ScreenshotImageView &image) const override {
        const auto png = encode_png_rgba(image);

        const auto parent = path.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        auto file = std::ofstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot write PNG screenshot: " + path.string());
        }

        file.write(reinterpret_cast<const char *>(png.data()), static_cast<std::streamsize>(png.size()));
        if (!file) {
            throw std::runtime_error("Failed while writing PNG screenshot: " + path.string());
        }
    }
};

}  // namespace

std::unique_ptr<IScreenshotService> create_png_screenshot_service() {
    return std::make_unique<PngScreenshotService>();
}

}  // namespace smgpc::render::capture
