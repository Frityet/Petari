#pragma once

#include "GXState.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace smgpc::game {

struct BrlytPane {
    std::string name;
    std::int32_t parent_index = -1;
    float translate_x = 0.0F;
    float translate_y = 0.0F;
    float scale_x = 1.0F;
    float scale_y = 1.0F;
    float width = 0.0F;
    float height = 0.0F;
    std::uint8_t base_position = 0U;
    std::uint8_t alpha = 255U;
    bool visible = true;
};

struct BrlytTexCoord {
    float u = 0.0F;
    float v = 0.0F;
};

struct BrlytMaterialTexture {
    std::uint16_t texture_index = 0U;
    std::string texture_name;
    std::uint8_t wrap_s = 0U;
    std::uint8_t wrap_t = 0U;
    std::uint8_t min_filter = 0U;
    std::uint8_t mag_filter = 0U;
};

struct BrlytTexSrt {
    float translate_s = 0.0F;
    float translate_t = 0.0F;
    float rotate = 0.0F;
    float scale_s = 1.0F;
    float scale_t = 1.0F;
};

struct BrlytTexCoordGen {
    std::uint8_t tex_gen_type = 0U;
    std::uint8_t tex_gen_src = 0U;
    std::uint8_t tex_mtx = 0U;
};

struct BrlytTevStageInOp {
    std::uint8_t a = 0U;
    std::uint8_t b = 0U;
    std::uint8_t c = 0U;
    std::uint8_t d = 0U;
    std::uint8_t op = 0U;
    std::uint8_t bias = 0U;
    std::uint8_t scale = 0U;
    std::uint8_t out_reg = 0U;
    std::uint8_t k_sel = 0U;
    bool clamp = true;
};

struct BrlytTevStage {
    std::uint8_t tex_coord_gen = 0U;
    std::uint8_t color_chan = 0U;
    std::uint16_t tex_map = 0U;
    std::uint8_t ras_swap = 0U;
    std::uint8_t tex_swap = 0U;
    BrlytTevStageInOp color {};
    BrlytTevStageInOp alpha {};
};

struct BrlytAlphaCompare {
    std::uint8_t comp0 = 7U;
    std::uint8_t ref0 = 0U;
    std::uint8_t op = 0U;
    std::uint8_t comp1 = 7U;
    std::uint8_t ref1 = 0U;
    bool enabled = false;
};

struct BrlytBlendMode {
    std::uint8_t type = 1U;
    std::uint8_t src_factor = 4U;
    std::uint8_t dst_factor = 5U;
    std::uint8_t op = 3U;
    bool enabled = false;
};

struct BrlytMaterial {
    std::string name;
    std::vector<BrlytMaterialTexture> textures;
    std::vector<BrlytTexSrt> tex_srts;
    std::vector<BrlytTexCoordGen> tex_coord_gens;
    std::array<std::array<std::uint8_t, 4U>, 3U> tev_colors {
        std::array<std::uint8_t, 4U> {0U, 0U, 0U, 0U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
    };
    std::array<std::array<std::uint8_t, 4U>, 4U> tev_k_colors {
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
    };
    std::vector<BrlytTevStage> tev_stages;
    std::array<std::uint8_t, 4U> mat_color {255U, 255U, 255U, 255U};
    BrlytAlphaCompare alpha_compare {};
    BrlytBlendMode blend_mode {};
    std::uint8_t chan_color_src = 1U;
    std::uint8_t chan_alpha_src = 1U;
    bool has_chan_ctrl = false;
    bool has_mat_color = false;
    GXMaterialState gx_state {};
};

struct BrlytPicturePane {
    std::string name;
    std::string texture_name;
    std::size_t pane_index = 0U;
    std::uint16_t material_index = 0U;
    std::uint8_t wrap_s = 0U;
    std::uint8_t wrap_t = 0U;
    std::uint8_t min_filter = 0U;
    std::uint8_t mag_filter = 0U;
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
    std::array<std::uint8_t, 4U> color {255U, 255U, 255U, 255U};
    std::array<std::array<std::uint8_t, 4U>, 4U> vertex_colors {
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
        std::array<std::uint8_t, 4U> {255U, 255U, 255U, 255U},
    };
    std::array<BrlytTexCoord, 4U> tex_coords {
        BrlytTexCoord {0.0F, 0.0F},
        BrlytTexCoord {1.0F, 0.0F},
        BrlytTexCoord {1.0F, 1.0F},
        BrlytTexCoord {0.0F, 1.0F},
    };
    bool visible = true;
};

struct BrlytTextBox {
    std::string name;
    std::string font_name;
    std::vector<std::uint16_t> text;
    std::size_t pane_index = 0U;
    std::uint16_t material_index = 0U;
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
    float font_width = 0.0F;
    float font_height = 0.0F;
    float char_space = 0.0F;
    float line_space = 0.0F;
    std::uint8_t text_position = 0U;
    std::uint8_t text_alignment = 0U;
    std::array<std::uint8_t, 4U> color {255U, 255U, 255U, 255U};
    std::array<std::uint8_t, 4U> color_mapping_min {0U, 0U, 0U, 0U};
    std::array<std::uint8_t, 4U> color_mapping_max {255U, 255U, 255U, 255U};
    bool visible = true;
};

struct BrlytLayout {
    float width = 0.0F;
    float height = 0.0F;
    std::vector<std::string> texture_names;
    std::vector<std::string> font_names;
    std::vector<BrlytMaterial> materials;
    std::vector<BrlytPane> panes;
    std::vector<BrlytPicturePane> pictures;
    std::vector<BrlytTextBox> text_boxes;
};

[[nodiscard]] BrlytLayout parse_brlyt_layout(std::span<const std::uint8_t> data);

}  // namespace smgpc::game
