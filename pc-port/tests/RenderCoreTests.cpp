#include "TestSuites.hpp"
#include "TestSupport.hpp"

namespace smgpc::tests {
    namespace {
        constexpr auto TEST_SUITE = std::string_view{"render/core"};

        template <int Line>
        struct TestCase;

        $test("writes valid PNG screenshots") {
            const std::array<std::uint8_t, 16U> pixels{
                255U,
                0U,
                0U,
                255U,
                0U,
                255U,
                0U,
                255U,
                0U,
                0U,
                255U,
                255U,
                255U,
                255U,
                255U,
                255U,
            };

            const auto output = std::filesystem::temp_directory_path() / "smg-pc-png-screenshot-service-test.png";
            const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();
            screenshot_service->write_png(output, smgpc::render::capture::ScreenshotImageView{
                                                      .width = 2U,
                                                      .height = 2U,
                                                      .pitch = 8U,
                                                      .pixels = std::span<const std::uint8_t>(pixels.data(), pixels.size()),
                                                      .format = smgpc::render::capture::PixelFormat::RGBA8,
                                                      .origin_bottom_left = false,
                                                  });

            const auto png = read_file(output);
            require(png.size() > 64U, "PNG service output is too small");
            require(png[0] == 0x89U && png[1] == 0x50U && png[2] == 0x4eU && png[3] == 0x47U, "PNG service output missing signature");
            require(read_be32(png, 8U) == 13U, "PNG service IHDR length mismatch");
            require(png[12] == 'I' && png[13] == 'H' && png[14] == 'D' && png[15] == 'R', "PNG service output missing IHDR");
            require(read_be32(png, 16U) == 2U, "PNG service width mismatch");
            require(read_be32(png, 20U) == 2U, "PNG service height mismatch");
            require(png[24] == 8U, "PNG service bit depth mismatch");
            require(png[25] == 6U, "PNG service color type mismatch");
        }

        $test("flushes async PNG screenshot writes") {
            const std::array<std::uint8_t, 16U> pixels{
                255U,
                255U,
                255U,
                255U,
                255U,
                0U,
                0U,
                255U,
                0U,
                255U,
                0U,
                255U,
                0U,
                0U,
                255U,
                255U,
            };

            const auto output = std::filesystem::temp_directory_path() / "smg-pc-async-png-screenshot-service-test.png";
            std::filesystem::remove(output);
            const auto screenshot_service = smgpc::render::capture::create_async_png_screenshot_service();
            screenshot_service->write_png(output, smgpc::render::capture::ScreenshotImageView{
                                                      .width = 2U,
                                                      .height = 2U,
                                                      .pitch = 8U,
                                                      .pixels = std::span<const std::uint8_t>(pixels.data(), pixels.size()),
                                                      .format = smgpc::render::capture::PixelFormat::RGBA8,
                                                      .origin_bottom_left = true,
                                                  });
            screenshot_service->flush();

            const auto png = read_file(output);
            require(png.size() > 64U, "async PNG service output is too small");
            require(png[0] == 0x89U && png[1] == 0x50U && png[2] == 0x4eU && png[3] == 0x47U, "async PNG service output missing signature");
            require(read_be32(png, 16U) == 2U, "async PNG service width mismatch");
            require(read_be32(png, 20U) == 2U, "async PNG service height mismatch");
        }

        $test("composes a single-texture J3D TEV material") {
            auto material = smgpc::compat::J3dMaterialSummary{};
            material.name = "synthetic-sky";
            material.material_colors[0U] = {149U, 195U, 165U, 255U};
            material.tev_k_colors[0U] = {0U, 28U, 43U, 255U};
            material.tev_stage_count = 1U;
            material.textures.push_back(smgpc::compat::J3dMaterialTextureBinding{
                .slot = 0U,
                .texture_index = 0U,
            });
            material.tev_orders.push_back(smgpc::compat::J3dTevOrderSummary{
                .stage = 0U,
                .tex_coord = 0U,
                .tex_map = 0U,
                .color_channel = 4U,
            });
            material.tev_stages.push_back(smgpc::compat::J3dTevStageSummary{
                .stage = 0U,
                .color_in = {14U, 10U, 8U, 15U},
                .color_op = 0U,
                .color_bias = 0U,
                .color_scale = 1U,
                .color_clamp = 1U,
                .color_out = 0U,
                .k_color_sel = 12U,
                .alpha_in = {7U, 4U, 5U, 7U},
                .alpha_op = 0U,
                .alpha_bias = 0U,
                .alpha_scale = 0U,
                .alpha_clamp = 1U,
                .alpha_out = 0U,
                .k_alpha_sel = 28U,
            });

            const auto source = smgpc::compat::DecodedTexture{
                .width = 2U,
                .height = 1U,
                .format = smgpc::compat::TplTextureFormat::I8,
                .rgba = {0U, 0U, 0U, 0U, 255U, 255U, 255U, 255U},
            };
            const auto composed = smgpc::compat::j3d_try_compose_material_texture(material, source, material.material_colors[0U], 0U);
            require(composed.has_value(), "single-texture J3D TEV material should compose");
            require(composed->raster_color_baked, "single-texture J3D TEV material should mark raster color as baked");
            require(composed->image.width == 2U && composed->image.height == 1U, "single-texture J3D TEV composition dimensions changed");
            require(composed->image.rgba[0U] == 0U && composed->image.rgba[1U] == 56U && composed->image.rgba[2U] == 86U &&
                        composed->image.rgba[3U] == 0U,
                    "single-texture J3D TEV composition did not apply konst-to-raster color ramp at texel 0");
            require(composed->image.rgba[4U] == 255U && composed->image.rgba[5U] == 255U && composed->image.rgba[6U] == 255U &&
                        composed->image.rgba[7U] == 255U,
                    "single-texture J3D TEV composition did not clamp raster color at texel 1");
        }

        $test("defines Wii logical render viewport dimensions") {
            require(smgpc::render::core::kWiiLogicalFramebufferWidth == 640U, "logical Wii framebuffer width should match Dolphin title captures");
            require(smgpc::render::core::kWiiLogicalFramebufferHeight == 456U, "logical Wii framebuffer height should match Dolphin title captures");
        }

        $test("keeps texture handle validity contract explicit") {
            const auto invalid = smgpc::render::TextureHandle{};
            require(!invalid.is_valid(), "default texture handles should be invalid");

            const auto zero = smgpc::render::TextureHandle{.value = 0U};
            require(zero.is_valid(), "texture handle value zero should be usable by render backends");

            const auto last_valid = smgpc::render::TextureHandle{.value = smgpc::render::TextureHandle::INVALID_VALUE - 1U};
            require(last_valid.is_valid(), "only the explicit invalid sentinel should be rejected");
        }

        $test("rejects invalid PNG screenshot image views") {
            const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();
            const auto output = std::filesystem::temp_directory_path() / "smg-pc-png-screenshot-invalid-test.png";
            const std::array<std::uint8_t, 8U> pixels{
                255U,
                0U,
                0U,
                255U,
                0U,
                255U,
                0U,
                255U,
            };

            auto rejected_zero_size = false;
            try {
                screenshot_service->write_png(output, smgpc::render::capture::ScreenshotImageView{
                                                          .width = 0U,
                                                          .height = 1U,
                                                          .pitch = 4U,
                                                          .pixels = std::span<const std::uint8_t>(pixels.data(), pixels.size()),
                                                          .format = smgpc::render::capture::PixelFormat::RGBA8,
                                                          .origin_bottom_left = false,
                                                      });
            } catch (const std::runtime_error &) {
                rejected_zero_size = true;
            }
            require(rejected_zero_size, "PNG screenshot service should reject zero-sized images");

            auto rejected_short_pitch = false;
            try {
                screenshot_service->write_png(output, smgpc::render::capture::ScreenshotImageView{
                                                          .width = 2U,
                                                          .height = 1U,
                                                          .pitch = 4U,
                                                          .pixels = std::span<const std::uint8_t>(pixels.data(), pixels.size()),
                                                          .format = smgpc::render::capture::PixelFormat::RGBA8,
                                                          .origin_bottom_left = false,
                                                      });
            } catch (const std::runtime_error &) {
                rejected_short_pitch = true;
            }
            require(rejected_short_pitch, "PNG screenshot service should reject pitches shorter than one row");
        }

    }  // namespace

    void run_render_core_tests() {
        run_registered_tests(TEST_SUITE);
    }

}  // namespace smgpc::tests
