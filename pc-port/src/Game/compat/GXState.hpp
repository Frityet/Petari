#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace smgpc::game {

    struct J3dMaterialSummary;
    struct BrlytMaterial;

    struct GXTextureBindingState {
        std::uint8_t slot = 0U;
        std::uint16_t texture_index = 0xffffU;
        std::uint8_t wrap_s = 0xffU;
        std::uint8_t wrap_t = 0xffU;
        std::uint8_t min_filter = 0xffU;
        std::uint8_t mag_filter = 0xffU;
    };

    struct GXTexCoordGenState {
        std::uint8_t slot = 0U;
        std::uint8_t type = 0U;
        std::uint8_t source = 0U;
        std::uint8_t matrix = 0xffU;
    };

    struct GXTexMatrixState {
        std::uint8_t slot = 0U;
        std::uint8_t projection = 0U;
        std::uint8_t info = 0U;
        std::array<float, 3U> center = {};
        float scale_s = 1.0F;
        float scale_t = 1.0F;
        std::int16_t rotation = 0;
        float translate_s = 0.0F;
        float translate_t = 0.0F;
        std::array<float, 16U> effect_matrix = {};
    };

    struct GXTevOrderState {
        std::uint8_t stage = 0U;
        std::uint8_t tex_coord = 0xffU;
        std::uint8_t tex_map = 0xffU;
        std::uint8_t color_channel = 0xffU;
    };

    struct GXTevStageState {
        std::uint8_t stage = 0U;
        std::array<std::uint8_t, 20U> raw = {};
        std::array<std::uint8_t, 4U> color_in = {};
        std::uint8_t color_op = 0U;
        std::uint8_t color_bias = 0U;
        std::uint8_t color_scale = 0U;
        std::uint8_t color_clamp = 1U;
        std::uint8_t color_out = 0U;
        std::uint8_t k_color_sel = 0xffU;
        std::array<std::uint8_t, 4U> alpha_in = {};
        std::uint8_t alpha_op = 0U;
        std::uint8_t alpha_bias = 0U;
        std::uint8_t alpha_scale = 0U;
        std::uint8_t alpha_clamp = 1U;
        std::uint8_t alpha_out = 0U;
        std::uint8_t k_alpha_sel = 0xffU;
    };

    using GXTevRegisterColor = std::array<std::int16_t, 4U>;

    using GXColorValue = std::array<std::uint8_t, 4U>;

    struct GXColorChannelControlState {
        std::uint32_t raw = 0x400U;
        std::uint8_t material_source = 0U;
        bool lighting_enabled = false;
        std::uint8_t light_mask = 0U;
        std::uint8_t ambient_source = 0U;
        std::uint8_t diffuse_function = 0U;
        std::uint8_t attenuation_function = 2U;
        std::uint8_t attenuation_mode = 2U;
    };

    struct GXColorChannelState {
        GXColorValue material_color = {255U, 255U, 255U, 255U};
        GXColorValue ambient_color = {0x32U, 0x32U, 0x32U, 0x32U};
        GXColorChannelControlState color_control = {};
        GXColorChannelControlState alpha_control = {};
    };

    struct GXLightState {
        bool loaded = false;
        GXColorValue color = {};
        std::array<float, 3U> cosine_attenuation = {1.0F, 0.0F, 0.0F};
        std::array<float, 3U> distance_attenuation = {1.0F, 0.0F, 0.0F};
        std::array<float, 3U> position = {};
        std::array<float, 3U> direction = {0.0F, 0.0F, -1.0F};
        std::array<std::uint32_t, 16U> raw_words = {};
        std::array<bool, 16U> word_loaded = {};
    };

    struct GXAlphaCompareState {
        std::uint8_t comp0 = 7U;
        std::uint8_t ref0 = 0U;
        std::uint8_t op = 0U;
        std::uint8_t comp1 = 7U;
        std::uint8_t ref1 = 0U;
        bool enabled = false;
    };

    struct GXBlendState {
        std::uint8_t type = 0U;
        std::uint8_t src_factor = 1U;
        std::uint8_t dst_factor = 0U;
        std::uint8_t op = 3U;
        bool enabled = false;
    };

    struct GXZModeState {
        std::uint8_t compare_enable = 1U;
        std::uint8_t function = 3U;
        std::uint8_t update_enable = 1U;
        bool enabled = false;
    };

    struct GXFogState {
        bool enabled = false;
        std::uint8_t type = 0U;
        std::uint8_t projection = 0U;
        bool range_adjust_enabled = false;
        std::uint16_t range_center = 0U;
        float a = 0.0F;
        float c = 0.0F;
        std::uint32_t b_magnitude = 0U;
        std::uint32_t b_shift = 0U;
        std::array<std::uint8_t, 4U> color = {};
        std::array<float, 10U> range_k = {};
        std::array<std::uint8_t, 44U> raw = {};
    };

    struct GXIndirectTextureOrderState {
        std::uint8_t stage = 0U;
        std::uint8_t tex_map = 0xffU;
        std::uint8_t tex_coord = 0xffU;
    };

    struct GXIndirectTextureMatrixState {
        std::uint8_t matrix = 0U;
        std::int16_t ma = 0;
        std::int16_t mb = 0;
        std::int16_t mc = 0;
        std::int16_t md = 0;
        std::int16_t me = 0;
        std::int16_t mf = 0;
        std::uint8_t scale = 0U;
        std::array<std::uint32_t, 3U> raw{};
    };

    struct GXIndirectTextureCoordScaleState {
        std::uint8_t stage = 0U;
        std::uint8_t scale_s = 0U;
        std::uint8_t scale_t = 0U;
    };

    struct GXIndirectTevStageState {
        std::uint8_t tev_stage = 0U;
        std::uint8_t ind_stage = 0U;
        std::uint8_t format = 0U;
        std::uint8_t bias = 0U;
        std::uint8_t bump_alpha = 0U;
        std::uint8_t matrix_index = 0U;
        std::uint8_t matrix_id = 0U;
        std::uint8_t wrap_s = 0U;
        std::uint8_t wrap_t = 0U;
        bool use_original_lod = false;
        bool add_previous = false;
        bool active = false;
        std::uint32_t raw = 0U;
    };

    struct GXIndirectState {
        std::uint8_t stage_count = 0U;
        std::vector<GXIndirectTextureOrderState> texture_orders;
        std::vector<GXIndirectTextureMatrixState> texture_matrices;
        std::vector<GXIndirectTextureCoordScaleState> texture_coord_scales;
        std::vector<GXIndirectTevStageState> tev_stages;
    };

    enum class GXRegisterSpace {
        BP,
        CP,
        XF,
        IndexedA,
        IndexedB,
        IndexedC,
        IndexedD,
        Unknown,
    };

    struct GXRegisterLoadState {
        GXRegisterSpace space = GXRegisterSpace::Unknown;
        std::uint32_t byte_offset = 0U;
        std::uint16_t address = 0U;
        std::uint8_t count = 1U;
        std::uint32_t value = 0U;
    };

    struct GXDisplayListStats {
        std::uint32_t parsed_bytes = 0U;
        std::uint32_t command_count = 0U;
        std::uint32_t bp_load_count = 0U;
        std::uint32_t cp_load_count = 0U;
        std::uint32_t xf_load_count = 0U;
        std::uint32_t indexed_load_count = 0U;
        std::uint32_t primitive_count = 0U;
        std::uint32_t unknown_opcode_count = 0U;
    };

    struct GXMaterialState {
        std::string source;
        std::string name;
        std::uint8_t material_mode = 0U;
        std::uint8_t cull_mode = 0xffU;
        std::uint8_t color_channel_count = 0U;
        std::uint8_t texgen_count = 0U;
        std::uint8_t tev_stage_count = 0U;
        std::uint8_t z_comp_loc = 0xffU;
        std::array<GXColorChannelState, 2U> color_channels = {};
        std::array<GXLightState, 8U> lights = {};
        std::array<std::array<std::uint8_t, 4U>, 4U> tev_k_colors = {
            std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U},
            std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U},
            std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U},
            std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U},
        };
        std::array<GXTevRegisterColor, 4U> tev_registers = {};
        std::vector<GXTextureBindingState> textures;
        std::vector<GXTexCoordGenState> tex_coord_gens;
        std::vector<GXTexMatrixState> tex_matrices;
        std::vector<GXTevOrderState> tev_orders;
        std::vector<GXTevStageState> tev_stages;
        GXAlphaCompareState alpha_compare = {};
        GXBlendState blend = {};
        GXZModeState z_mode = {};
        GXFogState fog = {};
        GXIndirectState indirect = {};
        std::vector<std::uint8_t> mdl3_display_list;
        GXDisplayListStats mdl3_stats = {};
        std::vector<GXRegisterLoadState> mdl3_register_loads;
    };

    [[nodiscard]] GXColorValue gx_color_from_xf_value(std::uint32_t value);
    [[nodiscard]] GXColorChannelControlState gx_color_channel_control_from_xf(std::uint32_t value);
    [[nodiscard]] GXColorChannelControlState gx_color_channel_control_from_j3d(std::uint8_t enable, std::uint8_t material_source,
                                                                               std::uint8_t light_mask, std::uint8_t diffuse_function,
                                                                               std::uint8_t attenuation_function, std::uint8_t ambient_source);
    [[nodiscard]] GXColorValue gx_evaluate_lit_raster_color(const GXMaterialState &state, std::uint8_t color_channel, GXColorValue vertex_color,
                                                            std::array<float, 3U> position, std::array<float, 3U> normal);
    [[nodiscard]] GXMaterialState gx_state_from_j3d_material(const J3dMaterialSummary &material);
    [[nodiscard]] GXMaterialState gx_state_from_brlyt_material(const BrlytMaterial &material);
    void gx_apply_mdl3_display_list(GXMaterialState &state, std::span<const std::uint8_t> display_list);

}  // namespace smgpc::game
