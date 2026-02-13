#include "assets/layout/Brlyt.hpp"
#include "game/layout/LayoutArchiveLoader.hpp"
#include "game/layout/LayoutRuntimeActor.hpp"
#include "render/layout/LayoutDrawList.hpp"
#include "tests/TestHarness.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace {

[[nodiscard]] smgpc::assets::layout::PaneDefinition make_picture_root_pane() {
    smgpc::assets::layout::PaneDefinition pane {};
    pane.type = smgpc::assets::layout::PaneType::Picture;
    pane.name = "RootPicture";
    pane.visible = true;
    pane.alpha = 255U;
    pane.scale = {.x = 1.0F, .y = 1.0F};
    pane.size = {.x = 64.0F, .y = 64.0F};
    pane.material_index = 0;
    pane.base_position = 0U;
    pane.tex_coords = {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F};
    pane.vertex_colors = std::array<smgpc::assets::layout::Color, 4> {
        smgpc::assets::layout::Color {.r = 255U, .g = 255U, .b = 255U, .a = 255U},
        smgpc::assets::layout::Color {.r = 255U, .g = 255U, .b = 255U, .a = 255U},
        smgpc::assets::layout::Color {.r = 255U, .g = 255U, .b = 255U, .a = 255U},
        smgpc::assets::layout::Color {.r = 255U, .g = 255U, .b = 255U, .a = 255U},
    };
    return pane;
}

[[nodiscard]] smgpc::assets::layout::tpl::DecodedImage make_solid_texture(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    smgpc::assets::layout::tpl::DecodedImage image {};
    image.width = 2U;
    image.height = 2U;
    image.rgba8.resize(2U * 2U * 4U);
    for (std::size_t pixel = 0; pixel < 4U; ++pixel) {
        const std::size_t base = pixel * 4U;
        image.rgba8[base + 0U] = r;
        image.rgba8[base + 1U] = g;
        image.rgba8[base + 2U] = b;
        image.rgba8[base + 3U] = a;
    }
    return image;
}

[[nodiscard]] smgpc::assets::layout::tpl::DecodedImage make_alpha_mask_texture() {
    smgpc::assets::layout::tpl::DecodedImage image {};
    image.width = 2U;
    image.height = 2U;
    image.rgba8 = {
        255U, 255U, 255U, 0U,
        255U, 255U, 255U, 255U,
        255U, 255U, 255U, 0U,
        255U, 255U, 255U, 255U,
    };
    return image;
}

[[nodiscard]] std::shared_ptr<smgpc::game::layout::LayoutArchiveData> make_single_texture_resource() {
    auto resource = std::make_shared<smgpc::game::layout::LayoutArchiveData>();
    resource->layout.center_origin = false;
    resource->layout.size = {.x = 64.0F, .y = 64.0F};
    resource->layout.root_pane = 0;
    resource->layout.panes.push_back(make_picture_root_pane());
    resource->layout.texture_names.push_back("Color.tpl");

    smgpc::assets::layout::MaterialDefinition material {};
    material.name = "SingleTexture";
    material.texture_index = 0;
    material.texture_indices.push_back(0);
    resource->layout.materials.push_back(std::move(material));

    resource->textures_by_name.emplace("color", make_solid_texture(64U, 128U, 192U, 255U));
    return resource;
}

[[nodiscard]] std::shared_ptr<smgpc::game::layout::LayoutArchiveData> make_two_texture_resource() {
    auto resource = std::make_shared<smgpc::game::layout::LayoutArchiveData>();
    resource->layout.center_origin = false;
    resource->layout.size = {.x = 64.0F, .y = 64.0F};
    resource->layout.root_pane = 0;
    resource->layout.panes.push_back(make_picture_root_pane());
    resource->layout.texture_names.push_back("Mask.tpl");
    resource->layout.texture_names.push_back("Color.tpl");

    smgpc::assets::layout::MaterialDefinition material {};
    material.name = "MaskedTexture";
    material.texture_index = 0;
    material.texture_indices = {0, 1};
    resource->layout.materials.push_back(std::move(material));

    resource->textures_by_name.emplace("mask", make_alpha_mask_texture());
    resource->textures_by_name.emplace("color", make_solid_texture(32U, 96U, 160U, 255U));
    return resource;
}

}  // namespace

$test("LayoutRuntimeActor single-texture material keeps color texture") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_single_texture_resource());
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto &quad = draw_list.quads().front();
    $pc_port_require(not quad.use_mask_texture);
    $pc_port_require(quad.texture.id != 0U);
    $pc_port_require(quad.texture.width > 0U);
    $pc_port_require(quad.texture.height > 0U);
}

$test("LayoutRuntimeActor two-texture material can emit mask rendering state") {
    smgpc::game::layout::LayoutRuntimeActor actor(make_two_texture_resource());
    actor.appear();

    smgpc::render::layout::LayoutDrawList draw_list {};
    actor.appendDrawCommands(&draw_list);

    $pc_port_require(not draw_list.quads().empty());
    const auto &quad = draw_list.quads().front();
    $pc_port_require(quad.texture.id != 0U);
    $pc_port_require(quad.texture.width > 0U);
    $pc_port_require(quad.texture.height > 0U);
    $pc_port_require(quad.use_mask_texture);
    $pc_port_require(quad.mask_texture.id != 0U);
    $pc_port_require(quad.mask_texture.width > 0U);
    $pc_port_require(quad.mask_texture.height > 0U);
}
