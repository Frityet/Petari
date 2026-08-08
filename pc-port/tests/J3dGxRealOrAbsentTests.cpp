#include "resource/RarcArchive.hpp"
#include "render/GXState.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/J3dMatrix.hpp"
#include "render/J3dModel.hpp"
#include "render/core/RenderTypes.hpp"

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
