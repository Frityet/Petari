#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>

#include "RenderTypes.hpp"

namespace smgpc::render::core {

class RenderCommandBuffer;

class IInputSnapshot {
public:
    virtual ~IInputSnapshot() = default;

    [[nodiscard]] virtual bool is_key_down(int key) const = 0;
};

class IInputService {
public:
    virtual ~IInputService() = default;

    [[nodiscard]] virtual const IInputSnapshot &snapshot() const = 0;

    [[nodiscard]] virtual bool is_key_down(int key) const {
        return snapshot().is_key_down(key);
    }
};

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
    virtual void submit(const RenderCommandBuffer &commands) = 0;
    virtual void submit(std::span<const RenderCommandBuffer> passes) = 0;
    virtual void end_frame() = 0;
    virtual void request_capture(RenderCaptureRequest request) = 0;
    [[nodiscard]] virtual std::optional<std::filesystem::path> poll_completed_capture() = 0;
    [[nodiscard]] virtual FramebufferInfo framebuffer_size() const = 0;
};

}  // namespace smgpc::render::core
