#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/SceneScheduler.hpp"

#include <aurora/gfx.h>
#include <dolphin/gx/GXAurora.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    constexpr auto kEfbWidth = std::uint16_t {640U};
    constexpr auto kEfbHeight = std::uint16_t {456U};

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    class DrawProbe final : public NameObj {
    public:
        explicit DrawProbe(const char *name) : NameObj(name) {
        }

        void draw() const override {
            ++draw_count;
        }

        mutable std::size_t draw_count = 0U;
    };

    void test_exact_scheduler_order_and_clipping_isolation() {
#ifdef NDEBUG
        throw std::runtime_error("Model3DFor2D scheduler proof requires a debug build");
#else
        auto scheduler = smgpc::runtime::SceneScheduler {};
        auto model_0x24 = LiveActor("model-0x24");
        auto model_0x25 = LiveActor("model-0x25");
        model_0x24.makeActorAppeared();
        model_0x25.makeActorAppeared();
        model_0x24.mPosition.set(1000000.0F, 1000000.0F, 1000000.0F);
        model_0x25.mPosition.set(-1000000.0F, -1000000.0F, -1000000.0F);
        smgpc::compat::configure_actor_clipping_sphere(&model_0x24, 100.0F, nullptr);
        smgpc::compat::configure_actor_clipping_sphere(&model_0x25, 100.0F, nullptr);
        scheduler.register_live_actor_model(model_0x24, -1, -1,
                                            MR::DrawBufferType_Model3DFor2D, -1);
        scheduler.register_live_actor_model(model_0x25, -1, -1,
                                            MR::DrawBufferType_0x25, -1);

        auto comet = DrawProbe("comet");
        auto galaxy_name = DrawProbe("galaxy-name");
        auto layout = DrawProbe("layout");
        auto layout_decoration = DrawProbe("layout-decoration");
        auto cinema = DrawProbe("cinema");
        auto talk = DrawProbe("talk");
        auto draw_0x44 = DrawProbe("draw-0x44");
        auto effect_2d = DrawProbe("effect-2d");
        auto effect_for_2d_model = DrawProbe("effect-for-2d-model");
        scheduler.connect_name_obj(comet, -1, -1, -1,
                                   MR::DrawType_CometScreenFilter);
        scheduler.connect_name_obj(galaxy_name, -1, -1, -1,
                                   MR::DrawType_GalaxyNamePlate);
        scheduler.connect_name_obj(layout, -1, -1, -1, MR::DrawType_Layout);
        scheduler.connect_name_obj(layout_decoration, -1, -1, -1,
                                   MR::DrawType_LayoutDecoration);
        scheduler.connect_name_obj(cinema, -1, -1, -1,
                                   MR::DrawType_CinemaFrame);
        scheduler.connect_name_obj(talk, -1, -1, -1,
                                   MR::DrawType_TalkLayout);
        scheduler.connect_name_obj(draw_0x44, -1, -1, -1, MR::DrawType_0x44);
        scheduler.connect_name_obj(effect_2d, -1, -1, -1,
                                   MR::DrawType_EffectDraw2D);
        scheduler.connect_name_obj(effect_for_2d_model, -1, -1, -1,
                                   MR::DrawType_EffectDrawFor2DModel);

        const auto perspective_camera = smgpc::camera::CameraPose {
            .eye = {0.0F, 0.0F, 0.0F},
            .watch = {0.0F, 0.0F, -1.0F},
        };
        scheduler.execute_draw_buffer_opa(perspective_camera,
                                          MR::DrawBufferType_Planet);
        require(!model_0x24.mFlag.mIsClipped && !model_0x25.mFlag.mIsClipped,
                "perspective draw buffers must not update clipping for 2D-model actors");
        scheduler.execute_draw_list_2d_normal();

        const auto trace = scheduler.last_execution_trace();
        require(trace.size() == 13U,
                "retail 2D list must execute four model-buffer passes and nine draw types");
        const auto require_entry = [&](std::size_t index, std::string_view name,
                                       smgpc::runtime::SceneSchedulerPhase phase,
                                       smgpc::runtime::SceneDrawBufferPass pass) {
            const auto &entry = trace[index];
            require(entry.name == name && entry.phase == phase &&
                        entry.draw_buffer_pass == pass,
                    "Model3DFor2D scheduler order diverged from SceneExecutor");
        };
        require_entry(0U, "model-0x24", smgpc::runtime::SceneSchedulerPhase::DrawBufferOpa,
                      smgpc::runtime::SceneDrawBufferPass::Opaque);
        require_entry(1U, "model-0x24", smgpc::runtime::SceneSchedulerPhase::DrawBufferXlu,
                      smgpc::runtime::SceneDrawBufferPass::Translucent);
        constexpr auto before_0x25_names = std::array<std::string_view, 7U> {
            "comet", "galaxy-name", "layout", "layout-decoration", "cinema",
            "talk", "draw-0x44",
        };
        for (auto index = std::size_t {}; index < before_0x25_names.size(); ++index) {
            require_entry(index + 2U, before_0x25_names[index],
                          smgpc::runtime::SceneSchedulerPhase::DrawType,
                          smgpc::runtime::SceneDrawBufferPass::None);
        }
        require_entry(9U, "model-0x25", smgpc::runtime::SceneSchedulerPhase::DrawBufferOpa,
                      smgpc::runtime::SceneDrawBufferPass::Opaque);
        require_entry(10U, "model-0x25", smgpc::runtime::SceneSchedulerPhase::DrawBufferXlu,
                      smgpc::runtime::SceneDrawBufferPass::Translucent);
        require_entry(11U, "effect-2d", smgpc::runtime::SceneSchedulerPhase::DrawType,
                      smgpc::runtime::SceneDrawBufferPass::None);
        require_entry(12U, "effect-for-2d-model",
                      smgpc::runtime::SceneSchedulerPhase::DrawType,
                      smgpc::runtime::SceneDrawBufferPass::None);

        require(!model_0x24.mFlag.mIsClipped && !model_0x25.mFlag.mIsClipped,
                "camera-free 2D model buffers must not run perspective clipping updates");
        require(comet.draw_count == 1U && galaxy_name.draw_count == 1U &&
                    layout.draw_count == 1U && layout_decoration.draw_count == 1U &&
                    cinema.draw_count == 1U && talk.draw_count == 1U &&
                    draw_0x44.draw_count == 1U && effect_2d.draw_count == 1U &&
                    effect_for_2d_model.draw_count == 1U,
                "each retail 2D draw type must execute exactly once");
#endif
    }

    [[nodiscard]] std::vector<std::uint8_t> read_display_copy() {
        auto width = 0U;
        auto height = 0U;
        require(AuroraGetDisplayCopySize(&width, &height) == GX_TRUE &&
                    width == kEfbWidth && height == kEfbHeight,
                "Model3DFor2D proof requires a 640x456 display copy");
        auto stride = 0U;
        auto pixels = std::vector<std::uint8_t>(
            static_cast<std::size_t>(width) * height * 4U);
        require(AuroraReadDisplayCopyRGBA8(pixels.data(),
                                           static_cast<u32>(pixels.size()),
                                           &width, &height, &stride) == GX_TRUE &&
                    stride == width * 4U,
                "Model3DFor2D display copy must be readable as RGBA8");
        return pixels;
    }

    [[nodiscard]] bool pixel_is_orange(std::span<const std::uint8_t> pixels,
                                       std::size_t x, std::size_t y) {
        const auto offset = (y * kEfbWidth + x) * 4U;
        return pixels[offset] >= 220U && pixels[offset + 1U] >= 75U &&
               pixels[offset + 1U] <= 105U && pixels[offset + 2U] <= 35U;
    }

    void test_exact_camera_free_projection() {
        auto window = smgpc::render::AuroraWindow({
            .width = kEfbWidth,
            .height = kEfbHeight,
            .title = "SMG PC Model3DFor2D projection proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        AuroraSetViewportPolicy(AURORA_VIEWPORT_NATIVE);
        renderer.set_copy_clear({
            .color = {0U, 0U, 0U, 255U},
            .depth = GX_MAX_Z24,
        });

        for (auto frame_index = 0U; frame_index < 2U; ++frame_index) {
            static_cast<void>(renderer.begin_frame());
            const auto context = smgpc::render::ScopedAuroraRendererContext(renderer);
            renderer.end_frame();
        }

        constexpr auto orange = std::array<std::uint8_t, 4U> {240U, 88U, 16U, 255U};
        static_cast<void>(renderer.begin_frame());
        const auto context = smgpc::render::ScopedAuroraRendererContext(renderer);
        const auto texture = renderer.create_rgba8_texture(1U, 1U, orange);
        const auto vertices = std::array<smgpc::render::TexturedVertex2D, 4U> {
            smgpc::render::TexturedVertex2D {.x = 0.0F, .y = 0.0F, .z = 5000.0F,
                                             .u = 0.0F, .v = 0.0F},
            smgpc::render::TexturedVertex2D {.x = 608.0F, .y = 0.0F, .z = 5000.0F,
                                             .u = 1.0F, .v = 0.0F},
            smgpc::render::TexturedVertex2D {.x = 608.0F, .y = -456.0F, .z = 5000.0F,
                                             .u = 1.0F, .v = 1.0F},
            smgpc::render::TexturedVertex2D {.x = 0.0F, .y = -456.0F, .z = 5000.0F,
                                             .u = 0.0F, .v = 1.0F},
        };
        constexpr auto indices = std::array<std::uint16_t, 6U> {0U, 1U, 2U, 0U, 2U, 3U};
        renderer.submit_textured_triangles_model_3d_for_2d(
            texture,
            smgpc::render::TexturedTriangleBatch2D {
                .vertices = vertices,
                .indices = indices,
                .blend = false,
                .blend_mode = smgpc::render::BlendMode::Opaque,
            },
            smgpc::render::Model3DFor2DProjection {
                .screen_width = 608.0F,
                .screen_height = 456.0F,
            });
        renderer.end_frame();

        const auto pixels = read_display_copy();
        require(pixel_is_orange(pixels, 8U, 8U) &&
                    pixel_is_orange(pixels, kEfbWidth - 9U, 8U) &&
                    pixel_is_orange(pixels, 8U, kEfbHeight - 9U) &&
                    pixel_is_orange(pixels, kEfbWidth - 9U, kEfbHeight - 9U),
                "608x456 screen coordinates must cover the 640x456 EFB through an identity-view orthographic projection");
    }
}  // namespace

int main() {
    try {
        test_exact_scheduler_order_and_clipping_isolation();
        test_exact_camera_free_projection();
        std::cout << "[ok] exact camera-free Model3DFor2D projection and scheduler contract\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] Model3DFor2D contract: " << error.what() << '\n';
        return 1;
    }
}
