#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

#include "../core/RenderCommandBuffer.hpp"

namespace smgpc::render::backends {

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    virtual void initialize(const smgpc::render::core::RenderInitDesc &description) = 0;
    virtual void shutdown() = 0;
    virtual void begin_frame(const smgpc::render::core::FrameContext &frame_context) = 0;
    virtual void execute(const smgpc::render::core::RenderCommandBuffer &commands) = 0;
    virtual void end_frame() = 0;
    virtual void resize(std::uint16_t width, std::uint16_t height) = 0;
    virtual void request_capture(const smgpc::render::core::RenderCaptureRequest &request) = 0;
    [[nodiscard]] virtual std::optional<std::filesystem::path> poll_completed_capture() = 0;
    [[nodiscard]] virtual smgpc::render::core::FramebufferInfo framebuffer_size() const = 0;
    [[nodiscard]] virtual bool is_initialized() const = 0;
};

}  // namespace smgpc::render::backends
