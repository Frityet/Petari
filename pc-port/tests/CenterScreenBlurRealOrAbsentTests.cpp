#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/FullScreenBlur.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "compat/CapturedFrameBlurService.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <dolphin/gx/GXAurora.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Function>
    void require_logic_error(Function&& function, std::string_view expected_text,
                             std::string_view message) {
        try {
            function();
        } catch (const std::logic_error& error) {
            require(std::string_view(error.what()).find(expected_text) !=
                        std::string_view::npos,
                    message);
            return;
        }
        throw std::runtime_error(std::string(message));
    }

    struct ExactCenterFixture {
        ExactCenterFixture()
            : dvd("/"), placements(), demo(dvd, placements), holder(), binding(holder) {
            MR::createCenterScreenBlur();
            blur = dynamic_cast<CenterScreenBlur*>(
                holder.getObj(SceneObj_CenterScreenBlur));
        }

        smgpc::runtime::DvdFileSystemService dvd;
        std::array<smgpc::scene::StagePlacementObject, 0U> placements;
        smgpc::compat::DemoSceneRuntime demo;
        SceneObjHolder holder;
        smgpc::scene::SceneObjHolderBinding binding;
        CenterScreenBlur* blur = nullptr;
    };

    void test_absent_without_scene_ownership() {
        require(smgpc::scene::current_captured_frame_blur_service() == nullptr,
                "a process without a scene must not synthesize blur history state");
        require_logic_error(
            [] { MR::createCenterScreenBlur(); }, "scene-owned SceneObjHolder",
            "CenterScreenBlur creation must stop at the missing scene boundary");
        require_logic_error(
            [] { MR::startCenterScreenBlur(12, 30.0F, 160U, 3, 3); },
            "must be created",
            "CenterScreenBlur start must not manufacture a process-global actor");
        require_logic_error(
            [] { MR::drawFullScreenBlur(15.0F, 15.0F, 128U, 64U); },
            "scene-owned captured-frame blur service",
            "the draw helper must not fall back to an event or a no-op");
    }

    void test_exact_scene_object_and_generalized_history_owner() {
        auto fixture = ExactCenterFixture{};
        auto* service =
            smgpc::scene::current_captured_frame_blur_service();

        require(fixture.blur != nullptr && fixture.blur->isDead(),
                "SceneObj 0x2D must synchronously initialize the exact dead actor");
        require(MR::createSceneObj(SceneObj_CenterScreenBlur) == fixture.blur,
                "SceneObj 0x2D must retain a single scene-owned exact actor");
        require(fixture.demo.simple_cast_registration_count(fixture.blur) == 1U,
                "exact init must retain the retail simple demo-cast registration");
        require(service != nullptr && service->history_width() == 128U &&
                    service->history_height() == 64U &&
                    service->history_texture() == nullptr &&
                    service->stats().draw_count == 0U &&
                    service->stats().history_capture_count == 0U &&
                    !service->stats().history_valid,
                "the scene must own one lazy 128x64 real history target");
    }

    void test_exact_nerve_lifecycle_and_strict_capture_boundary() {
        auto fixture = ExactCenterFixture{};
        auto* blur = fixture.blur;
        require(blur != nullptr, "the fixture must own the exact blur actor");

        MR::startCenterScreenBlur(6, 30.0F, 180U, 2, 2);
        require(!blur->isDead() && blur->mTime == 6 && blur->mFadeIn == 2 &&
                    blur->mFadeOut == 2 && blur->mOffset == 30.0F &&
                    blur->mAlpha == 180U && blur->mBlendRate == 0.0F,
                "start must preserve the exact retail parameter and appear contract");

        require_logic_error(
            [blur] { blur->draw(); }, "real CaptureScreenDirector texture",
            "an appeared exact actor must fail at the real capture dependency instead of drawing a substitute");

        blur->updateNerve();
        require(!blur->isDead() && blur->mBlendRate == 0.0F,
                "the exact FadeIn nerve must begin at zero blend");
        blur->updateNerve();
        require(std::fabs(blur->mBlendRate - 0.5F) < 0.00001F,
                "the exact FadeIn nerve must use the generalized linear nerve rate");

        for (auto frame = 0; frame < 16 && !blur->isDead(); ++frame) {
            blur->updateNerve();
        }
        require(blur->isDead(),
                "the exact FadeIn/Keep/FadeOut sequence must kill itself at completion");
    }

    void test_aurora_capability_queries_are_real_and_absent_outside_a_frame() {
        const auto identity = std::array<unsigned char, 32U>{};
        require(AuroraIsFrameActive() == GX_FALSE,
                "Aurora must report no GPU frame when no renderer frame is active");
        require(AuroraHasTextureCopy(identity.data()) == GX_FALSE,
                "an arbitrary host address must not be reported as a completed GXCopyTex");
    }

    [[nodiscard]] std::vector<std::uint8_t> read_display_copy() {
        auto width = 0U;
        auto height = 0U;
        require(AuroraGetDisplayCopySize(&width, &height) == GX_TRUE &&
                    width >= 640U && height >= 456U,
                "the GPU proof requires a materialized Aurora display copy");

        auto row_stride = 0U;
        auto pixels =
            std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height * 4U);
        require(AuroraReadDisplayCopyRGBA8(pixels.data(),
                                           static_cast<u32>(pixels.size()),
                                           &width, &height, &row_stride) == GX_TRUE &&
                    row_stride == width * 4U,
                "Aurora must return the real display-copy pixels");
        return pixels;
    }

    [[nodiscard]] smgpc::render::TexturedQuad2D full_frame_quad() {
        using smgpc::render::RenderSpace2D;
        using smgpc::render::TexturedQuad2D;
        using smgpc::render::TexturedVertex2D;
        return TexturedQuad2D{
            .vertices =
                {
                    TexturedVertex2D{.x = -320.0F, .y = -228.0F, .u = 0.0F,
                                     .v = 0.0F},
                    TexturedVertex2D{.x = 320.0F, .y = -228.0F, .u = 1.0F,
                                     .v = 0.0F},
                    TexturedVertex2D{.x = 320.0F, .y = 228.0F, .u = 1.0F,
                                     .v = 1.0F},
                    TexturedVertex2D{.x = -320.0F, .y = 228.0F, .u = 0.0F,
                                     .v = 1.0F},
                },
            .space = RenderSpace2D::CenteredFramebuffer,
            .blend = false,
        };
    }

    [[nodiscard]] std::array<std::uint8_t, 64U * 64U * 4U>
    make_checkerboard() {
        auto pixels = std::array<std::uint8_t, 64U * 64U * 4U>{};
        for (auto y = 0U; y < 64U; ++y) {
            for (auto x = 0U; x < 64U; ++x) {
                const auto index = static_cast<std::size_t>(y * 64U + x) * 4U;
                const auto marker = x >= 7U && x < 23U && y >= 11U && y < 29U;
                pixels[index + 0U] = marker ? 248U : static_cast<std::uint8_t>(x * 4U);
                pixels[index + 1U] = marker ? 24U : static_cast<std::uint8_t>(y * 4U);
                pixels[index + 2U] = marker
                                         ? 216U
                                         : static_cast<std::uint8_t>((x * 3U + y * 5U) & 0xffU);
                pixels[index + 3U] = 255U;
            }
        }
        return pixels;
    }

    void test_real_gpu_capture_blur_when_requested() {
        const auto* requested = std::getenv("SMGPC_BLUR_GPU_TEST");
        if (requested == nullptr || std::string_view(requested) != "1") {
            std::cout << "[skip] real GPU blur proof (set SMGPC_BLUR_GPU_TEST=1)\n";
            return;
        }

        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 456,
            .title = "SMG PC captured-frame blur proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
        auto fixture = std::make_unique<ExactCenterFixture>();
        const auto checkerboard = make_checkerboard();
        auto checker_texture = smgpc::render::TextureHandle{};
        const auto camera = smgpc::camera::CameraPose{
            .eye = {0.0F, 0.0F, 1000.0F},
            .watch = {0.0F, 0.0F, 0.0F},
        };

        {
            const auto frame = renderer.begin_frame();
            require(AuroraIsFrameActive() == GX_TRUE,
                    "renderer.begin_frame must expose a real Aurora GX frame");
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
            checker_texture = renderer.create_rgba8_texture(64U, 64U, checkerboard);
            require(checker_texture.is_valid(),
                    "the GPU proof checkerboard must be a real texture");
            renderer.submit_textured_quad(checker_texture, full_frame_quad());
            runtime.draw_3d_normal(camera);
            require(AuroraHasTextureCopy(MR::getScreenTexImage()) == GX_TRUE,
                    "CaptureScreenIndirect must materialize its real GXCopyTex before blur starts");
            renderer.end_frame();
        }
        const auto baseline = read_display_copy();
        if (const auto* screenshot = std::getenv("SMGPC_BLUR_SCREENSHOT");
            screenshot != nullptr && screenshot[0] != '\0') {
            auto baseline_path = std::filesystem::path(screenshot);
            baseline_path.replace_filename("baseline-frame.png");
            renderer.request_screenshot_png(baseline_path);
            std::cout << "[info] baseline screenshot=" << baseline_path.string()
                      << '\n';
        }

        MR::startCenterScreenBlur(30, 60.0F, 224U, 0, 8);
        void* history_image = nullptr;
        {
            const auto frame = renderer.begin_frame();
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
            renderer.submit_textured_quad(checker_texture, full_frame_quad());
            runtime.draw_3d_normal(camera);
            auto* service =
                smgpc::scene::current_captured_frame_blur_service();
            require(service != nullptr && service->stats().draw_count == 1U &&
                        service->stats().history_capture_count == 1U &&
                        service->stats().history_valid &&
                        service->history_texture() != nullptr &&
                        AuroraHasTextureCopy(service->history_texture()->mImage) ==
                            GX_TRUE,
                    "the visible pass must also resolve a real 128x64 GPU history capture");
            history_image = service->history_texture()->mImage;
            renderer.end_frame();
        }

        {
            const auto frame = renderer.begin_frame();
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
            renderer.submit_textured_quad(checker_texture, full_frame_quad());
            runtime.draw_3d_normal(camera);
            auto* service =
                smgpc::scene::current_captured_frame_blur_service();
            require(service != nullptr && service->stats().draw_count == 2U &&
                        service->stats().history_capture_count == 2U &&
                        service->stats().history_valid &&
                        service->history_texture() != nullptr &&
                        AuroraHasTextureCopy(service->history_texture()->mImage) ==
                            GX_TRUE,
                    "the second visible pass must sample and refresh the real GPU history texture");
            renderer.end_frame();
        }
        const auto blurred = read_display_copy();
        require(baseline.size() == blurred.size(),
                "baseline and blur readbacks must have the same display dimensions");
        const auto changed = std::inner_product(
            baseline.begin(), baseline.end(), blurred.begin(), std::size_t{0U},
            std::plus<>{}, [](std::uint8_t lhs, std::uint8_t rhs) {
                return lhs != rhs ? std::size_t{1U} : std::size_t{0U};
            });
        if (const auto* screenshot = std::getenv("SMGPC_BLUR_SCREENSHOT");
            screenshot != nullptr && screenshot[0] != '\0') {
            renderer.request_screenshot_png(std::filesystem::path(screenshot));
            std::cout << "[info] blur screenshot=" << screenshot << '\n';
        }
        std::cout << "[info] real GPU blur changed " << changed << " / "
                  << baseline.size() << " RGBA bytes\n";
        require(changed > baseline.size() / 100U,
                "the real blur draw must materially change the captured framebuffer");

        require(history_image != nullptr,
                "the GPU proof must retain the history texture identity for teardown validation");
        fixture.reset();
        {
            const auto frame = renderer.begin_frame();
            const auto renderer_context =
                smgpc::render::ScopedAuroraRendererContext(renderer);
            runtime.begin_frame(frame);
            renderer.end_frame();
        }
        require(AuroraHasTextureCopy(history_image) == GX_FALSE,
                "destroying the scene must evict the captured history texture before its host address can be reused");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"absent without scene ownership",
                 test_absent_without_scene_ownership},
        TestCase{"exact scene object and generalized history owner",
                 test_exact_scene_object_and_generalized_history_owner},
        TestCase{"exact nerve lifecycle and strict capture boundary",
                 test_exact_nerve_lifecycle_and_strict_capture_boundary},
        TestCase{"Aurora capability queries are real outside a frame",
                 test_aurora_capability_queries_are_real_and_absent_outside_a_frame},
        TestCase{"real GPU captured-frame blur when requested",
                 test_real_gpu_capture_blur_when_requested},
    };

    auto failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    return failures == 0 ? 0 : 1;
}
