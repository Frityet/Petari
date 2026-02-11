#include "RenderWindow.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Logger.hpp"

namespace smgpc::render
{
    namespace {
        [[nodiscard]] std::uint16_t clamp_view_dimension(int value)
        {
            constexpr int MAX_VIEW_SIZE = static_cast<int>(std::numeric_limits<std::uint16_t>::max());
            if (value < 0) {
                return 0U;
            }
            return static_cast<std::uint16_t>(std::min(value, MAX_VIEW_SIZE));
        }
    }

    class BGFXRenderer : public Renderer {
    public:
        class Exception : public std::exception {};

        class Callbacks : public bgfx::CallbackI {
        public:
            explicit Callbacks(std::shared_ptr<logging::ILogger> logger)
                : _logger(std::move(logger)) {
                if (not _logger) {
                    throw std::invalid_argument("BGFX callbacks requires a logger.");
                }
            }

            ~Callbacks() override = default;

            void fatal(const char *file_path, uint16_t line, bgfx::Fatal::Enum, const char *message) override {
                _logger->fatal(file_path, line, logging::Category::RENDERER, "BGFX error: {}", message);
                throw Exception();
            }

            void traceVargs(const char *file, uint16_t line, const char *format, va_list argument_list) override {
                char buf[1024];
                vsnprintf(buf, 1024, format, argument_list);
                _logger->debug(file, line, logging::Category::RENDERER, "BGFX trace: {}", buf);
            }

            void profilerBegin(const char *, uint32_t, const char *, uint16_t) override {}
            void profilerBeginLiteral(const char *, uint32_t, const char *, uint16_t) override {}
            void profilerEnd() override {}
            uint32_t cacheReadSize(uint64_t) override { return 0; }
            bool cacheRead(uint64_t, void *, uint32_t) override { return false; }
            void cacheWrite(uint64_t, const void *, uint32_t) override {}
            void screenShot(const char *, uint32_t, uint32_t, uint32_t, const void *, uint32_t, bool) override {}
            void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
            void captureEnd() override {}
            void captureFrame(const void *, uint32_t) override {}

        private:
            std::shared_ptr<logging::ILogger> _logger {};
        };

        BGFXRenderer(void *x11_window, void *x11_display, std::shared_ptr<logging::ILogger> logger)
        {
            auto init = bgfx::Init();
            init.platformData.nwh = x11_window;
            init.platformData.ndt = x11_display;
            _callbacks = std::make_unique<Callbacks>(std::move(logger));
            init.callback = _callbacks.get();

            if (not bgfx::init(init)) {
                throw std::runtime_error("Cannot init bgfx!");
            }
        }

        void on_frame_enter() override
        {
        }
        void draw() override
        {
            bgfx::frame();
        }
        void on_frame_exit() override
        {
        }

        ~BGFXRenderer() override = default;

    private:
        std::unique_ptr<Callbacks> _callbacks {};
    };

    class GLFWWindow : public Window {
    public:
        GLFWWindow(const WindowConfiguration &configuration, std::shared_ptr<logging::ILogger> logger)
            : _logger(std::move(logger))
        {
            if (not _logger) {
                throw std::invalid_argument("GLFW window requires a logger.");
            }

            if (not glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW");
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            _window = glfwCreateWindow(configuration.width, configuration.height, configuration.title.c_str(), nullptr, nullptr);
            if (not _window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }

            try {
                _renderer = std::make_unique<BGFXRenderer>(reinterpret_cast<void *>(static_cast<std::uintptr_t>(glfwGetX11Window(_window))), glfwGetX11Display(), _logger);
            } catch (...) {
                glfwDestroyWindow(_window);
                _window = nullptr;
                glfwTerminate();
                throw;
            }

            _logger->debug(__FILE__, __LINE__, logging::Category::RENDERER, "Using renderertype {}", bgfx::getRendererName(bgfx::getRendererType()));
            bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        }

        bool poll_events() override
        {
            glfwPollEvents();
            return not glfwWindowShouldClose(_window);
        }

        void render_frame() override
        {
            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(_window, &width, &height);
            bgfx::setViewRect(0, 0, 0, clamp_view_dimension(width), clamp_view_dimension(height));
            bgfx::touch(0);
        }

        Renderer &renderer() override
        {
            return *_renderer;
        }

        ~GLFWWindow() override
        {
            if (_renderer) {
                bgfx::shutdown();
            }
            if (_window) {
                glfwDestroyWindow(_window);
                _window = nullptr;
            }
            glfwTerminate();
        }
    private:
        GLFWwindow *_window {nullptr};
        std::unique_ptr<BGFXRenderer> _renderer {};
        std::shared_ptr<logging::ILogger> _logger {};
    };

    class GLFWWindowFactory : public IWindowFactory {
    public:
        explicit GLFWWindowFactory(std::shared_ptr<logging::ILogger> logger)
            : _logger(std::move(logger)) {
            if (not _logger) {
                throw std::invalid_argument("GLFWWindowFactory requires a logger.");
            }
        }

        [[nodiscard]] std::unique_ptr<Window> create(const WindowConfiguration &configuration) const override
        {
            return std::make_unique<GLFWWindow>(configuration, _logger);
        }

    private:
        std::shared_ptr<logging::ILogger> _logger {};
    };

    class WindowRendererService final : public IRendererService {
    public:
        WindowRendererService(std::shared_ptr<IWindowFactory> window_factory, WindowConfiguration configuration, std::shared_ptr<logging::ILogger> logger)
            : _logger(std::move(logger)) {
            if (not window_factory or not _logger) {
                throw std::invalid_argument("RendererService requires non-null dependencies.");
            }

            _window = window_factory->create(configuration);
            if (not _window) {
                throw std::runtime_error("RendererService failed to create a window.");
            }
        }

        Renderer &renderer() override
        {
            return _window->renderer();
        }

        bool poll_events() override
        {
            return _window->poll_events();
        }

        void render_frame() override
        {
            _window->render_frame();
        }

    private:
        std::unique_ptr<Window> _window {};
        std::shared_ptr<logging::ILogger> _logger {};
    };

    std::shared_ptr<IWindowFactory> create_default_window_factory(std::shared_ptr<logging::ILogger> logger)
    {
        return std::make_shared<GLFWWindowFactory>(std::move(logger));
    }

    std::shared_ptr<IRendererService> create_default_renderer_service(std::shared_ptr<IWindowFactory> window_factory, WindowConfiguration configuration, std::shared_ptr<logging::ILogger> logger) {
        return std::make_shared<WindowRendererService>(std::move(window_factory), std::move(configuration), std::move(logger));
    }
}
