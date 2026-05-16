#include "LayoutRenderPass.hpp"

#include <cstdlib>
#include <utility>

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

smgpc::render::core::RenderTriangleTextureCombineMode LayoutRenderPass::map_triangle_texture_combine_mode(
    TriangleTextureCombineMode mode) {
    switch (mode) {
    case TriangleTextureCombineMode::Multiply:
        return smgpc::render::core::RenderTriangleTextureCombineMode::Multiply;
    case TriangleTextureCombineMode::Add:
        return smgpc::render::core::RenderTriangleTextureCombineMode::Add;
    case TriangleTextureCombineMode::Screen:
        return smgpc::render::core::RenderTriangleTextureCombineMode::Screen;
    case TriangleTextureCombineMode::J3dTevColorStages:
        return smgpc::render::core::RenderTriangleTextureCombineMode::J3dTevColorStages;
    case TriangleTextureCombineMode::None:
    default:
        return smgpc::render::core::RenderTriangleTextureCombineMode::None;
    }
}

smgpc::render::core::RenderLayoutDrawItemKind LayoutRenderPass::map_draw_command_kind(DrawCommandKind kind) {
    return kind == DrawCommandKind::TriangleBatch
        ? smgpc::render::core::RenderLayoutDrawItemKind::TriangleBatch
        : smgpc::render::core::RenderLayoutDrawItemKind::Quad;
}

void LayoutRenderPass::record(
    smgpc::render::core::RenderCommandBuffer &commands,
    const LayoutDrawList &draw_list,
    std::uint16_t framebuffer_width,
    std::uint16_t framebuffer_height,
    float layout_width,
    float layout_height,
    bool clear_framebuffer,
    std::uint8_t view_id) {
    commands.clear();
    commands.set_viewport(view_id, 0U, 0U, framebuffer_width, framebuffer_height);
    commands.set_scissor(view_id, 0U, 0U, framebuffer_width, framebuffer_height);
    if (clear_framebuffer) {
        commands.clear_view(
            view_id,
            0x000000ffU,
            1.0F,
            0U);
    }

    smgpc::render::core::RenderDrawLayoutCommand command {
        .view_id = view_id,
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
                .coordinate_width = quad.coordinate_width,
                .coordinate_height = quad.coordinate_height,
                .use_custom_vertices = quad.use_custom_vertices,
                .x_tl = quad.x_tl,
                .y_tl = quad.y_tl,
                .x_tr = quad.x_tr,
                .y_tr = quad.y_tr,
                .x_bl = quad.x_bl,
                .y_bl = quad.y_bl,
                .x_br = quad.x_br,
                .y_br = quad.y_br,
                .u0 = quad.u0,
                .v0 = quad.v0,
                .q0 = quad.q0,
                .u1 = quad.u1,
                .v1 = quad.v1,
                .q1 = quad.q1,
                .u0_secondary = quad.u0_secondary,
                .v0_secondary = quad.v0_secondary,
                .q0_secondary = quad.q0_secondary,
                .u1_secondary = quad.u1_secondary,
                .v1_secondary = quad.v1_secondary,
                .q1_secondary = quad.q1_secondary,
                .use_custom_tex_coords = quad.use_custom_tex_coords,
                .u_tl = quad.u_tl,
                .v_tl = quad.v_tl,
                .q_tl = quad.q_tl,
                .u_tr = quad.u_tr,
                .v_tr = quad.v_tr,
                .q_tr = quad.q_tr,
                .u_bl = quad.u_bl,
                .v_bl = quad.v_bl,
                .q_bl = quad.q_bl,
                .u_br = quad.u_br,
                .v_br = quad.v_br,
                .q_br = quad.q_br,
                .u_tl_secondary = quad.u_tl_secondary,
                .v_tl_secondary = quad.v_tl_secondary,
                .q_tl_secondary = quad.q_tl_secondary,
                .u_tr_secondary = quad.u_tr_secondary,
                .v_tr_secondary = quad.v_tr_secondary,
                .q_tr_secondary = quad.q_tr_secondary,
                .u_bl_secondary = quad.u_bl_secondary,
                .v_bl_secondary = quad.v_bl_secondary,
                .q_bl_secondary = quad.q_bl_secondary,
                .u_br_secondary = quad.u_br_secondary,
                .v_br_secondary = quad.v_br_secondary,
                .q_br_secondary = quad.q_br_secondary,
                .color_tl = quad.color_tl,
                .color_tr = quad.color_tr,
                .color_bl = quad.color_bl,
                .color_br = quad.color_br,
                .blend_mode = map_blend_mode(quad.blend_mode),
                .use_mask_texture = quad.use_mask_texture,
                .invert_mask = quad.invert_mask,
                .mask_uses_alpha = quad.mask_uses_alpha,
                .texture_alpha_only = quad.texture_alpha_only,
                .texture_color_lerp = quad.texture_color_lerp,
                .tev_color0 = quad.tev_color0,
                .tev_color1 = quad.tev_color1,
                .tev_color_scale = quad.tev_color_scale,
                .texture = {
                    .id = quad.texture.id,
                    .rgba8 = quad.texture.rgba8,
                    .width = quad.texture.width,
                    .height = quad.texture.height,
                    .wrap_s = quad.texture.wrap_s,
                    .wrap_t = quad.texture.wrap_t,
                },
                .mask_texture = {
                    .id = quad.mask_texture.id,
                    .rgba8 = quad.mask_texture.rgba8,
                    .width = quad.mask_texture.width,
                    .height = quad.mask_texture.height,
                    .wrap_s = quad.mask_texture.wrap_s,
                    .wrap_t = quad.mask_texture.wrap_t,
                },
            });
        }

        command.triangle_batches.reserve(draw_list.triangle_batches().size());
        for (const auto &batch : draw_list.triangle_batches()) {
            smgpc::render::core::RenderLayoutTriangleBatch mapped {
                .coordinate_width = batch.coordinate_width,
                .coordinate_height = batch.coordinate_height,
                .blend_mode = map_blend_mode(batch.blend_mode),
                .secondary_texture_mode = map_triangle_texture_combine_mode(batch.secondary_texture_mode),
                .tev_color0 = batch.tev_color0,
                .tev_color1 = batch.tev_color1,
                .tev_stage_count = batch.tev_stage_count,
                .tev_stages = {
                    smgpc::render::core::RenderLayoutTriangleTevStage{
                        .color_args =
                            {
                                .a = batch.tev_stages[0U].color_args.a,
                                .b = batch.tev_stages[0U].color_args.b,
                                .c = batch.tev_stages[0U].color_args.c,
                                .d = batch.tev_stages[0U].color_args.d,
                            },
                        .color_op =
                            {
                                .op = batch.tev_stages[0U].color_op.op,
                                .bias = batch.tev_stages[0U].color_op.bias,
                                .scale = batch.tev_stages[0U].color_op.scale,
                                .clamp = batch.tev_stages[0U].color_op.clamp,
                                .output_register = batch.tev_stages[0U].color_op.output_register,
                            },
                    },
                    smgpc::render::core::RenderLayoutTriangleTevStage{
                        .color_args =
                            {
                                .a = batch.tev_stages[1U].color_args.a,
                                .b = batch.tev_stages[1U].color_args.b,
                                .c = batch.tev_stages[1U].color_args.c,
                                .d = batch.tev_stages[1U].color_args.d,
                            },
                        .color_op =
                            {
                                .op = batch.tev_stages[1U].color_op.op,
                                .bias = batch.tev_stages[1U].color_op.bias,
                                .scale = batch.tev_stages[1U].color_op.scale,
                                .clamp = batch.tev_stages[1U].color_op.clamp,
                                .output_register = batch.tev_stages[1U].color_op.output_register,
                            },
                    },
                },
                .texture = {
                    .id = batch.texture.id,
                    .rgba8 = batch.texture.rgba8,
                    .width = batch.texture.width,
                    .height = batch.texture.height,
                    .wrap_s = batch.texture.wrap_s,
                    .wrap_t = batch.texture.wrap_t,
                },
                .secondary_texture = {
                    .id = batch.secondary_texture.id,
                    .rgba8 = batch.secondary_texture.rgba8,
                    .width = batch.secondary_texture.width,
                    .height = batch.secondary_texture.height,
                    .wrap_s = batch.secondary_texture.wrap_s,
                    .wrap_t = batch.secondary_texture.wrap_t,
                },
            };
            mapped.vertices.reserve(batch.vertices.size());
            for (const auto &vertex : batch.vertices) {
                mapped.vertices.push_back(smgpc::render::core::RenderLayoutTriangleVertex {
                    .x = vertex.x,
                    .y = vertex.y,
                    .z = vertex.z,
                    .u = vertex.u,
                    .v = vertex.v,
                    .q = vertex.q,
                    .u_secondary = vertex.u_secondary,
                    .v_secondary = vertex.v_secondary,
                    .q_secondary = vertex.q_secondary,
                    .color = vertex.color,
                });
            }
            command.triangle_batches.push_back(std::move(mapped));
        }

        command.draw_order.reserve(draw_list.draw_order().size());
        for (const auto &entry : draw_list.draw_order()) {
            command.draw_order.push_back(smgpc::render::core::RenderLayoutDrawItem {
                .kind = map_draw_command_kind(entry.kind),
                .index = entry.index,
            });
        }
    }

    if (_logger && command.debug_trace) {
        _logger->debug(
            __FILE__,
            __LINE__,
            logging::Category::RENDERER,
            "layout command: fb={}x{}, quads={}, triangle_batches={}, layout={}x{}, solid={}, touch_only={}, force_touch={}",
            framebuffer_width,
            framebuffer_height,
            command.quads.size(),
            command.triangle_batches.size(),
            layout_width,
            layout_height,
            command.debug_solid_quad,
            command.debug_touch_only,
            command.debug_force_touch);
    }

    if (command.debug_touch_only || command.debug_solid_quad || command.debug_force_touch || not command.quads.empty() ||
        not command.triangle_batches.empty()) {
        commands.draw_layout(command);
    }
}

}  // namespace smgpc::render::layout
