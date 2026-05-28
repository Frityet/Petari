#pragma once

#include "J3dModel.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace smgpc::compat {

    struct J3dMatrix3x4 {
        std::array<float, 12U> m{
            1.0F,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            1.0F,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            1.0F,
            0.0F,
        };
    };

    struct J3dTextureCoordinate {
        float u = 0.0F;
        float v = 0.0F;
    };

    struct J3dTextureProjectionCoordinate {
        float u = 0.0F;
        float v = 0.0F;
        float q = 1.0F;
    };

    struct J3dMaterialTexturePass {
        std::uint8_t stage = 0U;
        std::uint8_t tex_coord_slot = 0xffU;
        std::uint8_t tex_map_slot = 0xffU;
        std::uint16_t texture_index = 0xffffU;
        std::optional<J3dTexCoordGenSummary> tex_coord_gen{};
        std::optional<J3dTexMatrixSummary> tex_matrix{};
        std::optional<GXTexCoordScaleState> tex_coord_scale{};
    };

    struct J3dComposedMaterialTexture {
        DecodedTexture image{};
        bool raster_color_baked = false;
    };

    struct J3dIndirectTextureTrace {
        std::uint8_t tev_stage = 0U;
        std::uint8_t indirect_stage = 0U;
        std::uint8_t indirect_tex_map = 0xffU;
        std::uint8_t indirect_tex_coord = 0xffU;
        std::uint8_t format = 0U;
        std::uint8_t bias = 0U;
        std::uint8_t matrix_index = 0U;
        std::uint8_t matrix_id = 0U;
        std::uint8_t wrap_s = 0U;
        std::uint8_t wrap_t = 0U;
        bool add_previous = false;
        J3dTextureCoordinate base_coord{};
        J3dTextureCoordinate indirect_coord{};
        std::int64_t base_indirect_s = 0;
        std::int64_t base_indirect_t = 0;
        std::int64_t scaled_indirect_s = 0;
        std::int64_t scaled_indirect_t = 0;
        std::array<int, 4U> sampled_indirect_color{};
        std::array<std::int32_t, 3U> biased_indirect_coord{};
        std::array<std::int64_t, 2U> translation{};
        std::int64_t base_s = 0;
        std::int64_t base_t = 0;
        std::int64_t transformed_s = 0;
        std::int64_t transformed_t = 0;
        J3dTextureCoordinate transformed_coord{};
    };

    [[nodiscard]] std::optional<std::uint8_t> j3d_tex_matrix_slot_from_gx_matrix(std::uint8_t matrix);
    [[nodiscard]] J3dTexMatrixSummary j3d_apply_projmap_effect_matrix(J3dTexMatrixSummary tex_matrix,
                                                                      const J3dMatrix3x4 &projmap_effect_matrix);
    [[nodiscard]] std::vector<J3dMaterialTexturePass> j3d_material_texture_passes(const J3dMaterialSummary &material);
    [[nodiscard]] std::optional<J3dMaterialTexturePass> j3d_representative_texture_pass(const J3dMaterialSummary &material);
    [[nodiscard]] std::optional<J3dComposedMaterialTexture> j3d_try_compose_material_texture(const J3dMaterialSummary &material,
                                                                                             const DecodedTexture &texture,
                                                                                             std::array<std::uint8_t, 4U> raster_color,
                                                                                             std::uint8_t texture_map_slot);
    [[nodiscard]] std::optional<J3dComposedMaterialTexture> j3d_try_compose_material_constant(const J3dMaterialSummary &material,
                                                                                              std::array<std::uint8_t, 4U> raster_color);
    [[nodiscard]] std::optional<J3dComposedMaterialTexture> j3d_try_compose_material_texture(const J3dMaterialSummary &material,
                                                                                             std::span<const J3dTexture> textures,
                                                                                             std::span<const J3dMaterialTexturePass> passes,
                                                                                             std::array<std::uint8_t, 4U> raster_color);
    [[nodiscard]] std::optional<std::array<std::uint8_t, 4U>>
    j3d_evaluate_material_color(const J3dMaterialSummary &material, std::span<const J3dTexture> textures,
                                std::span<const J3dMaterialTexturePass> passes, const J3dMeshVertex &source,
                                std::array<std::uint8_t, 4U> raster_color, const J3dMatrix3x4 *model_matrix = nullptr);
    [[nodiscard]] std::optional<J3dIndirectTextureTrace>
    j3d_trace_indirect_texture_transform(const J3dMaterialSummary &material, std::span<const J3dTexture> textures,
                                         const J3dMeshVertex &source, const J3dMaterialTexturePass &pass,
                                         const J3dMatrix3x4 *model_matrix = nullptr);
    [[nodiscard]] J3dTextureCoordinate j3d_transform_tex_coord(const J3dMeshVertex &source, const J3dTexCoordGenSummary *tex_coord_gen,
                                                               const J3dTexMatrixSummary *tex_matrix, const J3dMatrix3x4 *model_matrix = nullptr);
    [[nodiscard]] J3dTextureProjectionCoordinate j3d_project_tex_coord(const J3dMeshVertex &source, const J3dTexCoordGenSummary *tex_coord_gen,
                                                                       const J3dTexMatrixSummary *tex_matrix, const J3dMatrix3x4 *model_matrix = nullptr);
    [[nodiscard]] J3dTextureCoordinate j3d_apply_tex_coord_scale(const J3dTextureCoordinate &coord,
                                                                 const J3dMaterialTexturePass &pass,
                                                                 const J3dTexture &texture);
    [[nodiscard]] J3dTextureProjectionCoordinate j3d_apply_tex_coord_scale(const J3dTextureProjectionCoordinate &coord,
                                                                           const J3dMaterialTexturePass &pass,
                                                                           const J3dTexture &texture);

}  // namespace smgpc::compat
