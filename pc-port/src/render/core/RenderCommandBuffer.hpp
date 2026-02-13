#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <span>

#include "RenderTypes.hpp"

namespace smgpc::render::core {

struct RenderClearCommand {
    std::uint8_t view_id {0U};
    bool clear_color {true};
    bool clear_depth {true};
    bool clear_stencil {false};
    std::uint32_t color_value {0x000000ffU};
    float depth_value {1.0F};
    std::uint8_t stencil_value {0U};
};

struct RenderViewportCommand {
    std::uint8_t view_id {0U};
    std::uint16_t x {0U};
    std::uint16_t y {0U};
    std::uint16_t width {1U};
    std::uint16_t height {1U};
    float min_depth {0.0F};
    float max_depth {1.0F};
};

struct RenderScissorCommand {
    std::uint8_t view_id {0U};
    std::uint16_t x {0U};
    std::uint16_t y {0U};
    std::uint16_t width {1U};
    std::uint16_t height {1U};
};

struct RenderTextureRef {
    std::uint64_t id {0U};
    const std::uint8_t *rgba8 {nullptr};
    std::uint16_t width {0U};
    std::uint16_t height {0U};
};

struct RenderLayoutQuad {
    float x0 {};
    float y0 {};
    float x1 {};
    float y1 {};

    float u0 {};
    float v0 {};
    float u1 {};
    float v1 {};

    float u0_secondary {};
    float v0_secondary {};
    float u1_secondary {};
    float v1_secondary {};

    std::uint32_t color_tl {};
    std::uint32_t color_tr {};
    std::uint32_t color_bl {};
    std::uint32_t color_br {};
    RenderBlendMode blend_mode {RenderBlendMode::Alpha};

    bool use_mask_texture {};
    bool invert_mask {};
    bool mask_uses_alpha {};

    RenderTextureRef texture {};
    RenderTextureRef mask_texture {};
};

struct RenderDrawLayoutCommand {
    std::uint16_t framebuffer_width {1U};
    std::uint16_t framebuffer_height {1U};
    float layout_width {1.0F};
    float layout_height {1.0F};

    bool debug_solid_quad {false};
    bool debug_force_touch {false};
    bool debug_touch_only {false};
    bool debug_trace {false};

    std::vector<RenderLayoutQuad> quads {};
};

struct RenderBindTextureCommand {
    std::uint8_t view_id {0U};
    std::uint8_t sampler_slot {0U};
    RenderTextureRef texture {};
};

struct RenderMarkerCommand {
    std::string label {};
};

struct RenderBarrierCommand {
    std::uint8_t view_id {0U};
};

enum class RenderCommandType {
    Marker,
    Clear,
    SetViewport,
    SetScissor,
    DrawLayout,
    BindTexture,
    Barrier,
};

using RenderCommandPayload = std::variant<
    RenderMarkerCommand,
    RenderClearCommand,
    RenderViewportCommand,
    RenderScissorCommand,
    RenderDrawLayoutCommand,
    RenderBindTextureCommand,
    RenderBarrierCommand>;

struct RenderCommand {
    RenderCommandType type {RenderCommandType::Marker};
    RenderCommandPayload payload;
};

class RenderCommandBuffer {
public:
    using CommandList = std::vector<RenderCommand>;

    void clear() {
        _commands.clear();
    }

    void reserve(std::size_t command_count) {
        _commands.reserve(command_count);
    }

    void marker(std::string_view label) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::Marker,
            .payload = RenderMarkerCommand {.label = std::string(label)},
        });
    }

    void clear_view(std::uint8_t view_id, std::uint32_t color, float depth = 1.0F, std::uint8_t stencil = 0U) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::Clear,
            .payload = RenderClearCommand {
                .view_id = view_id,
                .color_value = color,
                .depth_value = depth,
                .stencil_value = stencil,
            },
        });
    }

    void set_viewport(
        std::uint8_t view_id,
        std::uint16_t x,
        std::uint16_t y,
        std::uint16_t width,
        std::uint16_t height,
        float min_depth = 0.0F,
        float max_depth = 1.0F) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::SetViewport,
            .payload = RenderViewportCommand {
                .view_id = view_id,
                .x = x,
                .y = y,
                .width = width,
                .height = height,
                .min_depth = min_depth,
                .max_depth = max_depth,
            },
        });
    }

    void set_scissor(std::uint8_t view_id, std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::SetScissor,
            .payload = RenderScissorCommand {
                .view_id = view_id,
                .x = x,
                .y = y,
                .width = width,
                .height = height,
            },
        });
    }

    void draw_layout(const RenderDrawLayoutCommand &command) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::DrawLayout,
            .payload = command,
        });
    }

    void bind_texture(std::uint8_t view_id, std::uint8_t sampler_slot, const RenderTextureRef &texture) {
        _commands.push_back(RenderCommand {
            .type = RenderCommandType::BindTexture,
            .payload = RenderBindTextureCommand {
                .view_id = view_id,
                .sampler_slot = sampler_slot,
                .texture = texture,
            },
        });
    }

    [[nodiscard]] std::span<const RenderCommand> commands() const {
        return std::span(_commands.data(), _commands.size());
    }

private:
    CommandList _commands {};
};

}  // namespace smgpc::render::core
