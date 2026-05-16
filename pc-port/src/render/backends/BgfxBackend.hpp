#pragma once

#include <cstdint>

#include "core/RenderTypes.hpp"

namespace smgpc::render::backends {

class BgfxBackend final {
public:
    BgfxBackend() = default;
    ~BgfxBackend();

    BgfxBackend(const BgfxBackend &) = delete;
    BgfxBackend &operator=(const BgfxBackend &) = delete;

    void initialize(const core::RenderInitDesc &description);
    void shutdown();
    void begin_frame(const core::FrameContext &frame_context);
    void end_frame();
    void resize(std::uint16_t width, std::uint16_t height);

    [[nodiscard]] core::FramebufferInfo framebuffer_size() const;
    [[nodiscard]] bool is_initialized() const;

private:
    bool _initialized = false;
    bool _vsync_enabled = true;
    std::uint16_t _framebuffer_width = 1U;
    std::uint16_t _framebuffer_height = 1U;
};

}  // namespace smgpc::render::backends
