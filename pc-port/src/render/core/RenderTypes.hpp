#pragma once

#include <cstdint>
#include <string>

namespace smgpc::render::core {

struct WindowConfiguration {
    int width = 800;
    int height = 600;
    std::string title = "SMG PC Port";
};

struct RenderInitDesc : WindowConfiguration {
    bool enable_vsync = true;
    void *native_window_handle = nullptr;
    void *native_display_handle = nullptr;
};

struct FramebufferInfo {
    std::uint16_t width = 1U;
    std::uint16_t height = 1U;
};

struct FrameContext {
    std::uint64_t frame_index = 0;
    double frame_time_seconds = 0;
    double frame_delta_seconds = 0;
    FramebufferInfo framebuffer = {};
    bool has_focus = true;
    bool is_minimized = false;
};

struct NativeWindowHandle {
    void *window_handle = nullptr;
    void *display_handle = nullptr;
};

}  // namespace smgpc::render::core
