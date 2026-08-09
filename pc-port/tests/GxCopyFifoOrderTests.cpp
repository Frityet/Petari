#include "RendererService.hpp"

#include <dolphin/gx.h>
#include <dolphin/gx/GXAurora.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    constexpr auto CopyWidth = std::uint16_t{640};
    constexpr auto CopyHeight = std::uint16_t{456};

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] smgpc::render::TexturedQuad2D full_frame_quad(std::array<std::uint8_t, 4> color, bool blend) {
        using smgpc::render::RenderSpace2D;
        using smgpc::render::TexturedQuad2D;
        using smgpc::render::TexturedVertex2D;
        return TexturedQuad2D{
            .vertices =
                {
                    TexturedVertex2D{.x = -320.0F, .y = -228.0F, .u = 0.0F, .v = 0.0F, .color = color},
                    TexturedVertex2D{.x = 320.0F, .y = -228.0F, .u = 1.0F, .v = 0.0F, .color = color},
                    TexturedVertex2D{.x = 320.0F, .y = 228.0F, .u = 1.0F, .v = 1.0F, .color = color},
                    TexturedVertex2D{.x = -320.0F, .y = 228.0F, .u = 0.0F, .v = 1.0F, .color = color},
                },
            .space = RenderSpace2D::CenteredFramebuffer,
            .min_filter = 0,
            .mag_filter = 0,
            .blend = blend,
        };
    }

    [[nodiscard]] std::array<std::uint8_t, 64U * 64U * 4U> quadrant_texture() {
        constexpr std::array colors{
            std::array<std::uint8_t, 4>{240, 16, 32, 255},
            std::array<std::uint8_t, 4>{24, 224, 48, 255},
            std::array<std::uint8_t, 4>{32, 48, 232, 255},
            std::array<std::uint8_t, 4>{240, 216, 24, 255},
        };
        auto pixels = std::array<std::uint8_t, 64U * 64U * 4U>{};
        for (auto y = std::size_t{}; y < 64; ++y) {
            for (auto x = std::size_t{}; x < 64; ++x) {
                const auto &color = colors[(y >= 32 ? 2U : 0U) + (x >= 32 ? 1U : 0U)];
                const auto offset = (y * 64U + x) * 4U;
                std::copy(color.begin(), color.end(), pixels.begin() + static_cast<std::ptrdiff_t>(offset));
            }
        }
        return pixels;
    }

    [[nodiscard]] std::vector<std::uint8_t> read_display_copy() {
        auto width = 0U;
        auto height = 0U;
        require(AuroraGetDisplayCopySize(&width, &height) == GX_TRUE && width == CopyWidth && height == CopyHeight,
                "GXCopyDisp must materialize the configured 640x456 display texture");
        auto stride = 0U;
        auto pixels = std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height * 4U);
        require(AuroraReadDisplayCopyRGBA8(pixels.data(), static_cast<u32>(pixels.size()), &width, &height, &stride) ==
                        GX_TRUE &&
                    stride == width * 4U,
                "the completed display copy must be readable as RGBA8 pixels");
        return pixels;
    }

    [[nodiscard]] std::span<const std::uint8_t, 4> pixel(std::span<const std::uint8_t> pixels, std::size_t x,
                                                         std::size_t y) {
        const auto offset = (y * CopyWidth + x) * 4U;
        return std::span<const std::uint8_t, 4>{pixels.data() + offset, 4};
    }

    [[nodiscard]] std::uint64_t fnv1a(std::span<const std::uint8_t> pixels) {
        auto hash = std::uint64_t{14695981039346656037ULL};
        for (const auto value : pixels) {
            hash ^= value;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    [[nodiscard]] std::size_t matching_pixels(std::span<const std::uint8_t> pixels,
                                              std::array<std::uint8_t, 3> expected) {
        auto count = std::size_t{};
        for (auto offset = std::size_t{}; offset + 3 < pixels.size(); offset += 4) {
            const auto close = [=](std::size_t channel) {
                const auto value = pixels[offset + channel];
                return value + 3U >= expected[channel] && value <= expected[channel] + 3U;
            };
            count += close(0) && close(1) && close(2);
        }
        return count;
    }

    void test_copy_boundaries() {
        auto window = smgpc::render::AuroraWindow({
            .width = CopyWidth,
            .height = CopyHeight,
            .title = "SMG PC GX copy FIFO-order proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        AuroraSetViewportPolicy(AURORA_VIEWPORT_NATIVE);
        const auto source_pixels = quadrant_texture();
        auto white_texture = smgpc::render::TextureHandle{};
        auto copy_texture = smgpc::render::TextureHandle{.texture = std::make_shared<smgpc::render::core::AuroraTexture>()};
        auto *const copy_record = copy_texture.texture.get();
        copy_record->width = CopyWidth;
        copy_record->height = CopyHeight;
        copy_record->format = GX_TF_RGBA8;
        copy_record->rgba.resize(GXGetTexBufferSize(CopyWidth, CopyHeight, GX_TF_RGBA8, GX_FALSE, 0));

        {
            const auto frame = renderer.begin_frame();
            (void)frame;
            const auto context = smgpc::render::ScopedAuroraRendererContext(renderer);
            const auto source_texture = renderer.create_rgba8_texture(64, 64, source_pixels);
            require(source_texture.is_valid(), "the copy-source pattern must own a real GX texture");
            renderer.submit_textured_quad(source_texture, full_frame_quad({255, 255, 255, 255}, false));

            GXSetTexCopySrc(0, 0, CopyWidth, CopyHeight);
            GXSetTexCopyDst(CopyWidth, CopyHeight, GX_TF_RGBA8, GX_FALSE);
            GXCopyTex(copy_record->rgba.data(), GX_TRUE);
            require(AuroraHasTextureCopy(copy_record->rgba.data()) == GX_TRUE,
                    "GXCopyTex must own a real GPU texture at the destination identity");

            GXInitTexObj(&copy_record->object, copy_record->rgba.data(), CopyWidth, CopyHeight, GX_TF_RGBA8, GX_CLAMP,
                         GX_CLAMP, GX_FALSE);
            GXInitTexObjLOD(&copy_record->object, GX_NEAR, GX_NEAR, 0.0F, 0.0F, 0.0F, GX_FALSE, GX_FALSE, GX_ANISO_1);
            copy_record->alive = true;
            renderer.submit_textured_quad(copy_texture, full_frame_quad({255, 255, 255, 255}, false));
            renderer.end_frame();
        }

        const auto copied_pixels = read_display_copy();
        constexpr std::array expected_colors{
            std::array<std::uint8_t, 3>{240, 16, 32},
            std::array<std::uint8_t, 3>{24, 224, 48},
            std::array<std::uint8_t, 3>{32, 48, 232},
            std::array<std::uint8_t, 3>{240, 216, 24},
        };
        for (const auto &expected : expected_colors) {
            require(matching_pixels(copied_pixels, expected) > 60000U,
                    "the first GXCopyTex must preserve every colored quadrant before it is sampled for GXCopyDisp");
        }
        std::cout << "[info] first GXCopyTex->GXCopyDisp pixels fnv1a=0x" << std::hex << fnv1a(copied_pixels)
                  << std::dec << '\n';

        {
            const auto frame = renderer.begin_frame();
            (void)frame;
            const auto context = smgpc::render::ScopedAuroraRendererContext(renderer);
            constexpr auto white = std::array<std::uint8_t, 4>{255, 255, 255, 255};
            white_texture = renderer.create_rgba8_texture(1, 1, white);
            require(white_texture.is_valid(), "the FIFO blend proof must own a real white texture");
            renderer.submit_textured_quad(white_texture, full_frame_quad({255, 0, 0, 128}, true));
            renderer.submit_textured_quad(white_texture, full_frame_quad({0, 255, 0, 128}, true));
            renderer.end_frame();
        }

        const auto ordered_pixels = read_display_copy();
        const auto center = pixel(ordered_pixels, CopyWidth / 2U, CopyHeight / 2U);
        require(center[0] >= 55U && center[0] <= 75U && center[1] >= 120U && center[1] <= 140U && center[2] <= 3U,
                "GXCopyDisp must execute the queued red-then-green blend exactly once and in FIFO order");
        std::cout << "[info] first GXCopyDisp ordered center=" << static_cast<unsigned>(center[0]) << ','
                  << static_cast<unsigned>(center[1]) << ',' << static_cast<unsigned>(center[2]) << ','
                  << static_cast<unsigned>(center[3]) << " fnv1a=0x" << std::hex << fnv1a(ordered_pixels) << std::dec
                  << '\n';

        const auto *copy_identity = copy_record->rgba.data();
        {
            const auto frame = renderer.begin_frame();
            (void)frame;
            const auto context = smgpc::render::ScopedAuroraRendererContext(renderer);
            GXDestroyTexObj(&copy_record->object);
            GXDestroyCopyTex(copy_record->rgba.data());
            copy_record->alive = false;
            renderer.end_frame();
        }
        require(AuroraHasTextureCopy(copy_identity) == GX_FALSE,
                "destroying the copy destination must release its generalized GPU ownership");
    }

}  // namespace

int main() {
    try {
        test_copy_boundaries();
        std::cout << "[ok] GX copy FIFO ordering and cold-pipeline completion\n";
        return 0;
    } catch (const std::exception &exception) {
        std::cerr << "[fail] GX copy FIFO ordering and cold-pipeline completion: " << exception.what() << '\n';
        return 1;
    }
}
