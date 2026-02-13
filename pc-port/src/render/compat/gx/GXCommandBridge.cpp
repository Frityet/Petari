#include "GXCommandBridge.hpp"

namespace smgpc::render::compat::gx {

void GXCommandBridge::begin_pass() {
    _state.reset();
}

void GXCommandBridge::end_pass() {
}

void GXCommandBridge::clear_view(std::uint8_t view_id, std::uint32_t color, float depth, std::uint8_t stencil) {
    _commands.clear_view(view_id, color, depth, stencil);
}

void GXCommandBridge::set_viewport(std::uint8_t view_id, std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height, float min_depth, float max_depth) {
    _state.set_viewport(x, y, width, height);
    _commands.set_viewport(view_id, x, y, width, height, min_depth, max_depth);
}

void GXCommandBridge::set_scissor(std::uint8_t view_id, std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height) {
    _state.set_scissor(x, y, width, height);
    _commands.set_scissor(view_id, x, y, width, height);
}

void GXCommandBridge::marker(std::string_view label) {
    _commands.marker(label);
}

void GXCommandBridge::bind_texture(std::uint8_t view_id, std::uint8_t sampler_slot, std::uint64_t texture_id, const std::uint8_t *rgba8, std::uint16_t width, std::uint16_t height) {
    _state.set_bound_texture_id(texture_id, sampler_slot);
    _commands.bind_texture(view_id, sampler_slot, {
        .id = texture_id,
        .rgba8 = rgba8,
        .width = width,
        .height = height,
    });
}

smgpc::render::core::RenderBlendMode GXCommandBridge::map_blend_mode(BlendMode blend_mode) {
    return blend_mode == BlendMode::Additive ? smgpc::render::core::RenderBlendMode::Additive : smgpc::render::core::RenderBlendMode::Alpha;
}

smgpc::render::core::RenderLayoutQuad GXCommandBridge::map_quad(const QuadCommand &quad) {
    return smgpc::render::core::RenderLayoutQuad {
        .x0 = quad.x0,
        .y0 = quad.y0,
        .x1 = quad.x1,
        .y1 = quad.y1,
        .u0 = quad.u0,
        .v0 = quad.v0,
        .u1 = quad.u1,
        .v1 = quad.v1,
        .u0_secondary = quad.u0_secondary,
        .v0_secondary = quad.v0_secondary,
        .u1_secondary = quad.u1_secondary,
        .v1_secondary = quad.v1_secondary,
        .color_tl = quad.color_tl,
        .color_tr = quad.color_tr,
        .color_bl = quad.color_bl,
        .color_br = quad.color_br,
        .blend_mode = map_blend_mode(quad.blend_mode),
        .use_mask_texture = quad.use_mask_texture,
        .invert_mask = quad.invert_mask,
        .mask_uses_alpha = quad.mask_uses_alpha,
        .texture = {
            .id = quad.texture.id,
            .rgba8 = quad.texture.rgba8,
            .width = quad.texture.width,
            .height = quad.texture.height,
        },
        .mask_texture = {
            .id = quad.mask_texture.id,
            .rgba8 = quad.mask_texture.rgba8,
            .width = quad.mask_texture.width,
            .height = quad.mask_texture.height,
        },
    };
}

void GXCommandBridge::emit_layout(
    std::uint16_t framebuffer_width,
    std::uint16_t framebuffer_height,
    float layout_width,
    float layout_height,
    std::span<const QuadCommand> quads,
    bool debug_solid_quad,
    bool debug_force_touch,
    bool debug_touch_only,
    bool debug_trace) {
    RenderDrawLayoutCommand command {
        .framebuffer_width = framebuffer_width,
        .framebuffer_height = framebuffer_height,
        .layout_width = layout_width,
        .layout_height = layout_height,
        .debug_solid_quad = debug_solid_quad,
        .debug_force_touch = debug_force_touch,
        .debug_touch_only = debug_touch_only,
        .debug_trace = debug_trace,
    };
    command.quads.reserve(quads.size());
    for (const auto &quad : quads) {
        command.quads.push_back(map_quad(quad));
    }
    _commands.draw_layout(command);
}

void GXCommandBridge::emit_marker(std::string_view label) {
    _commands.marker(label);
}

}  // namespace smgpc::render::compat::gx
