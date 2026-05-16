#include "render/ScreenshotWriter.hpp"

#include <array>
#include <cstdint>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "tests/TestHarness.hpp"

namespace {

struct LoadedImage {
    int width {};
    int height {};
    int channels {};
    unsigned char *pixels {};

    ~LoadedImage() {
        stbi_image_free(pixels);
    }
};

[[nodiscard]] LoadedImage load_rgba(const std::filesystem::path &path) {
    LoadedImage image {};
    image.pixels = stbi_load(path.string().c_str(), &image.width, &image.height, &image.channels, 4);
    if (image.pixels == nullptr) {
        throw std::runtime_error("failed to load screenshot test image");
    }
    return image;
}

[[nodiscard]] std::array<std::uint8_t, 4> pixel_at(const LoadedImage &image, int x, int y) {
    const auto offset = static_cast<std::size_t>((y * image.width + x) * 4);
    return {
        image.pixels[offset + 0U],
        image.pixels[offset + 1U],
        image.pixels[offset + 2U],
        image.pixels[offset + 3U],
    };
}

}  // namespace

$test("Render::write_screenshot_png converts BGRA rows to opaque RGBA") {
    const auto output = std::filesystem::temp_directory_path() / "smgpc-screenshot-writer-test.png";
    const std::array<std::uint8_t, 16> bgra {
        30U, 20U, 10U, 255U,
        60U, 50U, 40U, 128U,
        90U, 80U, 70U, 64U,
        120U, 110U, 100U, 32U,
    };

    $pc_port_require(smgpc::render::write_screenshot_png(output, 2U, 2U, 0U, bgra.data(), false));

    const auto image = load_rgba(output);
    $pc_port_require_eq(image.width, 2);
    $pc_port_require_eq(image.height, 2);
    $pc_port_require(pixel_at(image, 0, 0) == (std::array<std::uint8_t, 4> {10U, 20U, 30U, 255U}));
    $pc_port_require(pixel_at(image, 1, 0) == (std::array<std::uint8_t, 4> {40U, 50U, 60U, 255U}));
    $pc_port_require(pixel_at(image, 0, 1) == (std::array<std::uint8_t, 4> {70U, 80U, 90U, 255U}));
    $pc_port_require(pixel_at(image, 1, 1) == (std::array<std::uint8_t, 4> {100U, 110U, 120U, 255U}));
}

$test("Render::write_screenshot_png can vertically flip BGRA input") {
    const auto output = std::filesystem::temp_directory_path() / "smgpc-screenshot-writer-yflip-test.png";
    const std::array<std::uint8_t, 16> bgra {
        3U, 2U, 1U, 255U,
        6U, 5U, 4U, 255U,
        9U, 8U, 7U, 255U,
        12U, 11U, 10U, 255U,
    };

    $pc_port_require(smgpc::render::write_screenshot_png(output, 2U, 2U, 0U, bgra.data(), true));

    const auto image = load_rgba(output);
    $pc_port_require(pixel_at(image, 0, 0) == (std::array<std::uint8_t, 4> {7U, 8U, 9U, 255U}));
    $pc_port_require(pixel_at(image, 1, 0) == (std::array<std::uint8_t, 4> {10U, 11U, 12U, 255U}));
    $pc_port_require(pixel_at(image, 0, 1) == (std::array<std::uint8_t, 4> {1U, 2U, 3U, 255U}));
    $pc_port_require(pixel_at(image, 1, 1) == (std::array<std::uint8_t, 4> {4U, 5U, 6U, 255U}));
}
