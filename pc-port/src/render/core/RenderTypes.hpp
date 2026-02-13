#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace smgpc::render::core {

class IInputSnapshot;

struct WindowConfiguration {
    int width {800};
    int height {600};
    std::string title {"SMG PC Port"};
};

struct RenderInitDesc : WindowConfiguration {
    bool enable_vsync {true};
    void *native_window_handle {nullptr};
    void *native_display_handle {nullptr};
};

enum class RenderBlendMode : std::uint8_t {
    Alpha,
    Additive,
};

struct FramebufferInfo {
    std::uint16_t width {1U};
    std::uint16_t height {1U};
};

struct FrameContext {
    std::uint64_t frame_index {};
    double frame_time_seconds {};
    double frame_delta_seconds {};
    FramebufferInfo framebuffer {};
    bool has_focus {true};
    bool is_minimized {false};
    const IInputSnapshot *input_snapshot {nullptr};
};

struct NativeWindowHandle {
    void *window_handle {};
    void *display_handle {};
};

struct RenderCaptureRequest {
    std::filesystem::path path {};
};

}  // namespace smgpc::render::core
