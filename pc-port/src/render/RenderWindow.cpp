#include "RenderWindow.hpp"

#include <memory>
#include <stdexcept>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Logger.hpp"

namespace smgpc::render
{
    class BGFXRenderer : public Renderer {
    public:
        class Exception : std::exception {};

        class Callbacks : public bgfx::CallbackI {
        public:
            virtual ~Callbacks() {}

            virtual void (fatal)(const char *filePath, uint16_t line, bgfx::Fatal::Enum, const char *str) override {
                logging::log(stderr, filePath, line, logging::Level::FATAL, logging::Category::RENDERER, "BGFX error: {}", str);
                throw Exception();
            }

            virtual void traceVargs(const char *file, uint16_t line, const char *format, va_list argList) override
            {
                char buf[1024];
                vsnprintf(buf, 1024, format, argList);
                logging::log(stdout, file, line, logging::Level::DEBUG, logging::Category::RENDERER, "BGFX trace: {}", buf);
            }

            virtual void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
            virtual void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
            virtual void profilerEnd() override {}
            virtual uint32_t cacheReadSize(uint64_t) override { return 0; }
            virtual bool cacheRead(uint64_t, void*, uint32_t) override { return false; }
            virtual void cacheWrite(uint64_t, const void*, uint32_t) override {}
            virtual void screenShot(const char*, uint32_t, uint32_t, uint32_t, const void*, uint32_t, bool) override {}
            virtual void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
            virtual void captureEnd() override {}
            virtual void captureFrame(const void*, uint32_t) override {}
        };

        BGFXRenderer(void *x11wind, void *x11disp)
        {
            auto init = bgfx::Init();
            init.platformData.nwh = x11wind;
            init.platformData.ndt = x11disp;
            init.callback = new Callbacks();

            if (not bgfx::init(init)) {
                throw std::runtime_error("Cannot init bgfx!");
            }
        }

        virtual void on_frame_enter() override
        {

        }
        virtual void draw() override
        {
            bgfx::frame();
        }
        virtual void on_frame_exit() override
        {

        }

        ~BGFXRenderer() {}
    };

    class GLFWWindow : public Window {
    public:
        GLFWWindow(int width, int height, std::string_view title)
        {
            if (not glfwInit()) {
                throw std::runtime_error("Failed to initialize GLFW");
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            _window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
            if (not _window) {
                glfwTerminate();
                throw std::runtime_error("Failed to create GLFW window");
            }

            _rend = std::make_unique<BGFXRenderer>(reinterpret_cast<void *>(static_cast<uintptr_t>(glfwGetX11Window(_window))), glfwGetX11Display());

            logging::debug(logging::Category::RENDERER, "Using renderertype {}", bgfx::getRendererName(bgfx::getRendererType()));
		    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        }

        bool poll_events()
        {
            glfwPollEvents();
            return not glfwWindowShouldClose(_window);
        }

        virtual void handle_events() override
        {
            while (poll_events()) {
            }
        }

        virtual Renderer &renderer() override
        { return *_rend; }

        ~GLFWWindow()
        {
            bgfx::shutdown();
            if (_window) {
                glfwDestroyWindow(_window);
                _window = nullptr;
            }
            glfwTerminate();
        }
    private:
        GLFWwindow *_window;
        std::unique_ptr<BGFXRenderer> _rend;
    };
    Renderer::~Renderer() {}
    Window::~Window() {}

    std::unique_ptr<Window> Window::create(int width, int height, std::string_view title)
    {
        return std::make_unique<GLFWWindow>(width, height, title);
    }
}
