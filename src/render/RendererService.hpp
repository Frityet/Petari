#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include "ServiceProvider.hpp"
#include "render/BrightVisibilityService.hpp"
#include "camera/CameraPose.hpp"
#include "core/RenderTypes.hpp"

namespace smgpc::logging {
    class ILogger;
}

namespace smgpc::render {

    using FrameContext = core::FrameContext;
    using FramebufferInfo = core::FramebufferInfo;
    using CopyEvent = core::CopyEvent;
    using CopyEventKind = core::CopyEventKind;
    using CopyRect = core::CopyRect;
    using CopyViewport = core::CopyViewport;
    using NativeWindowHandle = core::NativeWindowHandle;
    using InputButton = core::InputButton;
    using DebugInput = core::DebugInput;
    using BlendMode = core::BlendMode;
    using DepthCompare = core::DepthCompare;
    using CullMode = core::CullMode;
    using GxAlphaCompare2D = core::GxAlphaCompare2D;
    using GxBlendMode2D = core::GxBlendMode2D;
    using GxFog2D = core::GxFog2D;
    using TextureHandle = core::TextureHandle;
    using InputPointerState = core::InputPointerState;
    using GxMaterialTriangleBatch2D = core::GxMaterialTriangleBatch2D;
    using GxMaterialVertex2D = core::GxMaterialVertex2D;
    using GxTevRegisterColor2D = core::GxTevRegisterColor2D;
    using GxTevStage2D = core::GxTevStage2D;
    using GxTextureStage2D = core::GxTextureStage2D;
    using TexturedTriangleBatch2D = core::TexturedTriangleBatch2D;
    using TexturedQuad2D = core::TexturedQuad2D;
    using TexturedVertex2D = core::TexturedVertex2D;
    using PrimitiveTopology = core::PrimitiveTopology;
    using RenderSpace2D = core::RenderSpace2D;
    using WindowConfiguration = core::WindowConfiguration;

    struct Model3DFor2DProjection {
        // Retail Model3DFor2D coordinates are screen coordinates rather than
        // EFB pixels.  In 4:3 this is 608x456; widescreen expands only the
        // coordinate width while retaining the 640x456 EFB viewport.
        float screen_width = static_cast<float>(core::kWiiLayoutWidth);
        float screen_height = static_cast<float>(core::kWiiLogicalFramebufferHeight);

        bool operator==(const Model3DFor2DProjection &) const = default;
    };

    struct CopyClearState {
        // GameSystemObjHolder::initDisplay installs this retail scene clear.
        // Scenes may override it through AuroraRenderer without embedding
        // route-specific policy in the backend.
        std::array<std::uint8_t, 4U> color = {30U, 30U, 200U, 0U};
        std::uint32_t depth = 0x00ffffffU;

        bool operator==(const CopyClearState &) const = default;
    };

    class AuroraWindow {
    public:
        explicit AuroraWindow(const WindowConfiguration &configuration);
        AuroraWindow(const AuroraWindow &) = delete;
        AuroraWindow &operator=(const AuroraWindow &) = delete;
        ~AuroraWindow();

        bool poll_events();
        [[nodiscard]] bool should_close() const;
        [[nodiscard]] bool is_focused() const;
        [[nodiscard]] bool is_minimized() const;
        [[nodiscard]] FramebufferInfo framebuffer_size() const;
        [[nodiscard]] NativeWindowHandle native_handle() const;
        [[nodiscard]] bool is_input_pressed(InputButton button) const;
        [[nodiscard]] bool is_debug_input_pressed(DebugInput input) const;
        [[nodiscard]] InputPointerState input_pointer_state() const;
        void close();
        void shutdown();

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    class AuroraRenderer {
    public:
        explicit AuroraRenderer(AuroraWindow &window);
        AuroraRenderer(const AuroraRenderer &) = delete;
        AuroraRenderer &operator=(const AuroraRenderer &) = delete;
        ~AuroraRenderer();

        [[nodiscard]] FrameContext begin_frame();
        void end_frame();
        // MainLoopFramework enables the display-copy vertical filter by
        // default; callers such as LogoScene may explicitly suspend it.
        void end_frame(const GXRenderModeObj &render_mode, bool use_vertical_filter = true);
        void shutdown();
        void set_copy_clear(const CopyClearState &state);
        [[nodiscard]] const CopyClearState &copy_clear() const;
        void request_screenshot_png(const std::filesystem::path &path);
        [[nodiscard]] TextureHandle create_rgba8_texture(std::uint16_t width, std::uint16_t height,
                                                         std::span<const std::uint8_t> rgba);
        [[nodiscard]] TextureHandle create_rgba8_mip_texture(std::uint16_t width, std::uint16_t height,
                                                             std::span<const std::uint8_t> rgba, float min_lod,
                                                             float max_lod, float lod_bias, bool bias_clamp,
                                                             bool edge_lod, GXAnisotropy max_anisotropy,
                                                             std::uint8_t min_filter, std::uint8_t mag_filter);
        [[nodiscard]] TextureHandle create_gx_texture(std::uint16_t width, std::uint16_t height, GXTexFmt format,
                                                      std::span<const std::uint8_t> image_data, bool mipmap,
                                                      float min_lod, float max_lod, float lod_bias, bool bias_clamp,
                                                      bool edge_lod, GXAnisotropy max_anisotropy,
                                                      std::uint8_t min_filter, std::uint8_t mag_filter);
        void submit_textured_quad(TextureHandle texture, const TexturedQuad2D &quad);
        void submit_textured_triangles(TextureHandle texture, const TexturedTriangleBatch2D &batch);
        void submit_textured_triangles_3d(TextureHandle texture, const TexturedTriangleBatch2D &batch,
                                          const smgpc::camera::CameraPose &camera_pose);
        void submit_textured_triangles_model_3d_for_2d(
            TextureHandle texture, const TexturedTriangleBatch2D &batch,
            const Model3DFor2DProjection &projection);
        void submit_gx_material_triangles(const GxMaterialTriangleBatch2D &batch);
        void submit_gx_material_triangles_3d(const GxMaterialTriangleBatch2D &batch,
                                             const smgpc::camera::CameraPose &camera_pose);
        void submit_gx_material_triangles_model_3d_for_2d(
            const GxMaterialTriangleBatch2D &batch,
            const Model3DFor2DProjection &projection);
        // Retail calls drawInitFor2DModel before each of its two model-backed
        // 2D buffer groups, even when the group is empty. Keep that state
        // boundary independent of whether a model happens to submit geometry.
        void prepare_model_3d_for_2d(const Model3DFor2DProjection &projection);
        [[nodiscard]] FramebufferInfo framebuffer_size() const;
        [[nodiscard]] FramebufferInfo logical_framebuffer_size() const;
        [[nodiscard]] BrightVisibilityService &bright_visibility_service();

    private:
        void end_frame_impl(const GXRenderModeObj *render_mode, bool use_vertical_filter);

        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    [[nodiscard]] AuroraRenderer &current_aurora_renderer();
    [[nodiscard]] AuroraRenderer *try_current_aurora_renderer() noexcept;

    class ScopedAuroraRendererContext final {
    public:
        explicit ScopedAuroraRendererContext(AuroraRenderer &renderer);
        ScopedAuroraRendererContext(const ScopedAuroraRendererContext &) = delete;
        ScopedAuroraRendererContext &operator=(const ScopedAuroraRendererContext &) = delete;
        ~ScopedAuroraRendererContext();

    private:
        AuroraRenderer *_previous = nullptr;
        ScopedBrightVisibilityServiceOverride _bright_visibility_context;
    };

}  // namespace smgpc::render
