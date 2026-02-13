#include "LayoutRenderPass.hpp"

#include <cstdlib>

#include "Logger.hpp"

namespace smgpc::render::layout {
namespace {

[[nodiscard]] bool env_enabled(const char *value) {
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

}  // namespace

LayoutRenderPass::LayoutRenderPass(smgpc::di::OptionalDependencyReference<smgpc::logging::ILogger> logger)
    : _logger(logger) {
}

bool LayoutRenderPass::env_is_enabled(const char *name) {
    return env_enabled(std::getenv(name));
}

smgpc::render::core::RenderBlendMode LayoutRenderPass::map_blend_mode(BlendMode blend_mode) {
    return blend_mode == BlendMode::Additive
        ? smgpc::render::core::RenderBlendMode::Additive
        : smgpc::render::core::RenderBlendMode::Alpha;
}

void LayoutRenderPass::record(
    smgpc::render::core::RenderCommandBuffer &commands,
    const LayoutDrawList &draw_list,
    std::uint16_t framebuffer_width,
    std::uint16_t framebuffer_height,
    float layout_width,
    float layout_height) {
    commands.clear();
    commands.set_viewport(0U, 0U, 0U, framebuffer_width, framebuffer_height);
    commands.set_scissor(0U, 0U, 0U, framebuffer_width, framebuffer_height);
    commands.clear_view(
        0U,
        0x000000ffU,
        1.0F,
        0U);

    smgpc::render::core::RenderDrawLayoutCommand command {
        .framebuffer_width = framebuffer_width,
        .framebuffer_height = framebuffer_height,
        .layout_width = layout_width,
        .layout_height = layout_height,
        .debug_solid_quad = env_is_enabled("SMGPC_DEBUG_SOLID_QUAD"),
        .debug_force_touch = env_is_enabled("SMGPC_DEBUG_FORCE_TOUCH"),
        .debug_touch_only = env_is_enabled("SMGPC_DEBUG_TOUCH_ONLY"),
        .debug_trace = env_is_enabled("SMGPC_DEBUG_LAYOUT_TRACE"),
    };

    if (not command.debug_solid_quad) {
        command.quads.reserve(draw_list.quads().size());
        for (const auto &quad : draw_list.quads()) {
            command.quads.push_back(smgpc::render::core::RenderLayoutQuad {
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
            });
        }
    }

    if (_logger && command.debug_trace) {
        _logger->debug(
            __FILE__,
            __LINE__,
            logging::Category::RENDERER,
            "layout command: fb={}x{}, quads={}, layout={}x{}, solid={}, touch_only={}, force_touch={}",
            framebuffer_width,
            framebuffer_height,
            command.quads.size(),
            layout_width,
            layout_height,
            command.debug_solid_quad,
            command.debug_touch_only,
            command.debug_force_touch);
    }

    if (command.debug_touch_only || command.debug_solid_quad || command.debug_force_touch || not command.quads.empty()) {
        commands.draw_layout(command);
    }
}

}  // namespace smgpc::render::layout
