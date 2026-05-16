#include "render/core/RenderCommandBuffer.hpp"
#include "render/layout/LayoutDrawList.hpp"
#include "render/layout/LayoutRenderPass.hpp"

#include <array>

#include "tests/TestHarness.hpp"

namespace {

using smgpc::render::core::RenderCommandBuffer;
using smgpc::render::core::RenderClearCommand;
using smgpc::render::core::RenderDrawJ3dCommand;
using smgpc::render::core::RenderDrawLayoutCommand;
using smgpc::render::core::RenderJ3dMaterialBatch;
using smgpc::render::core::RenderJ3dVertex;
using smgpc::render::core::RenderLayoutDrawItemKind;
using smgpc::render::core::RenderMarkerCommand;
using smgpc::render::core::RenderScissorCommand;
using smgpc::render::core::RenderViewportCommand;

}  // namespace

$test("Render::RenderCommandBuffer preserves command emission order") {
    RenderCommandBuffer commands {};
    commands.set_viewport(0U, 1U, 2U, 100U, 50U);
    commands.set_scissor(0U, 3U, 4U, 20U, 10U);
    commands.clear_view(0U, 0x11223344U);
    commands.marker("layout");

    auto emitted = commands.commands();
    $pc_port_require_eq(emitted.size(), 4ULL);

    const auto &viewport = std::get<RenderViewportCommand>(emitted[0].payload);
    $pc_port_require_eq(viewport.view_id, 0U);
    $pc_port_require_eq(viewport.x, 1U);
    $pc_port_require_eq(viewport.y, 2U);
    $pc_port_require_eq(viewport.width, 100U);
    $pc_port_require_eq(viewport.height, 50U);

    const auto &scissor = std::get<RenderScissorCommand>(emitted[1].payload);
    $pc_port_require_eq(scissor.view_id, 0U);
    $pc_port_require_eq(scissor.x, 3U);
    $pc_port_require_eq(scissor.y, 4U);
    $pc_port_require_eq(scissor.width, 20U);
    $pc_port_require_eq(scissor.height, 10U);

    const auto &clear = std::get<RenderClearCommand>(emitted[2].payload);
    $pc_port_require_eq(clear.view_id, 0U);
    $pc_port_require_eq(clear.color_value, 0x11223344U);

    const auto &marker = std::get<RenderMarkerCommand>(emitted[3].payload);
    $pc_port_require(marker.label == "layout");
}

$test("Render::RenderCommandBuffer preserves world-space J3D commands") {
    static const std::array<std::uint8_t, 16U> texture {
        255U, 255U, 255U, 255U,
        255U, 255U, 255U, 255U,
        255U, 255U, 255U, 255U,
        255U, 255U, 255U, 255U,
    };

    RenderJ3dMaterialBatch batch {
        .blend_mode = smgpc::render::core::RenderBlendMode::Additive,
        .cull_mode = smgpc::render::core::RenderCullMode::None,
        .depth_mode = smgpc::render::core::RenderDepthMode::Less,
        .write_depth = false,
        .secondary_texture_mode = smgpc::render::core::RenderTriangleTextureCombineMode::J3dTevColorStages,
        .tev_color0 = 0x10203040U,
        .tev_color1 = 0x50607080U,
        .tev_stage_count = 2U,
        .texture = {.id = 11U, .rgba8 = texture.data(), .width = 2U, .height = 2U, .wrap_s = 1U, .wrap_t = 0U},
        .secondary_texture = {.id = 12U, .rgba8 = texture.data(), .width = 2U, .height = 2U, .wrap_s = 2U, .wrap_t = 1U},
        .vertices = {
            RenderJ3dVertex {.x = 0.0F, .y = 1.0F, .z = 2.0F, .u = 0.0F, .v = 0.0F, .color = 0xFFFFFFFFU},
            RenderJ3dVertex {.x = 3.0F, .y = 4.0F, .z = 5.0F, .u = 1.0F, .v = 0.0F, .color = 0xFFFFFFFFU},
            RenderJ3dVertex {.x = 6.0F, .y = 7.0F, .z = 8.0F, .u = 0.0F, .v = 1.0F, .color = 0xFFFFFFFFU},
        },
    };
    batch.tev_stages[0U].color_args = {.a = 15U, .b = 8U, .c = 10U, .d = 14U};
    batch.tev_stages[0U].color_op = {.clamp = 1U};
    batch.tev_stages[1U].color_args = {.a = 15U, .b = 10U, .c = 8U, .d = 0U};
    batch.tev_stages[1U].color_op = {.clamp = 1U};
    batch.tev_stage_texture_swizzles[1U].channels = {2U, 2U, 2U, 3U};
    batch.tev_stage_raster_swizzles[1U].channels = {1U, 1U, 1U, 3U};
    batch.alpha_compare = {
        .valid = true,
        .comp0 = 7U,
        .ref0 = 0U,
        .op = 1U,
        .comp1 = 7U,
        .ref1 = 0U,
    };
    batch.indirect_texture_stage_count = 1U;
    batch.indirect_texture_orders[0U] = {
        .valid = true,
        .texture_index = 1U,
        .texture_coordinate = 1U,
        .texture_map = 1U,
    };
    batch.indirect_texture_matrices[0U] = {
        .valid = true,
        .values = {0.5F, 0.0F, 0.0F, 0.0F, 0.5F, 0.0F},
        .scale_exponent = -2,
    };
    batch.indirect_tev_stages[0U] = {
        .valid = true,
        .ind_stage = 0U,
        .format = 0U,
        .bias = 7U,
        .matrix = 1U,
    };

    RenderDrawJ3dCommand draw {
        .view_id = 3U,
        .framebuffer_width = 640U,
        .framebuffer_height = 480U,
    };
    draw.view_matrix[12U] = 15.0F;
    draw.projection_matrix[0U] = 2.0F;
    draw.batches.push_back(batch);

    RenderCommandBuffer commands {};
    commands.marker("before-j3d");
    commands.draw_j3d(draw);
    commands.draw_layout({
        .framebuffer_width = 640U,
        .framebuffer_height = 480U,
        .layout_width = 640.0F,
        .layout_height = 480.0F,
    });

    auto emitted = commands.commands();
    $pc_port_require_eq(emitted.size(), 3ULL);

    const auto &marker = std::get<RenderMarkerCommand>(emitted[0U].payload);
    $pc_port_require(marker.label == "before-j3d");
    const auto &j3d = std::get<RenderDrawJ3dCommand>(emitted[1U].payload);
    $pc_port_require_eq(j3d.view_id, static_cast< std::uint8_t >(3U));
    $pc_port_require_eq(j3d.framebuffer_width, static_cast< std::uint16_t >(640U));
    $pc_port_require_eq(j3d.framebuffer_height, static_cast< std::uint16_t >(480U));
    $pc_port_require_eq(j3d.view_matrix[12U], 15.0F);
    $pc_port_require_eq(j3d.projection_matrix[0U], 2.0F);
    $pc_port_require_eq(j3d.batches.size(), 1ULL);
    $pc_port_require(j3d.batches[0U].blend_mode == smgpc::render::core::RenderBlendMode::Additive);
    $pc_port_require(j3d.batches[0U].cull_mode == smgpc::render::core::RenderCullMode::None);
    $pc_port_require(j3d.batches[0U].depth_mode == smgpc::render::core::RenderDepthMode::Less);
    $pc_port_require(!j3d.batches[0U].write_depth);
    $pc_port_require(j3d.batches[0U].secondary_texture_mode == smgpc::render::core::RenderTriangleTextureCombineMode::J3dTevColorStages);
    $pc_port_require_eq(j3d.batches[0U].tev_stage_count, static_cast< std::uint8_t >(2U));
    $pc_port_require_eq(j3d.batches[0U].tev_stages[1U].color_args.b, static_cast< std::uint8_t >(10U));
    $pc_port_require_eq(j3d.batches[0U].tev_stage_texture_swizzles[1U].channels[0U], static_cast< std::uint8_t >(2U));
    $pc_port_require_eq(j3d.batches[0U].tev_stage_raster_swizzles[1U].channels[0U], static_cast< std::uint8_t >(1U));
    $pc_port_require(j3d.batches[0U].alpha_compare.valid);
    $pc_port_require_eq(j3d.batches[0U].alpha_compare.op, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(j3d.batches[0U].indirect_texture_stage_count, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(j3d.batches[0U].indirect_texture_orders[0U].texture_index, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(j3d.batches[0U].indirect_texture_matrices[0U].scale_exponent, static_cast< std::int8_t >(-2));
    $pc_port_require_eq(j3d.batches[0U].indirect_tev_stages[0U].bias, static_cast< std::uint8_t >(7U));
    $pc_port_require_eq(j3d.batches[0U].secondary_texture.id, 12ULL);
    $pc_port_require_eq(j3d.batches[0U].vertices.size(), 3ULL);
    $pc_port_require_eq(j3d.batches[0U].vertices[2U].z, 8.0F);
    (void)std::get<RenderDrawLayoutCommand>(emitted[2U].payload);
}

$test("Render::RenderCommandBuffer::clear() drops prior commands") {
    RenderCommandBuffer commands {};
    commands.set_viewport(0U, 1U, 2U, 3U, 4U);
    commands.draw_layout({
        .framebuffer_width = 64U,
        .framebuffer_height = 64U,
        .layout_width = 64.0F,
        .layout_height = 64.0F,
    });

    commands.clear();
    $pc_port_require_eq(commands.commands().size(), 0ULL);
}

$test("LayoutRenderPass preserves mixed quad and triangle emission order") {
    smgpc::render::layout::LayoutDrawList draw_list {};
    draw_list.push_quad(smgpc::render::layout::QuadCommand {
        .x0 = 0.0F,
        .y0 = 0.0F,
        .x1 = 16.0F,
        .y1 = 16.0F,
    });
    draw_list.push_triangle_batch(smgpc::render::layout::TriangleBatchCommand {
        .secondary_texture_mode = smgpc::render::layout::TriangleTextureCombineMode::J3dTevColorStages,
        .tev_stage_count = 2U,
        .tev_stages = {
            smgpc::render::layout::TriangleTevStage {
                .color_args = {.a = 15U, .b = 8U, .c = 10U, .d = 14U},
                .color_op = {.op = 0U, .bias = 0U, .scale = 0U, .clamp = 1U, .output_register = 0U},
            },
            smgpc::render::layout::TriangleTevStage {
                .color_args = {.a = 15U, .b = 10U, .c = 8U, .d = 0U},
                .color_op = {.op = 0U, .bias = 0U, .scale = 0U, .clamp = 1U, .output_register = 0U},
            },
        },
        .vertices = {
            smgpc::render::layout::TriangleVertex {.x = 0.0F, .y = 0.0F},
            smgpc::render::layout::TriangleVertex {.x = 8.0F, .y = 0.0F},
            smgpc::render::layout::TriangleVertex {.x = 0.0F, .y = 8.0F},
        },
    });
    draw_list.push_quad(smgpc::render::layout::QuadCommand {
        .x0 = 16.0F,
        .y0 = 16.0F,
        .x1 = 32.0F,
        .y1 = 32.0F,
    });

    RenderCommandBuffer commands {};
    smgpc::render::layout::LayoutRenderPass pass {};
    pass.record(commands, draw_list, 64U, 64U, 64.0F, 64.0F);

    const auto &emitted = commands.commands();
    $pc_port_require(!emitted.empty());
    const auto &draw = std::get<RenderDrawLayoutCommand>(emitted.back().payload);
    $pc_port_require_eq(draw.quads.size(), 2ULL);
    $pc_port_require_eq(draw.triangle_batches.size(), 1ULL);
    $pc_port_require(draw.triangle_batches[0U].secondary_texture_mode == smgpc::render::core::RenderTriangleTextureCombineMode::J3dTevColorStages);
    $pc_port_require_eq(draw.triangle_batches[0U].tev_stage_count, static_cast< std::uint8_t >(2U));
    $pc_port_require_eq(draw.triangle_batches[0U].tev_stages[0U].color_args.a, static_cast< std::uint8_t >(15U));
    $pc_port_require_eq(draw.triangle_batches[0U].tev_stages[0U].color_args.b, static_cast< std::uint8_t >(8U));
    $pc_port_require_eq(draw.triangle_batches[0U].tev_stages[0U].color_args.c, static_cast< std::uint8_t >(10U));
    $pc_port_require_eq(draw.triangle_batches[0U].tev_stages[1U].color_args.b, static_cast< std::uint8_t >(10U));
    $pc_port_require_eq(draw.triangle_batches[0U].tev_stages[1U].color_args.c, static_cast< std::uint8_t >(8U));
    $pc_port_require_eq(draw.triangle_batches[0U].tev_stages[1U].color_op.clamp, static_cast< std::uint8_t >(1U));
    $pc_port_require_eq(draw.draw_order.size(), 3ULL);
    $pc_port_require(draw.draw_order[0U].kind == RenderLayoutDrawItemKind::Quad);
    $pc_port_require_eq(draw.draw_order[0U].index, 0ULL);
    $pc_port_require(draw.draw_order[1U].kind == RenderLayoutDrawItemKind::TriangleBatch);
    $pc_port_require_eq(draw.draw_order[1U].index, 0ULL);
    $pc_port_require(draw.draw_order[2U].kind == RenderLayoutDrawItemKind::Quad);
    $pc_port_require_eq(draw.draw_order[2U].index, 1ULL);
}
