#include "render/core/RenderCommandBuffer.hpp"

#include "tests/TestHarness.hpp"

namespace {

using smgpc::render::core::RenderCommandBuffer;
using smgpc::render::core::RenderClearCommand;
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
