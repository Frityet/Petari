#pragma once

#include "GXState.hpp"
#include "J3dTexture.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace smgpc::game {

    struct J3dSectionInfo {
        std::string tag;
        std::uint32_t offset = 0U;
        std::uint32_t size = 0U;
    };

    struct J3dHierarchyEntry {
        std::uint16_t type = 0U;
        std::uint16_t value = 0U;
    };

    struct J3dInfoSummary {
        std::uint16_t flags = 0U;
        std::uint32_t packet_count = 0U;
        std::uint32_t vertex_count = 0U;
        std::vector< J3dHierarchyEntry > hierarchy;
    };

    struct J3dVertexAttributeFormat {
        std::uint32_t attr = 0U;
        std::uint32_t component_count = 0U;
        std::uint32_t component_type = 0U;
        std::uint8_t fraction = 0U;
    };

    struct J3dVertexArraySummary {
        std::uint32_t attr = 0U;
        std::uint32_t offset = 0U;
        std::uint32_t stride = 0U;
        std::uint32_t inferred_count = 0U;
    };

    struct J3dVertexSummary {
        std::vector< J3dVertexAttributeFormat > formats;
        std::vector< J3dVertexArraySummary > arrays;
    };

    struct J3dVertexDesc {
        std::uint32_t attr = 0U;
        std::uint32_t type = 0U;
    };

    struct J3dPrimitiveSummary {
        std::uint8_t command = 0U;
        std::uint8_t primitive = 0U;
        std::uint8_t vertex_format = 0U;
        std::uint16_t vertex_count = 0U;
        std::uint32_t triangle_count = 0U;
    };

    struct J3dShapeMatrixGroupSummary {
        std::uint16_t group_index = 0U;
        std::uint16_t use_matrix_index = 0xffffU;
        std::uint16_t use_matrix_count = 0U;
        std::uint32_t first_matrix_table_index = 0U;
        std::uint32_t display_list_offset = 0U;
        std::uint32_t display_list_size = 0U;
        std::vector< std::uint16_t > matrix_table;
        std::vector< J3dPrimitiveSummary > primitives;
        std::uint32_t parsed_display_list_bytes = 0U;
        std::uint32_t triangle_count = 0U;
    };

    struct J3dShapeSummary {
        std::string name;
        std::uint16_t index = 0U;
        std::uint16_t draw_order = 0xffffU;
        std::uint16_t material_index = 0xffffU;
        std::uint16_t joint_index = 0xffffU;
        std::uint8_t matrix_type = 0U;
        std::uint16_t matrix_group_count = 0U;
        std::uint16_t vertex_desc_list_index = 0U;
        std::uint16_t matrix_init_data_index = 0U;
        std::uint16_t draw_init_data_index = 0U;
        float radius = 0.0F;
        std::array< float, 3U > min{};
        std::array< float, 3U > max{};
        std::vector< J3dVertexDesc > vertex_desc;
        std::vector< J3dPrimitiveSummary > primitives;
        std::uint32_t display_list_bytes = 0U;
        std::uint32_t parsed_display_list_bytes = 0U;
        std::uint32_t triangle_count = 0U;
        std::vector< J3dShapeMatrixGroupSummary > matrix_groups;
    };

    struct J3dShapeBlockSummary {
        std::uint16_t shape_count = 0U;
        std::vector< J3dShapeSummary > shapes;
    };

    struct J3dJointSummary {
        std::string name;
        std::uint16_t index = 0U;
        std::uint16_t kind = 0U;
        std::uint8_t scale_compensate = 0U;
        std::array< float, 3U > scale{1.0F, 1.0F, 1.0F};
        std::array< std::int16_t, 3U > rotation{};
        std::array< float, 3U > translation{};
        float radius = 0.0F;
        std::array< float, 3U > min{};
        std::array< float, 3U > max{};
    };

    struct J3dJointBlockSummary {
        std::uint16_t joint_count = 0U;
        std::vector< std::uint16_t > remap_table;
        std::vector< std::uint16_t > parent_indices;
        std::vector< J3dJointSummary > joints;
    };

    struct J3dEnvelopeMatrixSummary {
        std::vector< std::uint16_t > joint_indices;
        std::vector< float > weights;
    };

    struct J3dEnvelopeBlockSummary {
        std::uint16_t matrix_count = 0U;
        std::vector< J3dEnvelopeMatrixSummary > matrices;
        std::vector< std::array< float, 12U > > inverse_bind_matrices;
    };

    struct J3dDrawMatrixSummary {
        bool weighted = false;
        std::uint16_t index = 0xffffU;
    };

    struct J3dDrawBlockSummary {
        std::uint16_t matrix_count = 0U;
        std::vector< J3dDrawMatrixSummary > matrices;
    };

    struct J3dMaterialTextureBinding {
        std::uint8_t slot = 0U;
        std::uint16_t texture_index = 0xffffU;
    };

    struct J3dTexCoordGenSummary {
        std::uint8_t slot = 0U;
        std::uint8_t type = 0U;
        std::uint8_t source = 0U;
        std::uint8_t matrix = 0U;
    };

    struct J3dTexMatrixSummary {
        std::uint8_t slot = 0U;
        std::uint8_t projection = 0U;
        std::uint8_t info = 0U;
        std::array< float, 3U > center{};
        float scale_s = 1.0F;
        float scale_t = 1.0F;
        std::int16_t rotation = 0;
        float translate_s = 0.0F;
        float translate_t = 0.0F;
        std::array< float, 16U > effect_matrix{};
    };

    struct J3dTevOrderSummary {
        std::uint8_t stage = 0U;
        std::uint8_t tex_coord = 0xffU;
        std::uint8_t tex_map = 0xffU;
        std::uint8_t color_channel = 0xffU;
    };

    struct J3dTevStageSummary {
        std::uint8_t stage = 0U;
        std::array< std::uint8_t, 20U > raw{};
        std::array< std::uint8_t, 4U > color_in{};
        std::uint8_t color_op = 0U;
        std::uint8_t color_bias = 0U;
        std::uint8_t color_scale = 0U;
        std::uint8_t color_clamp = 0U;
        std::uint8_t color_out = 0U;
        std::uint8_t k_color_sel = 0xffU;
        std::array< std::uint8_t, 4U > alpha_in{};
        std::uint8_t alpha_op = 0U;
        std::uint8_t alpha_bias = 0U;
        std::uint8_t alpha_scale = 0U;
        std::uint8_t alpha_clamp = 0U;
        std::uint8_t alpha_out = 0U;
        std::uint8_t k_alpha_sel = 0xffU;
    };

    struct J3dAlphaCompareSummary {
        std::uint8_t comp0 = 7U;
        std::uint8_t ref0 = 0U;
        std::uint8_t op = 0U;
        std::uint8_t comp1 = 7U;
        std::uint8_t ref1 = 0U;
        bool enabled = false;
    };

    struct J3dBlendSummary {
        std::uint8_t type = 0U;
        std::uint8_t src_factor = 1U;
        std::uint8_t dst_factor = 0U;
        std::uint8_t op = 3U;
        bool enabled = false;
    };

    struct J3dZModeSummary {
        std::uint8_t compare_enable = 1U;
        std::uint8_t function = 3U;
        std::uint8_t update_enable = 1U;
        bool enabled = false;
    };

    struct J3dMaterialSummary {
        std::string name;
        std::uint16_t index = 0U;
        std::uint16_t material_id = 0U;
        std::uint8_t material_mode = 0U;
        std::array< std::array< std::uint8_t, 4U >, 2U > material_colors{
            std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
            std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
        };
        std::array< std::array< std::uint8_t, 4U >, 4U > tev_k_colors{
            std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
            std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
            std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
            std::array< std::uint8_t, 4U >{255U, 255U, 255U, 255U},
        };
        std::array< GXTevRegisterColor, 4U > tev_colors{};
        std::uint8_t texgen_count = 0U;
        std::uint8_t tev_stage_count = 0U;
        std::uint16_t alpha_comp_index = 0xffffU;
        std::uint16_t blend_index = 0xffffU;
        std::uint8_t cull_mode_index = 0xffU;
        std::uint8_t cull_mode = 0xffU;
        std::uint8_t z_comp_loc_index = 0xffU;
        std::uint8_t z_comp_loc = 0xffU;
        std::uint8_t z_mode_index = 0xffU;
        std::vector< J3dMaterialTextureBinding > textures;
        std::vector< J3dTexCoordGenSummary > tex_coord_gens;
        std::vector< J3dTexMatrixSummary > tex_matrices;
        std::vector< J3dTevOrderSummary > tev_orders;
        std::vector< J3dTevStageSummary > tev_stages;
        J3dAlphaCompareSummary alpha_compare{};
        J3dBlendSummary blend{};
        J3dZModeSummary z_mode{};
        GXMaterialState gx_state{};
    };

    struct J3dMdl3PacketSummary {
        std::uint32_t offset = 0U;
        std::uint32_t size = 0U;
        std::vector< std::uint8_t > bytes;
    };

    struct J3dMdl3BlockSummary {
        std::uint16_t material_count = 0U;
        std::vector< J3dMdl3PacketSummary > packets;
    };

    struct J3dMaterialBlockSummary {
        std::uint16_t material_count = 0U;
        std::vector< J3dMaterialSummary > materials;
    };

    struct J3dModelSummary {
        std::uint32_t magic = 0U;
        std::uint32_t model_type = 0U;
        std::uint32_t section_count = 0U;
        std::vector< J3dSectionInfo > sections;
        std::optional< J3dInfoSummary > info;
        std::optional< J3dVertexSummary > vertices;
        std::optional< J3dEnvelopeBlockSummary > envelopes;
        std::optional< J3dDrawBlockSummary > draw_matrices;
        std::optional< J3dJointBlockSummary > joints;
        std::optional< J3dShapeBlockSummary > shapes;
        std::optional< J3dMaterialBlockSummary > materials;
        std::optional< J3dMdl3BlockSummary > mdl3;
        std::vector< J3dTexture > textures;
    };

    struct J3dMeshVertex {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float u = 0.0F;
        float v = 0.0F;
        std::array< std::uint8_t, 4U > color{255U, 255U, 255U, 255U};
        std::uint8_t position_matrix_slot = 0xffU;
        std::uint16_t draw_matrix_index = 0xffffU;
    };

    struct J3dShapeMesh {
        std::uint16_t shape_index = 0U;
        std::uint16_t draw_order = 0xffffU;
        std::uint16_t material_index = 0xffffU;
        std::uint16_t joint_index = 0xffffU;
        std::uint8_t matrix_type = 0U;
        std::vector< J3dShapeMatrixGroupSummary > matrix_groups;
        std::vector< J3dMeshVertex > vertices;
        std::vector< std::uint16_t > indices;
    };

    struct J3dModelGeometry {
        std::optional< J3dMaterialBlockSummary > materials;
        std::optional< J3dEnvelopeBlockSummary > envelopes;
        std::optional< J3dDrawBlockSummary > draw_matrices;
        std::optional< J3dJointBlockSummary > joints;
        std::vector< J3dTexture > textures;
        std::vector< J3dShapeMesh > shapes;
    };

    [[nodiscard]] J3dModelSummary inspect_j3d_model(std::span< const std::uint8_t > model_data);
    [[nodiscard]] J3dModelGeometry extract_j3d_model_geometry(std::span< const std::uint8_t > model_data);
    [[nodiscard]] std::string j3d_hierarchy_type_name(std::uint16_t type);
    [[nodiscard]] std::string j3d_vertex_attr_name(std::uint32_t attr);
    [[nodiscard]] std::string j3d_vertex_attr_type_name(std::uint32_t type);
    [[nodiscard]] std::string j3d_primitive_name(std::uint8_t primitive);

}  // namespace smgpc::game
