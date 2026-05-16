#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace smgpc::render::capture {

enum class PixelFormat {
    RGBA8,
    BGRA8,
};

struct ScreenshotImageView {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint32_t pitch = 0U;
    std::span<const std::uint8_t> pixels = {};
    PixelFormat format = PixelFormat::RGBA8;
    bool origin_bottom_left = false;
};

class IScreenshotService {
public:
    virtual ~IScreenshotService() = default;

    virtual void write_png(const std::filesystem::path &path, const ScreenshotImageView &image) const = 0;
};

[[nodiscard]] std::unique_ptr<IScreenshotService> create_png_screenshot_service();

}  // namespace smgpc::render::capture
