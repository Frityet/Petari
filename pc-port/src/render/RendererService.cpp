#include "RendererService.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
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

        [[nodiscard]] bool read_bool_env(const char* name, bool fallback) {
            const auto* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return fallback;
            }

            const auto text = std::string_view(value);
            if (text == "0" || text == "false" || text == "False" || text == "off" || text == "OFF" || text == "no" || text == "NO") {
                return false;
            }
            if (text == "1" || text == "true" || text == "True" || text == "on" || text == "ON" || text == "yes" || text == "YES") {
                return true;
            }

            return fallback;
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
                refresh_cached_window_state();
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
                refresh_cached_window_state();
                return not should_close();
            }

            [[nodiscard]] bool should_close() const override {
                return _window == nullptr || _should_close;
            }

            [[nodiscard]] bool is_focused() const override {
                return _focused;
            }

            [[nodiscard]] bool is_minimized() const override {
                return _minimized;
            }

            [[nodiscard]] FramebufferInfo framebuffer_size() const override {
                return _framebuffer;
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
                    return glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_ENTER) == GLFW_PRESS ||
                           glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
                case InputButton::CORE_PAD_B:
                    return glfwGetKey(_window, GLFW_KEY_B) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_BACKSPACE) == GLFW_PRESS ||
                           glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
                case InputButton::CORE_PAD_UP:
                    return glfwGetKey(_window, GLFW_KEY_UP) == GLFW_PRESS;
                case InputButton::CORE_PAD_DOWN:
                    return glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS;
                case InputButton::CORE_PAD_LEFT:
                    return glfwGetKey(_window, GLFW_KEY_LEFT) == GLFW_PRESS;
                case InputButton::CORE_PAD_RIGHT:
                    return glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
                case InputButton::CORE_PAD_PLUS:
                    return glfwGetKey(_window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_KP_ADD) == GLFW_PRESS;
                case InputButton::CORE_PAD_MINUS:
                    return glfwGetKey(_window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS;
                case InputButton::CORE_PAD_HOME:
                    return glfwGetKey(_window, GLFW_KEY_HOME) == GLFW_PRESS || glfwGetKey(_window, GLFW_KEY_H) == GLFW_PRESS;
                case InputButton::CORE_PAD_C:
                    return glfwGetKey(_window, GLFW_KEY_C) == GLFW_PRESS;
                case InputButton::CORE_PAD_Z:
                    return glfwGetKey(_window, GLFW_KEY_Z) == GLFW_PRESS;
                case InputButton::COUNT:
                    break;
                }

                return false;
            }

            [[nodiscard]] InputPointerState input_pointer_state() const override {
                if (_window == nullptr) {
                    return {};
                }

                int width = 1;
                int height = 1;
                glfwGetWindowSize(_window, &width, &height);
                if (width <= 0 || height <= 0) {
                    return {};
                }

                double cursor_x = 0.0;
                double cursor_y = 0.0;
                glfwGetCursorPos(_window, &cursor_x, &cursor_y);
                const auto valid = cursor_x >= 0.0 && cursor_y >= 0.0 && cursor_x < static_cast< double >(width) &&
                                   cursor_y < static_cast< double >(height);
                return InputPointerState {
                    .x = static_cast< float >(cursor_x * static_cast< double >(core::kWiiLogicalFramebufferWidth) / static_cast< double >(width)),
                    .y = static_cast< float >(cursor_y * static_cast< double >(core::kWiiLogicalFramebufferHeight) / static_cast< double >(height)),
                    .valid = valid,
                };
            }

            void close() override {
                if (_window != nullptr) {
                    glfwSetWindowShouldClose(_window, GLFW_TRUE);
                    _should_close = true;
                }
            }

        private:
            void refresh_cached_window_state() {
                if (_window == nullptr) {
                    _framebuffer = {1U, 1U};
                    _focused = false;
                    _minimized = false;
                    _should_close = true;
                    return;
                }

                int width = 1;
                int height = 1;
                glfwGetFramebufferSize(_window, &width, &height);
                _framebuffer = {
                    .width = clamp_window_dimension(width),
                    .height = clamp_window_dimension(height),
                };
                _focused = glfwGetWindowAttrib(_window, GLFW_FOCUSED) == GLFW_TRUE;
                _minimized = glfwGetWindowAttrib(_window, GLFW_ICONIFIED) == GLFW_TRUE;
                _should_close = glfwWindowShouldClose(_window) == GLFW_TRUE;
            }

            GLFWwindow* _window {nullptr};
            FramebufferInfo _framebuffer {1U, 1U};
            bool _focused {false};
            bool _minimized {false};
            bool _should_close {false};
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
                const auto raw_delta = (_frame_anchor == std::chrono::steady_clock::time_point {}) ?
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

                const FrameContext context {
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

                core::RenderInitDesc init_desc {};
                init_desc.width = framebuffer.width;
                init_desc.height = framebuffer.height;
                init_desc.title = "SMG PC Port";
                init_desc.enable_vsync = read_bool_env("SMGPC_ENABLE_VSYNC", true);
                init_desc.native_window_handle = native_handle.window_handle;
                init_desc.native_display_handle = native_handle.display_handle;

                _backend.initialize(init_desc);
                _framebuffer = framebuffer;
                _framebuffer_ready = true;
                _frame_anchor = std::chrono::steady_clock::now();
            }

            di::DependencyReference< IWindowService > _window_service;
            backends::BgfxBackend _backend {};
            std::uint64_t _frame_index {};
            double _frame_time {};
            bool _framebuffer_ready {};
            FramebufferInfo _framebuffer {1U, 1U};
            std::chrono::steady_clock::time_point _frame_anchor {};
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
