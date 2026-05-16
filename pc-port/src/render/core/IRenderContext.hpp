#pragma once

#include <filesystem>
#include <span>

#include "RenderTypes.hpp"

namespace smgpc::render::core {

    class IWindowService {
    public:
        virtual ~IWindowService() = default;

        virtual bool poll_events() = 0;
        [[nodiscard]] virtual bool should_close() const = 0;
        [[nodiscard]] virtual bool is_focused() const = 0;
        [[nodiscard]] virtual bool is_minimized() const = 0;
        [[nodiscard]] virtual FramebufferInfo framebuffer_size() const = 0;
        [[nodiscard]] virtual NativeWindowHandle native_handle() const = 0;
        [[nodiscard]] virtual bool is_input_pressed(InputButton button) const = 0;

        virtual void close() {
        }
    };

    class IRendererEngine {
    public:
        virtual ~IRendererEngine() = default;

        [[nodiscard]] virtual FrameContext begin_frame() = 0;
        virtual void end_frame() = 0;
        virtual void shutdown() = 0;
        virtual void request_screenshot_png(const std::filesystem::path& path) = 0;
        [[nodiscard]] virtual TextureHandle create_rgba8_texture(std::uint16_t width, std::uint16_t height, std::span< const std::uint8_t > rgba) = 0;
        virtual void destroy_texture(TextureHandle texture) = 0;
        virtual void submit_textured_quad(TextureHandle texture, const TexturedQuad2D& quad) = 0;
        virtual void submit_textured_triangles(TextureHandle texture, const TexturedTriangleBatch2D& batch) = 0;
        [[nodiscard]] virtual FramebufferInfo framebuffer_size() const = 0;
        [[nodiscard]] virtual FramebufferInfo logical_framebuffer_size() const = 0;
    };

}  // namespace smgpc::render::core
