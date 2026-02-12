#pragma once

#include <cstdint>
#include <filesystem>

namespace smgpc::render {

[[nodiscard]] bool write_screenshot_png(const std::filesystem::path &output_path, std::uint32_t width, std::uint32_t height, std::uint32_t pitch, const void *data, bool yflip);

}  // namespace smgpc::render
