#pragma once

#include "J3dModel.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace smgpc::game {

    struct J3dMatrix3x4 {
        std::array< float, 12U > m{
            1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F,
        };
    };

    struct J3dTextureCoordinate {
        float u = 0.0F;
        float v = 0.0F;
    };

    struct J3dMaterialTexturePass {
        std::uint8_t stage = 0U;
        std::uint8_t tex_coord_slot = 0xffU;
        std::uint8_t tex_map_slot = 0xffU;
        std::uint16_t texture_index = 0xffffU;
        std::optional< J3dTexCoordGenSummary > tex_coord_gen{};
        std::optional< J3dTexMatrixSummary > tex_matrix{};
    };

    struct J3dComposedMaterialTexture {
        DecodedTexture image{};
        bool raster_color_baked = false;
    };

    [[nodiscard]] std::optional< std::uint8_t > j3d_tex_matrix_slot_from_gx_matrix(std::uint8_t matrix);
    [[nodiscard]] std::vector< J3dMaterialTexturePass > j3d_material_texture_passes(const J3dMaterialSummary& material);
    [[nodiscard]] std::optional< J3dMaterialTexturePass > j3d_representative_texture_pass(const J3dMaterialSummary& material);
    [[nodiscard]] std::optional< J3dComposedMaterialTexture > j3d_try_compose_material_texture(const J3dMaterialSummary& material,
                                                                                               const DecodedTexture& texture,
                                                                                               std::array< std::uint8_t, 4U > raster_color,
                                                                                               std::uint8_t texture_map_slot);
    [[nodiscard]] J3dTextureCoordinate j3d_transform_tex_coord(const J3dMeshVertex& source, const J3dTexCoordGenSummary* tex_coord_gen,
                                                               const J3dTexMatrixSummary* tex_matrix, const J3dMatrix3x4* model_matrix = nullptr);

}  // namespace smgpc::game
