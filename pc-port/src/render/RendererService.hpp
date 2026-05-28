#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include "ServiceProvider.hpp"
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
    using WindowConfiguration = core::WindowConfiguration;

    class AuroraWindow {
    public:
        explicit AuroraWindow(const WindowConfiguration &configuration);
        AuroraWindow(const AuroraWindow &) = delete;
        AuroraWindow &operator=(const AuroraWindow &) = delete;
        virtual ~AuroraWindow();

        virtual bool poll_events();
        [[nodiscard]] virtual bool should_close() const;
        [[nodiscard]] virtual bool is_focused() const;
        [[nodiscard]] virtual bool is_minimized() const;
        [[nodiscard]] virtual FramebufferInfo framebuffer_size() const;
        [[nodiscard]] virtual NativeWindowHandle native_handle() const;
        [[nodiscard]] virtual bool is_input_pressed(InputButton button) const;
        [[nodiscard]] virtual InputPointerState input_pointer_state() const;
        virtual void close();
        virtual void shutdown();

    protected:
        AuroraWindow();

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    class AuroraRenderer {
    public:
        explicit AuroraRenderer(AuroraWindow &window);
        AuroraRenderer(const AuroraRenderer &) = delete;
        AuroraRenderer &operator=(const AuroraRenderer &) = delete;
        virtual ~AuroraRenderer();

        [[nodiscard]] virtual FrameContext begin_frame();
        virtual void end_frame();
        virtual void shutdown();
        virtual void request_screenshot_png(const std::filesystem::path &path);
        [[nodiscard]] virtual TextureHandle create_rgba8_texture(std::uint16_t width, std::uint16_t height,
                                                                 std::span<const std::uint8_t> rgba);
        virtual void destroy_texture(TextureHandle texture);
        virtual void submit_textured_quad(TextureHandle texture, const TexturedQuad2D &quad);
        virtual void submit_textured_triangles(TextureHandle texture, const TexturedTriangleBatch2D &batch);
        virtual void submit_gx_material_triangles(const GxMaterialTriangleBatch2D &batch);
        [[nodiscard]] virtual FramebufferInfo framebuffer_size() const;
        [[nodiscard]] virtual FramebufferInfo logical_framebuffer_size() const;

    protected:
        AuroraRenderer();

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

}  // namespace smgpc::render
