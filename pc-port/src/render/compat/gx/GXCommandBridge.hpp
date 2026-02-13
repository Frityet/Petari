#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "core/RenderCommandBuffer.hpp"
#include "layout/LayoutDrawList.hpp"
#include "GXCompatState.hpp"

namespace smgpc::render::compat::gx {

using smgpc::render::core::RenderCommandBuffer;
using smgpc::render::core::RenderLayoutQuad;
using smgpc::render::core::RenderDrawLayoutCommand;
using smgpc::render::layout::QuadCommand;
using smgpc::render::layout::BlendMode;

class IGXCommandSink {
public:
    virtual ~IGXCommandSink() = default;
    virtual void begin_pass() = 0;
    virtual void end_pass() = 0;
};

class GXCommandBridge final : public IGXCommandSink {
public:
    explicit GXCommandBridge(RenderCommandBuffer &commands)
        : _commands(commands) {
    }

    [[nodiscard]] const GXCompatState &state() const {
        return _state;
    }

    void begin_pass() override;
    void end_pass() override;

    void clear_view(std::uint8_t view_id, std::uint32_t color = 0x000000ffU, float depth = 1.0F, std::uint8_t stencil = 0U);
    void set_viewport(std::uint8_t view_id, std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height, float min_depth = 0.0F, float max_depth = 1.0F);
    void set_scissor(std::uint8_t view_id, std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height);
    void marker(std::string_view label);
    void bind_texture(std::uint8_t view_id, std::uint8_t sampler_slot, std::uint64_t texture_id, const std::uint8_t *rgba8, std::uint16_t width, std::uint16_t height);
    void emit_layout(
        std::uint16_t framebuffer_width,
        std::uint16_t framebuffer_height,
        float layout_width,
        float layout_height,
        std::span<const QuadCommand> quads,
        bool debug_solid_quad = false,
        bool debug_force_touch = false,
        bool debug_touch_only = false,
        bool debug_trace = false);
    void emit_marker(std::string_view label);

private:
    [[nodiscard]] static RenderLayoutQuad map_quad(const QuadCommand &quad);
    [[nodiscard]] static smgpc::render::core::RenderBlendMode map_blend_mode(BlendMode blend_mode);

    RenderCommandBuffer &_commands;
    GXCompatState _state {};
};

}  // namespace smgpc::render::compat::gx
