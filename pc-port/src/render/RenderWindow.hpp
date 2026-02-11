#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace smgpc::logging {
    class ILogger;
}

namespace smgpc::render
{
    class Renderer {
    public:
        Renderer(const Renderer &) = delete;
        Renderer(Renderer &&) = delete;

        virtual void on_frame_enter() = 0;
        virtual void draw() = 0;
        virtual void on_frame_exit() = 0;

        virtual ~Renderer() = default;
    protected:
        Renderer() = default;
    };

    struct WindowConfiguration {
        int width;
        int height;
        std::string title;
    };

    class Window {
    public:
        virtual Renderer &renderer() = 0;
        virtual bool poll_events() = 0;
        virtual void render_frame() = 0;
        [[nodiscard]] virtual bool is_key_down(int key) const = 0;
        [[nodiscard]] virtual std::pair<std::uint16_t, std::uint16_t> framebuffer_size() const = 0;
        virtual ~Window() = default;
    };

    class IWindowFactory {
    public:
        virtual ~IWindowFactory() = default;
        [[nodiscard]] virtual std::unique_ptr<Window> create(const WindowConfiguration &configuration) const = 0;
    };

    class IRendererService {
    public:
        virtual ~IRendererService() = default;
        virtual Renderer &renderer() = 0;
        virtual bool poll_events() = 0;
        virtual void render_frame() = 0;
        [[nodiscard]] virtual bool is_key_down(int key) const = 0;
        [[nodiscard]] virtual std::pair<std::uint16_t, std::uint16_t> framebuffer_size() const = 0;
    };

    [[nodiscard]] std::shared_ptr<IWindowFactory> create_default_window_factory(std::shared_ptr<logging::ILogger> logger);

    [[nodiscard]] std::shared_ptr<IRendererService> create_default_renderer_service(std::shared_ptr<IWindowFactory> window_factory, WindowConfiguration configuration, std::shared_ptr<logging::ILogger> logger);
}
