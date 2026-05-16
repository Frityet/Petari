#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "AssetServices.hpp"
#include "J3dTexture.hpp"

namespace smgpc::assets::layout {

    inline constexpr std::uint16_t J3D_NO_TEXTURE_INDEX = 0xFFFFU;
    inline constexpr std::uint16_t J3D_NO_VERTEX_INDEX = 0xFFFFU;
    inline constexpr std::uint16_t J3D_NO_JOINT_INDEX = 0xFFFFU;

    struct J3dVec2 {
        float x{};
        float y{};
    };

    struct J3dVec3 {
        float x{};
        float y{};
        float z{};
    };

    struct J3dColor {
        std::uint8_t r{255U};
        std::uint8_t g{255U};
        std::uint8_t b{255U};
        std::uint8_t a{255U};
    };

    struct J3dTextureSrt {
        float scale_x{1.0F};
        float scale_y{1.0F};
        std::int16_t rotation{};
        float translation_x{};
        float translation_y{};
    };

    struct J3dTextureMatrix {
        bool valid{};
        std::uint8_t projection{};
        std::uint8_t info{};
        J3dVec3 center{0.5F, 0.5F, 0.5F};
        J3dTextureSrt srt{};
        std::array< float, 16U > effect_matrix{
            1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
        };
    };

    struct J3dTextureCoordGen {
        bool valid{};
        std::uint8_t type{};
        std::uint8_t source{};
        std::uint8_t matrix{};
    };

    struct J3dBlendMode {
        bool valid{};
        std::uint8_t type{};
        std::uint8_t source_factor{};
        std::uint8_t destination_factor{};
        std::uint8_t operation{};
    };

    struct J3dTevOrder {
        bool valid{};
        std::uint8_t texture_coordinate{};
        std::uint8_t texture_map{};
        std::uint8_t color_channel{};
    };

    struct J3dTevColorS10 {
        bool valid{};
        std::int16_t r{};
        std::int16_t g{};
        std::int16_t b{};
        std::int16_t a{};
    };

    struct J3dTevKColor {
        bool valid{};
        std::uint8_t r{};
        std::uint8_t g{};
        std::uint8_t b{};
        std::uint8_t a{};
    };

    struct J3dTevStageArgs {
        std::uint8_t a{};
        std::uint8_t b{};
        std::uint8_t c{};
        std::uint8_t d{};
    };

    struct J3dTevStageOp {
        std::uint8_t op{};
        std::uint8_t bias{};
        std::uint8_t scale{};
        std::uint8_t clamp{};
        std::uint8_t output_register{};
    };

    struct J3dTevStageInfoRaw {
        bool valid{};
        std::array< std::uint8_t, 0x14U > bytes{};
        J3dTevStageArgs color_args{};
        J3dTevStageOp color_op{};
        J3dTevStageArgs alpha_args{};
        J3dTevStageOp alpha_op{};
    };

    struct J3dTevSwapMode {
        bool valid{};
        std::uint8_t ras_sel{};
        std::uint8_t tex_sel{};
        std::array< std::uint8_t, 4U > raw{};
    };

    struct J3dTevSwapModeTable {
        bool valid{};
        std::array< std::uint8_t, 4U > channels{0U, 1U, 2U, 3U};
    };

    struct J3dAlphaCompare {
        bool valid{};
        std::uint8_t comp0{};
        std::uint8_t ref0{};
        std::uint8_t op{};
        std::uint8_t comp1{};
        std::uint8_t ref1{};
        std::array< std::uint8_t, 8U > raw{};
    };

    struct J3dIndirectTextureOrder {
        bool valid{};
        std::uint8_t texture_coordinate{};
        std::uint8_t texture_map{};
        std::array< std::uint8_t, 4U > raw{};
    };

    struct J3dIndirectTextureMatrix {
        bool valid{};
        std::array< float, 6U > values{0.5F, 0.0F, 0.0F, 0.0F, 0.5F, 0.0F};
        std::int8_t scale_exponent{1};
        std::array< std::uint8_t, 0x1CU > raw{};
    };

    struct J3dIndirectTextureCoordScale {
        bool valid{};
        std::uint8_t scale_s{};
        std::uint8_t scale_t{};
        std::array< std::uint8_t, 4U > raw{};
    };

    struct J3dIndirectTevStage {
        bool valid{};
        std::uint8_t ind_stage{};
        std::uint8_t format{};
        std::uint8_t bias{};
        std::uint8_t matrix{};
        std::uint8_t wrap_s{};
        std::uint8_t wrap_t{};
        std::uint8_t add_prev{};
        std::uint8_t use_original_lod{};
        std::uint8_t alpha{};
        std::array< std::uint8_t, 0x0CU > raw{};
    };

    struct J3dZMode {
        bool valid{};
        std::uint8_t compare_enable{};
        std::uint8_t function{};
        std::uint8_t update_enable{};
    };

    struct J3dColorChannelInfo {
        bool valid{};
        std::uint8_t enable{};
        std::uint8_t material_source{};
        std::uint8_t light_mask{};
        std::uint8_t diffuse_function{};
        std::uint8_t attenuation_function{};
        std::uint8_t ambient_source{};
    };

    struct J3dMaterial {
        std::string name{};
        std::uint8_t material_mode{};
        std::uint8_t cull_mode{0xFFU};
        std::uint8_t color_channel_count{};
        std::uint8_t texture_generator_count{};
        std::uint8_t tev_stage_count{};
        std::array< std::uint16_t, 8U > texture_indices{
            J3D_NO_TEXTURE_INDEX, J3D_NO_TEXTURE_INDEX, J3D_NO_TEXTURE_INDEX, J3D_NO_TEXTURE_INDEX,
            J3D_NO_TEXTURE_INDEX, J3D_NO_TEXTURE_INDEX, J3D_NO_TEXTURE_INDEX, J3D_NO_TEXTURE_INDEX,
        };
        std::array< J3dTextureCoordGen, 8U > texture_coord_generators{};
        std::array< J3dTextureMatrix, 8U > texture_matrices{};
        std::array< J3dTevOrder, 16U > tev_orders{};
        std::array< J3dTevColorS10, 4U > tev_colors{};
        std::array< J3dTevKColor, 4U > tev_k_colors{};
        std::array< std::uint8_t, 16U > tev_k_color_selectors{};
        std::array< std::uint8_t, 16U > tev_k_alpha_selectors{};
        std::array< J3dTevStageInfoRaw, 16U > tev_stages{};
        std::array< J3dTevSwapMode, 16U > tev_swap_modes{};
        std::array< J3dTevSwapModeTable, 4U > tev_swap_mode_tables{};
        J3dColor material_color{};
        std::array< J3dColorChannelInfo, 4U > color_channels{};
        J3dAlphaCompare alpha_compare{};
        std::uint8_t indirect_texture_stage_count{};
        std::array< J3dIndirectTextureOrder, 3U > indirect_texture_orders{};
        std::array< J3dIndirectTextureMatrix, 3U > indirect_texture_matrices{};
        std::array< J3dIndirectTextureCoordScale, 3U > indirect_texture_coord_scales{};
        std::array< J3dIndirectTevStage, 16U > indirect_tev_stages{};
        J3dBlendMode blend{};
        J3dZMode z_mode{};
    };

    struct J3dJoint {
        std::string name{};
        std::uint16_t parent_index{J3D_NO_JOINT_INDEX};
        std::uint16_t kind{};
        bool scale_compensate{};
        J3dVec3 scale{};
        std::array< std::int16_t, 3U > rotation{};
        J3dVec3 translation{};
        float bounding_radius{};
        J3dVec3 bounds_min{};
        J3dVec3 bounds_max{};
    };

    struct J3dDrawMatrix {
        bool weighted{};
        std::uint16_t index{};
    };

    struct J3dVertex {
        J3dVec3 position{};
        J3dVec3 normal{};
        J3dColor color{};
        J3dVec2 texcoord{};
        std::uint16_t position_index{J3D_NO_VERTEX_INDEX};
        std::uint16_t normal_index{J3D_NO_VERTEX_INDEX};
        std::uint16_t color_index{J3D_NO_VERTEX_INDEX};
        std::uint16_t texcoord_index{J3D_NO_VERTEX_INDEX};
        std::uint16_t draw_matrix_index{J3D_NO_VERTEX_INDEX};
    };

    struct J3dTriangle {
        J3dVertex v0{};
        J3dVertex v1{};
        J3dVertex v2{};
    };

    struct J3dShapeMatrixGroup {
        std::uint16_t draw_matrix_index{};
        std::uint16_t matrix_count{};
        std::uint32_t first_matrix_table_index{};
        std::uint32_t display_list_offset{};
        std::uint32_t display_list_size{};
    };

    struct J3dShape {
        std::string name{};
        std::uint16_t material_index{};
        std::uint16_t shape_init_index{};
        std::vector< J3dShapeMatrixGroup > matrix_groups{};
        std::vector< J3dTriangle > triangles{};
    };

    struct J3dModel {
        std::vector< J3dTexture > textures{};
        std::vector< J3dMaterial > materials{};
        std::vector< J3dJoint > joints{};
        std::vector< J3dDrawMatrix > draw_matrices{};
        std::vector< J3dShape > shapes{};
    };

    [[nodiscard]] AssetResult< J3dModel > parse_j3d_model(std::span< const std::byte > bdl_bytes);

}  // namespace smgpc::assets::layout
