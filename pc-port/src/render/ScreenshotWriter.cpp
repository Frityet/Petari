#include "ScreenshotWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

namespace smgpc::render {

bool write_screenshot_png(const std::filesystem::path &output_path, std::uint32_t width, std::uint32_t height, std::uint32_t pitch, const void *data, bool yflip) {
    if (width == 0U || height == 0U || data == nullptr) {
        return false;
    }

    const std::uint32_t row_pitch = pitch == 0U ? (width * 4U) : pitch;
    if (row_pitch < width * 4U) {
        return false;
    }

    std::error_code filesystem_error {};
    if (output_path.has_parent_path()) {
        std::filesystem::create_directories(output_path.parent_path(), filesystem_error);
        if (filesystem_error) {
            return false;
        }
    }

    const auto *source_bytes = static_cast<const std::uint8_t *>(data);
    std::vector<std::uint8_t> rgba_pixels(static_cast<std::size_t>(width) * height * 4U);
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint32_t source_y = yflip ? (height - 1U - y) : y;
        const auto *source_row = source_bytes + static_cast<std::size_t>(source_y) * row_pitch;
        auto *destination_row = rgba_pixels.data() + static_cast<std::size_t>(y) * width * 4U;
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t source_offset = static_cast<std::size_t>(x) * 4U;
            const std::size_t destination_offset = static_cast<std::size_t>(x) * 4U;
            destination_row[destination_offset + 0U] = source_row[source_offset + 2U];
            destination_row[destination_offset + 1U] = source_row[source_offset + 1U];
            destination_row[destination_offset + 2U] = source_row[source_offset + 0U];
            destination_row[destination_offset + 3U] = source_row[source_offset + 3U];
        }
    }

    return stbi_write_png(output_path.string().c_str(), static_cast<int>(width), static_cast<int>(height), 4, rgba_pixels.data(), static_cast<int>(width * 4U)) != 0;
}

}  // namespace smgpc::render
