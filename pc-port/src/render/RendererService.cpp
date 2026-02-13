#include "RendererService.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "backends/BgfxBackend.hpp"

namespace smgpc::render {
namespace {

constexpr std::uint16_t clamp_window_dimension(int value) {
    return static_cast<std::uint16_t>(std::clamp(value, 1, std::numeric_limits<int>::max() / 16));
}

class GLFWWindowService final : public IWindowService {
public:
    explicit GLFWWindowService(const WindowConfiguration &configuration) {
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

    GLFWWindowService(const GLFWWindowService &) = delete;
    GLFWWindowService &operator=(const GLFWWindowService &) = delete;

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
        if (_window == nullptr) {
            return true;
        }
        return glfwWindowShouldClose(_window);
    }

    [[nodiscard]] bool is_focused() const override {
        if (_window == nullptr) {
            return false;
        }
        return glfwGetWindowAttrib(_window, GLFW_FOCUSED) == GLFW_TRUE;
    }

    [[nodiscard]] bool is_minimized() const override {
        if (_window == nullptr) {
            return false;
        }
        return glfwGetWindowAttrib(_window, GLFW_ICONIFIED) == GLFW_TRUE;
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
            .window_handle = reinterpret_cast<void *>(static_cast<std::uintptr_t>(glfwGetX11Window(_window))),
            .display_handle = glfwGetX11Display(),
        };
#else
        return {reinterpret_cast<void *>(_window), nullptr};
#endif
    }

    void close() override {
        if (_window != nullptr) {
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
        }
    }

    [[nodiscard]] GLFWwindow *window_handle() const {
        return _window;
    }

private:
    GLFWwindow *_window {nullptr};
};

class GLFWWindowServiceFactory final : public IWindowFactory {
public:
    [[nodiscard]] std::unique_ptr<IWindowService> create(const WindowConfiguration &configuration) const override {
        return std::make_unique<GLFWWindowService>(configuration);
    }
};

class GLFWInputSnapshot final : public IInputSnapshot {
public:
    void refresh(GLFWwindow *window) {
        if (window == nullptr) {
            _keys.fill(false);
            return;
        }

        for (std::size_t key = 0U; key < _keys.size(); ++key) {
            _keys[key] = glfwGetKey(window, static_cast<int>(key)) == GLFW_PRESS;
        }
    }

    [[nodiscard]] bool is_key_down(int key) const override {
        if (key < 0 || key >= static_cast<int>(_keys.size())) {
            return false;
        }
        return _keys[static_cast<std::size_t>(key)];
    }

private:
    std::array<bool, 1024> _keys {};
};

class GLFWInputService final : public IInputService {
public:
    explicit GLFWInputService(GLFWwindow *window)
        : _window(window) {
    }

    [[nodiscard]] const IInputSnapshot &snapshot() const override {
        _snapshot.refresh(_window);
        return _snapshot;
    }

private:
    GLFWwindow *_window {nullptr};
    mutable GLFWInputSnapshot _snapshot {};
};

class RendererService final : public IRendererEngine {
public:
    RendererService(
        di::DependencyReference<IWindowService> window_service,
        di::DependencyReference<IInputService> input_service,
        std::unique_ptr<backends::IRenderBackend> backend)
        : _window_service(std::move(window_service)),
          _input_service(std::move(input_service)),
          _backend(std::move(backend)) {
        if (_backend == nullptr) {
            throw std::invalid_argument("Renderer backend cannot be null");
        }
    }

    RendererService(const RendererService &) = delete;
    RendererService &operator=(const RendererService &) = delete;

    [[nodiscard]] FrameContext begin_frame() override {
        if (not _backend->is_initialized()) {
            initialize_backend();
        }

        const auto now = std::chrono::steady_clock::now();
        const auto raw_delta = (_frame_anchor == std::chrono::steady_clock::time_point{})
            ? std::chrono::duration<double>::zero()
            : std::chrono::duration<double>(now - _frame_anchor);
        _frame_anchor = now;

        const double delta = std::max(0.0, raw_delta.count());
        _frame_time += delta;
        ++_frame_index;

        auto framebuffer = _window_service->framebuffer_size();
        if (framebuffer.width == 0U || framebuffer.height == 0U) {
            framebuffer = FramebufferInfo {1U, 1U};
        }

        if (not _framebuffer_ready || framebuffer.width != _framebuffer.width || framebuffer.height != _framebuffer.height) {
            _backend->resize(framebuffer.width, framebuffer.height);
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
            .input_snapshot = &_input_service->snapshot(),
        };

        _backend->begin_frame(context);
        return context;
    }

    void submit(const RenderCommandBuffer &commands) override {
        if (not _backend->is_initialized()) {
            return;
        }
        _backend->execute(commands);
    }

    void submit(std::span<const RenderCommandBuffer> passes) override {
        if (not _backend->is_initialized()) {
            return;
        }
        for (const auto &commands : passes) {
            _backend->execute(commands);
        }
    }

    void end_frame() override {
        if (not _backend->is_initialized()) {
            return;
        }
        _backend->end_frame();
    }

    void request_capture(RenderCaptureRequest request) override {
        if (not _backend->is_initialized()) {
            return;
        }
        _backend->request_capture(request);
    }

    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() override {
        if (not _backend->is_initialized()) {
            return std::nullopt;
        }
        return _backend->poll_completed_capture();
    }

    [[nodiscard]] FramebufferInfo framebuffer_size() const override {
        if (not _backend->is_initialized()) {
            return _window_service->framebuffer_size();
        }
        return _backend->framebuffer_size();
    }

private:
    void initialize_backend() {
        const auto framebuffer = _window_service->framebuffer_size();
        const auto native_handle = _window_service->native_handle();

        core::RenderInitDesc init_desc {};
        init_desc.width = static_cast<int>(framebuffer.width);
        init_desc.height = static_cast<int>(framebuffer.height);
        init_desc.title = "SMG PC Port";
        init_desc.enable_vsync = true;
        init_desc.native_window_handle = native_handle.window_handle;
        init_desc.native_display_handle = native_handle.display_handle;

        _backend->initialize(init_desc);
        _framebuffer = framebuffer;
        _framebuffer_ready = true;
        _frame_anchor = std::chrono::steady_clock::now();
    }

    di::DependencyReference<IWindowService> _window_service;
    di::DependencyReference<IInputService> _input_service;
    std::unique_ptr<backends::IRenderBackend> _backend {};

    std::uint64_t _frame_index {};
    double _frame_time {};
    bool _framebuffer_ready {};
    FramebufferInfo _framebuffer {1U, 1U};
    std::chrono::steady_clock::time_point _frame_anchor {};
};

}  // namespace

std::unique_ptr<IWindowFactory> create_default_window_factory(di::DependencyReference<logging::ILogger>) {
    return std::make_unique<GLFWWindowServiceFactory>();
}

std::unique_ptr<IInputService> create_default_input_service(
    di::DependencyReference<IWindowService> window_service,
    di::DependencyReference<logging::ILogger>) {
    auto *const window_service_ptr = dynamic_cast<GLFWWindowService *>(std::addressof(window_service.get()));
    if (window_service_ptr == nullptr) {
        throw std::invalid_argument("Default input service currently supports only GLFWWindowService instances");
    }

    return std::make_unique<GLFWInputService>(window_service_ptr->window_handle());
}

std::unique_ptr<IRendererEngine> create_default_renderer_engine(
    di::DependencyReference<IWindowService> window_service,
    di::DependencyReference<IInputService> input_service,
    di::DependencyReference<logging::ILogger>) {
    return std::make_unique<RendererService>(
        std::move(window_service),
        std::move(input_service),
        std::make_unique<backends::BgfxBackend>());
}

}  // namespace smgpc::render
