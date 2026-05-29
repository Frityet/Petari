#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include "ServiceProvider.hpp"
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
        void shutdown();
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
        void submit_gx_material_triangles(const GxMaterialTriangleBatch2D &batch);
        void submit_gx_material_triangles_3d(const GxMaterialTriangleBatch2D &batch,
                                             const smgpc::camera::CameraPose &camera_pose);
        [[nodiscard]] FramebufferInfo framebuffer_size() const;
        [[nodiscard]] FramebufferInfo logical_framebuffer_size() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    [[nodiscard]] AuroraRenderer &current_aurora_renderer();

    class ScopedAuroraRendererContext final {
    public:
        explicit ScopedAuroraRendererContext(AuroraRenderer &renderer);
        ScopedAuroraRendererContext(const ScopedAuroraRendererContext &) = delete;
        ScopedAuroraRendererContext &operator=(const ScopedAuroraRendererContext &) = delete;
        ~ScopedAuroraRendererContext();

    private:
        AuroraRenderer *_previous = nullptr;
    };

}  // namespace smgpc::render
