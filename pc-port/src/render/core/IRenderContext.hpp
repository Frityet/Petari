#pragma once

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

    virtual void close() {}
};

class IRendererEngine {
public:
    virtual ~IRendererEngine() = default;

    [[nodiscard]] virtual FrameContext begin_frame() = 0;
    virtual void end_frame() = 0;
    [[nodiscard]] virtual FramebufferInfo framebuffer_size() const = 0;
};

}  // namespace smgpc::render::core
