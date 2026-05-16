#include "RendererService.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3.h>
#if defined(__linux__)
#include <GLFW/glfw3native.h>
#endif

#include "backends/BgfxBackend.hpp"

namespace smgpc::render {
    namespace {

        constexpr std::uint16_t clamp_window_dimension(int value) {
            return static_cast< std::uint16_t >(std::clamp(value, 1, std::numeric_limits< int >::max() / 16));
        }

        class GLFWWindowService final : public IWindowService {
        public:
            explicit GLFWWindowService(const WindowConfiguration& configuration) {
                if (not glfwInit()) {
                    throw std::runtime_error("Failed to initialize GLFW");
                }

                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                _window = glfwCreateWindow(configuration.width, configuration.height, configuration.title.c_str(), nullptr, nullptr);
                if (_window == nullptr) {
                    glfwTerminate();
                    throw std::runtime_error("Failed to create GLFW window");
                }
            }

            GLFWWindowService(const GLFWWindowService&) = delete;
            GLFWWindowService& operator=(const GLFWWindowService&) = delete;

            ~GLFWWindowService() override {
                if (_window != nullptr) {
                    glfwDestroyWindow(_window);
                    _window = nullptr;
                }
                glfwTerminate();
            }

            bool poll_events() override {
                if (_window == nullptr) {
                    return false;
                }

                glfwPollEvents();
                return not should_close();
            }

            [[nodiscard]] bool should_close() const override {
                return _window == nullptr || glfwWindowShouldClose(_window);
            }

            [[nodiscard]] bool is_focused() const override {
                return _window != nullptr && glfwGetWindowAttrib(_window, GLFW_FOCUSED) == GLFW_TRUE;
            }

            [[nodiscard]] bool is_minimized() const override {
                return _window != nullptr && glfwGetWindowAttrib(_window, GLFW_ICONIFIED) == GLFW_TRUE;
            }

            [[nodiscard]] FramebufferInfo framebuffer_size() const override {
                if (_window == nullptr) {
                    return {1U, 1U};
                }

                int width = 1;
                int height = 1;
                glfwGetFramebufferSize(_window, &width, &height);
                return {
                    .width = clamp_window_dimension(width),
                    .height = clamp_window_dimension(height),
                };
            }

            [[nodiscard]] NativeWindowHandle native_handle() const override {
#if defined(__linux__)
                return {
                    .window_handle = reinterpret_cast< void* >(static_cast< std::uintptr_t >(glfwGetX11Window(_window))),
                    .display_handle = glfwGetX11Display(),
                };
#else
                return {reinterpret_cast< void* >(_window), nullptr};
#endif
            }

            [[nodiscard]] bool is_input_pressed(InputButton button) const override {
                if (_window == nullptr) {
                    return false;
                }

                switch (button) {
                case InputButton::CORE_PAD_A:
                    return glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_ENTER) == GLFW_PRESS;
                case InputButton::CORE_PAD_B:
                    return glfwGetKey(_window, GLFW_KEY_B) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
                }

                return false;
            }

            void close() override {
                if (_window != nullptr) {
                    glfwSetWindowShouldClose(_window, GLFW_TRUE);
                }
            }

        private:
            GLFWwindow* _window{nullptr};
        };

        class GLFWWindowFactory final : public IWindowFactory {
        public:
            [[nodiscard]] std::unique_ptr< IWindowService > create(const WindowConfiguration& configuration) const override {
                return std::make_unique< GLFWWindowService >(configuration);
            }
        };

        class RendererService final : public IRendererEngine {
        public:
            explicit RendererService(di::DependencyReference< IWindowService > window_service) : _window_service(std::move(window_service)) {
            }

            RendererService(const RendererService&) = delete;
            RendererService& operator=(const RendererService&) = delete;

            [[nodiscard]] FrameContext begin_frame() override {
                if (not _backend.is_initialized()) {
                    initialize_backend();
                }

                const auto now = std::chrono::steady_clock::now();
                const auto raw_delta = (_frame_anchor == std::chrono::steady_clock::time_point{}) ?
                                           std::chrono::duration< double >::zero() :
                                           std::chrono::duration< double >(now - _frame_anchor);
                _frame_anchor = now;

                const double delta = std::max(0.0, raw_delta.count());
                _frame_time += delta;
                ++_frame_index;

                auto framebuffer = _window_service->framebuffer_size();
                if (framebuffer.width == 0U || framebuffer.height == 0U) {
                    framebuffer = {1U, 1U};
                }

                if (not _framebuffer_ready || framebuffer.width != _framebuffer.width || framebuffer.height != _framebuffer.height) {
                    _backend.resize(framebuffer.width, framebuffer.height);
                    _framebuffer = framebuffer;
                    _framebuffer_ready = true;
                }

                const FrameContext context{
                    .frame_index = _frame_index,
                    .frame_time_seconds = _frame_time,
                    .frame_delta_seconds = delta,
                    .framebuffer = _framebuffer,
                    .has_focus = _window_service->is_focused(),
                    .is_minimized = _window_service->is_minimized(),
                };

                _backend.begin_frame(context);
                return context;
            }

            void end_frame() override {
                _backend.end_frame();
            }

            void shutdown() override {
                _backend.shutdown();
            }

            void request_screenshot_png(const std::filesystem::path& path) override {
                if (not _backend.is_initialized()) {
                    initialize_backend();
                }

                _backend.request_screenshot_png(path);
            }

            [[nodiscard]] TextureHandle create_rgba8_texture(std::uint16_t width, std::uint16_t height,
                                                             std::span< const std::uint8_t > rgba) override {
                if (not _backend.is_initialized()) {
                    initialize_backend();
                }

                return _backend.create_rgba8_texture(width, height, rgba);
            }

            void destroy_texture(TextureHandle texture) override {
                if (_backend.is_initialized()) {
                    _backend.destroy_texture(texture);
                }
            }

            void submit_textured_quad(TextureHandle texture, const TexturedQuad2D& quad) override {
                if (not _backend.is_initialized()) {
                    initialize_backend();
                }

                _backend.submit_textured_quad(texture, quad);
            }

            void submit_textured_triangles(TextureHandle texture, const TexturedTriangleBatch2D& batch) override {
                if (not _backend.is_initialized()) {
                    initialize_backend();
                }

                _backend.submit_textured_triangles(texture, batch);
            }

            void submit_gx_material_triangles(const GxMaterialTriangleBatch2D& batch) override {
                if (not _backend.is_initialized()) {
                    initialize_backend();
                }

                _backend.submit_gx_material_triangles(batch);
            }

            [[nodiscard]] FramebufferInfo framebuffer_size() const override {
                if (_backend.is_initialized()) {
                    return _backend.framebuffer_size();
                }

                return _window_service->framebuffer_size();
            }

            [[nodiscard]] FramebufferInfo logical_framebuffer_size() const override {
                if (_backend.is_initialized()) {
                    return _backend.logical_framebuffer_size();
                }

                return {
                    .width = core::kWiiLogicalFramebufferWidth,
                    .height = core::kWiiLogicalFramebufferHeight,
                };
            }

        private:
            void initialize_backend() {
                const auto framebuffer = _window_service->framebuffer_size();
                const auto native_handle = _window_service->native_handle();

                core::RenderInitDesc init_desc{};
                init_desc.width = framebuffer.width;
                init_desc.height = framebuffer.height;
                init_desc.title = "SMG PC Port";
                init_desc.enable_vsync = true;
                init_desc.native_window_handle = native_handle.window_handle;
                init_desc.native_display_handle = native_handle.display_handle;

                _backend.initialize(init_desc);
                _framebuffer = framebuffer;
                _framebuffer_ready = true;
                _frame_anchor = std::chrono::steady_clock::now();
            }

            di::DependencyReference< IWindowService > _window_service;
            backends::BgfxBackend _backend{};
            std::uint64_t _frame_index{};
            double _frame_time{};
            bool _framebuffer_ready{};
            FramebufferInfo _framebuffer{1U, 1U};
            std::chrono::steady_clock::time_point _frame_anchor{};
        };

    }  // namespace

    std::unique_ptr< IWindowFactory > create_default_window_factory(di::DependencyReference< logging::ILogger >) {
        return std::make_unique< GLFWWindowFactory >();
    }

    std::unique_ptr< IRendererEngine > create_default_renderer_engine(di::DependencyReference< IWindowService > window_service,
                                                                      di::DependencyReference< logging::ILogger >) {
        return std::make_unique< RendererService >(std::move(window_service));
    }

}  // namespace smgpc::render
