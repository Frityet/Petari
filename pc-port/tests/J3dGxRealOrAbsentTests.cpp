#include "resource/RarcArchive.hpp"
#include "render/GXState.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/J3dMatrix.hpp"
#include "render/J3dModel.hpp"
#include "render/J3dModelRenderer.hpp"
#include "render/core/RenderTypes.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_logic_error(const std::function<void()> &operation, std::string_view message) {
        auto rejected = false;
        try {
            operation();
        } catch (const std::logic_error &) {
            rejected = true;
        }
        require(rejected, message);
    }

    [[nodiscard]] bool near(float left, float right) {
        return std::abs(left - right) < 0.0001F;
    }

    [[nodiscard]] std::optional<std::filesystem::path> find_object_archive(std::string_view archive_name) {
        for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
            const std::filesystem::path candidates[]{
                root.parent_path() / "orig/RMGK02/files/ObjectData" / archive_name,
                root.parent_path() / "orig/RMGK01/files/ObjectData" / archive_name,
                root / "orig/RMGK02/files/ObjectData" / archive_name,
                root / "orig/RMGK01/files/ObjectData" / archive_name,
                root / "container/orig/RMGK02/files/ObjectData" / archive_name,
                root / "container/orig/RMGK01/files/ObjectData" / archive_name,
                root / "pc-port/container/orig/RMGK02/files/ObjectData" / archive_name,
                root / "pc-port/container/orig/RMGK01/files/ObjectData" / archive_name,
            };
            for (const auto &candidate : candidates) {
                auto error = std::error_code{};
                if (std::filesystem::is_regular_file(candidate, error)) {
                    return candidate;
                }
            }
            if (root == root.root_path()) {
                break;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] smgpc::render::J3dModelGeometry load_model(const std::filesystem::path &archive_path,
                                                              std::string_view model_name) {
        const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);
        const auto *entry = archive.find_by_basename(model_name);
        require(entry != nullptr, "retail archive is missing its expected J3D model");
        return smgpc::render::extract_j3d_model_geometry(archive.file_data(*entry));
    }

    void test_exact_texgen_inputs() {
        auto vertex = smgpc::render::J3dMeshVertex{
            .u = 0.75F,
            .v = 0.875F,
        };
        vertex.tex_coords[0U] = {0.125F, 0.25F};
        vertex.tex_coord_count = 1U;

        const auto tex0 = smgpc::render::J3dTexCoordGenSummary{
            .type = 1U,
            .source = 4U,
            .matrix = 60U,
        };
        const auto projected = smgpc::render::j3d_project_tex_coord(vertex, &tex0, nullptr, nullptr);
        require(near(projected.u, 0.125F) && near(projected.v, 0.25F),
                "GX_TG_TEX0 must read the vertex TEX0 attribute, not the legacy u/v aliases");

        auto tex1 = tex0;
        tex1.source = 5U;
        require_logic_error([&] { (void)smgpc::render::j3d_project_tex_coord(vertex, &tex1, nullptr, nullptr); },
                            "a missing TEX1 vertex attribute must not alias TEX0");

        auto color0 = tex0;
        color0.type = 10U;
        color0.source = 19U;
        require_logic_error([&] { (void)smgpc::render::j3d_project_tex_coord(vertex, &color0, nullptr, nullptr); },
                            "GX_TG_COLOR0 must not be replaced by source UVs");
        require_logic_error([&] { (void)smgpc::render::j3d_project_tex_coord(vertex, nullptr, nullptr, nullptr); },
                            "an absent texcoord generator must not manufacture a TEX0 source");
    }

    void test_gx_initial_selector_defaults() {
        const auto texture = smgpc::render::core::GxTextureStage2D{};
        require(static_cast<std::uint32_t>(texture.texgen_source) == 0xffU,
                "an unconfigured texture stage must not guess a TEX0 texgen source");

        const auto stage = smgpc::render::core::GxTevStage2D{};
        require(stage.k_color_sel == static_cast<std::uint8_t>(GX_TEV_KCSEL_1_4) &&
                    stage.k_alpha_sel == static_cast<std::uint8_t>(GX_TEV_KASEL_1),
                "host TEV stage defaults must match GXInit instead of carrying private 0xff sentinels");
        require(stage.texture_coord_stage == 0xffU && stage.texture_map_stage == 0xffU,
                "an unbound host TEV stage must have an explicit null texture order");

        const auto textured = smgpc::render::gx_brlyt_default_texture_color_stage(3U);
        require(textured.texture_coord_stage == 3U && textured.texture_map_stage == 3U,
                "a synthesized NW4R texture stage must state both real GX selectors explicitly");
    }

    void test_hardware_unbound_texture_input() {
        auto material = smgpc::render::J3dMaterialSummary{};
        material.tev_stage_count = 1U;
        material.tev_stages.push_back(smgpc::render::J3dTevStageSummary{
            .color_in = {15U, 15U, 15U, 8U},
            .color_clamp = 1U,
            .alpha_in = {7U, 7U, 7U, 4U},
            .alpha_clamp = 1U,
        });
        material.tev_orders.push_back(smgpc::render::J3dTevOrderSummary{
            .stage = 0U,
            .tex_coord = 0xffU,
            .tex_map = 0xffU,
            .color_channel = 0xffU,
        });

        const auto no_texgens = smgpc::render::j3d_try_compose_material_constant(material, {255U, 255U, 255U, 255U});
        require(no_texgens.has_value() && no_texgens->image.rgba == std::vector<std::uint8_t>({0U, 0U, 0U, 0U}),
                "GX's unbound texture input must be black when no texgens are enabled");

        material.gx_state.texgen_count = 1U;
        const auto disabled_order = smgpc::render::j3d_try_compose_material_constant(material, {255U, 255U, 255U, 255U});
        require(disabled_order.has_value() && disabled_order->image.rgba == std::vector<std::uint8_t>({255U, 255U, 255U, 255U}),
                "GX's disabled texture order must be white when texgens are enabled");

        material.tev_orders.front().tex_coord = 0U;
        material.tev_orders.front().tex_map = 0U;
        require(!smgpc::render::j3d_evaluate_material_color(material, {}, {}, {}, {255U, 255U, 255U, 255U}).has_value(),
                "a mapped TEV order with no texture pass must remain absent");
    }

    void test_inverse_transpose_normal_basis() {
        const auto mirrored_scale = smgpc::render::J3dMatrix3x4{{
            -2.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 3.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 4.0F, 0.0F,
        }};
        const auto normal = smgpc::render::j3d_transform_normal(mirrored_scale, {1.0F, 0.0F, 0.0F});
        require(near(normal[0U], -1.0F) && near(normal[1U], 0.0F) && near(normal[2U], 0.0F),
                "inverse-transpose normals must retain a mirrored transform's determinant sign");

        const auto singular = smgpc::render::J3dMatrix3x4{{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 0.0F, 0.0F,
        }};
        require_logic_error([&] { (void)smgpc::render::j3d_transform_normal(singular, {0.0F, 0.0F, 1.0F}); },
                            "a singular normal basis must not fall back to the model matrix");
    }

    void test_scene_light_space_and_actor_ambient() {
        auto material = smgpc::render::GXMaterialState{};
        material.color_channels[0U].ambient_color = {1U, 2U, 3U, 4U};
        material.color_channels[1U].ambient_color = {5U, 6U, 7U, 8U};
        material.lights[0U].loaded = true;
        material.lights[0U].position = {90.0F, 91.0F, 92.0F};

        auto scene_lights = std::array<smgpc::render::GXLightState, 8U>{};
        scene_lights[0U].loaded = true;
        scene_lights[0U].coordinate_space = smgpc::render::GXLightCoordinateSpace::World;
        scene_lights[0U].position = {100.0F, 100.0F, 100.0F};
        scene_lights[1U].loaded = true;
        scene_lights[1U].coordinate_space = smgpc::render::GXLightCoordinateSpace::World;
        scene_lights[1U].position = {7.0F, 25.0F, 42.0F};
        scene_lights[1U].direction = {1.0F, 0.0F, 0.0F};
        scene_lights[2U].loaded = true;
        scene_lights[2U].coordinate_space = smgpc::render::GXLightCoordinateSpace::View;
        scene_lights[2U].position = {2.0F, 3.0F, 4.0F};
        scene_lights[2U].direction = {0.0F, 0.0F, -1.0F};

        const auto camera = smgpc::camera::CameraPose{
            .eye = {10.0F, 20.0F, 30.0F},
            .watch = {10.0F, 20.0F, 31.0F},
            .up = {0.0F, 1.0F, 0.0F},
        };
        const auto ambient = smgpc::render::GXColorValue{90U, 91U, 92U, 60U};
        const auto effective = smgpc::render::j3d_material_with_scene_lights(
            material, scene_lights, ambient, &camera);

        require(effective.lights[0U].position != material.lights[0U].position &&
                    near(effective.lights[0U].position[0U], -90.0F) &&
                    near(effective.lights[0U].position[1U], 80.0F) &&
                    near(effective.lights[0U].position[2U], -70.0F),
                "ActorLightCtrl loads must replace the earlier material display-list light register");
        require(effective.lights[1U].coordinate_space ==
                        smgpc::render::GXLightCoordinateSpace::View &&
                    near(effective.lights[1U].position[0U], 3.0F) &&
                    near(effective.lights[1U].position[1U], 5.0F) &&
                    near(effective.lights[1U].position[2U], -12.0F) &&
                    near(effective.lights[1U].direction[0U], -1.0F),
                "world LightData positions and directions must enter GX's -Z-front active camera basis");
        require(effective.lights[2U].position == scene_lights[2U].position &&
                    effective.lights[2U].direction == scene_lights[2U].direction,
                "authored follow-camera lights already use GX view space and must not be transformed a second time");
        require(effective.color_channels[0U].ambient_color == ambient &&
                    effective.color_channels[1U].ambient_color ==
                        material.color_channels[1U].ambient_color,
                "ActorLightInfo ambient must replace only the GX_COLOR0A0 ambient register");

        const auto unresolved = smgpc::render::j3d_material_with_scene_lights(
            smgpc::render::GXMaterialState{}, scene_lights, {}, nullptr);
        require(unresolved.lights[1U].coordinate_space ==
                    smgpc::render::GXLightCoordinateSpace::World,
                "packet inspection without a camera must retain an explicit world-space marker");

        auto service = smgpc::runtime::SceneLightService{};
        service.set_actor_ambient(ambient);
        service.set_light(1U, scene_lights[1U]);
        require(service.actor_ambient().has_value() && *service.actor_ambient() == ambient,
                "the scene light service must carry the actor ambient alongside its light registers");
        const auto draw_options =
            smgpc::render::j3d_scene_light_draw_options(service);
        require(draw_options.scene_lights.size() == 8U &&
                    draw_options.scene_lights[1U].loaded &&
                    draw_options.scene_ambient_color == ambient,
                "renderer-only exact models must be able to consume the same generalized resolved scene-light state");
        service.clear();
        require(!service.actor_ambient().has_value(),
                "scene reset must not leak one actor's ambient register into a later scene");

        auto material_light = smgpc::render::GXMaterialState{};
        material_light.color_channel_count = 1U;
        material_light.color_channels[0U].material_color = {255U, 255U, 255U, 255U};
        material_light.color_channels[0U].ambient_color = {0U, 0U, 0U, 0U};
        material_light.color_channels[0U].color_control.lighting_enabled = true;
        material_light.color_channels[0U].color_control.light_mask = 1U;
        material_light.color_channels[0U].color_control.diffuse_function = 2U;
        material_light.lights[0U].loaded = true;
        material_light.lights[0U].coordinate_space =
            smgpc::render::GXLightCoordinateSpace::View;
        material_light.lights[0U].color = {255U, 255U, 255U, 255U};
        material_light.lights[0U].position = {0.0F, 0.0F, -20.0F};
        const auto gx_vertex = smgpc::render::j3d_host_view_vector_to_gx_view(
            {0.0F, 0.0F, 10.0F});
        const auto gx_normal = smgpc::render::j3d_host_view_vector_to_gx_view(
            {0.0F, 0.0F, 1.0F});
        const auto lit = smgpc::render::gx_evaluate_lit_raster_color(
            material_light, 0U, {255U, 255U, 255U, 255U}, gx_vertex, gx_normal);
        const auto mirrored = smgpc::render::gx_evaluate_lit_raster_color(
            material_light, 0U, {255U, 255U, 255U, 255U},
            {0.0F, 0.0F, 10.0F}, {0.0F, 0.0F, 1.0F});
        require(lit[0U] > 250U && mirrored[0U] == 0U,
                "decoded material-DL GX-view lights must evaluate against -Z-front vertex and normal coordinates");

        const auto gentle = smgpc::render::gx_light_distance_attenuation(100.0F, 0.5F, 1U);
        const auto medium = smgpc::render::gx_light_distance_attenuation(100.0F, 0.5F, 2U);
        const auto steep = smgpc::render::gx_light_distance_attenuation(100.0F, 0.5F, 3U);
        const auto disabled = smgpc::render::gx_light_distance_attenuation(100.0F, 1.0F, 2U);
        const auto zero_brightness =
            smgpc::render::gx_light_distance_attenuation(100.0F, 0.0F, 2U);
        require(near(gentle[0U], 1.0F) && near(gentle[1U], 0.01F) &&
                    near(gentle[2U], 0.0F) && near(medium[1U], 0.005F) &&
                    near(medium[2U], 0.00005F) && near(steep[1U], 0.0F) &&
                    near(steep[2U], 0.0001F) && disabled == std::array{1.0F, 0.0F, 0.0F} &&
                    zero_brightness == std::array{1.0F, 0.0F, 0.0F},
                "point lights must use GXInitLightDistAttn coefficients for every authored attenuation mode");
    }

    void test_retail_j3d_selector_defaults(const std::filesystem::path &archive_path) {
        const auto geometry = load_model(archive_path, "begomanroomplanet.bmd");
        require(geometry.materials.has_value(), "retail BegomanRoomPlanet must contain MAT3");
        const auto material = std::ranges::find_if(geometry.materials->materials, [](const auto &candidate) {
            return candidate.name == "FrontColor";
        });
        require(material != geometry.materials->materials.end() && !material->tev_stages.empty(),
                "retail BegomanRoomPlanet must contain the FrontColor TEV stage");
        require(material->tev_stages.front().k_color_sel == 0x0cU && material->tev_stages.front().k_alpha_sel == 0x1cU,
                "MAT3 0xff selector sentinels must resolve exactly as J3DMaterialFactory does");
    }

    void test_retail_color_texgen_stays_absent(const std::filesystem::path &archive_path) {
        const auto geometry = load_model(archive_path, "fruitsboatb.bmd");
        require(geometry.materials.has_value(), "retail FruitsBoatB must contain MAT3");
        const auto material = std::ranges::find_if(geometry.materials->materials, [](const auto &candidate) {
            return candidate.name == "_guri";
        });
        require(material != geometry.materials->materials.end(), "retail FruitsBoatB must contain its _guri material");
        const auto color_texgen = std::ranges::find_if(material->tex_coord_gens, [](const auto &gen) {
            return gen.type == 10U && gen.source == 19U;
        });
        require(color_texgen != material->tex_coord_gens.end(),
                "retail FruitsBoatB must retain its GX_TG_COLOR0/GX_TG_SRTG declaration");

        const auto passes = smgpc::render::j3d_material_texture_passes(*material);
        require(std::ranges::any_of(passes, [](const auto &pass) {
                    return pass.tex_coord_gen.has_value() && pass.tex_coord_gen->source == 19U;
                }),
                "the retail COLOR0 texture pass must remain visible to capability selection");
        require(!smgpc::render::j3d_try_compose_material_texture(*material, geometry.textures, passes,
                                                                  {255U, 255U, 255U, 255U})
                     .has_value(),
                "CPU texture composition must remain absent when post-lighting COLOR0 is unavailable");
    }
}  // namespace

int main() {
    auto passed = 0;
    test_exact_texgen_inputs();
    ++passed;
    test_gx_initial_selector_defaults();
    ++passed;
    test_hardware_unbound_texture_input();
    ++passed;
    test_inverse_transpose_normal_basis();
    ++passed;
    test_scene_light_space_and_actor_ambient();
    ++passed;

    if (const auto archive = find_object_archive("BegomanRoomPlanet.arc")) {
        test_retail_j3d_selector_defaults(*archive);
        ++passed;
    } else {
        std::cout << "[skip] retail BegomanRoomPlanet selector check\n";
    }

    if (const auto archive = find_object_archive("FruitsBoatB.arc")) {
        test_retail_color_texgen_stays_absent(*archive);
        ++passed;
    } else {
        std::cout << "[skip] retail FruitsBoatB COLOR0 texgen check\n";
    }

    std::cout << "J3D/GX real-or-absent tests passed: " << passed << "/6\n";
    return 0;
}
