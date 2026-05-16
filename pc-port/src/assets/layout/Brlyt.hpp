#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "AssetServices.hpp"

namespace smgpc::assets::layout {

struct Vec2 {
    float x {};
    float y {};
};

struct Vec3 {
    float x {};
    float y {};
    float z {};
};

struct Color {
    std::uint8_t r {};
    std::uint8_t g {};
    std::uint8_t b {};
    std::uint8_t a {};
};

struct Insets {
    float left {};
    float right {};
    float top {};
    float bottom {};
};

struct WindowFrameDefinition {
    std::int32_t material_index {-1};
    std::uint8_t texture_flip {};
};

struct TexSrtDefinition {
    Vec2 translate {};
    float rotate {};
    Vec2 scale {1.0F, 1.0F};
};

struct TextureMapDefinition {
    std::int32_t texture_index {-1};
    std::uint8_t wrap_s {};
    std::uint8_t wrap_t {};
    std::uint8_t min_filter {};
    std::uint8_t mag_filter {};
};

struct MaterialBlendDefinition {
    bool enabled {};
    std::uint8_t type {};
    std::uint8_t source_factor {};
    std::uint8_t destination_factor {};
    std::uint8_t operation {};
};

struct MaterialTevStageDefinition {
    std::array<std::uint8_t, 16> raw {};
};

enum class PaneType {
    Pane, Picture, Text, Window
};

enum class MaterialBlendMode : std::uint8_t {
    Alpha,
    Additive,
};

struct MaterialDefinition {
    std::string name {};
    std::int32_t texture_index {-1};
    std::vector<std::int32_t> texture_indices {};
    std::vector<TextureMapDefinition> texture_maps {};
    std::vector<TexSrtDefinition> texture_srts {};
    std::array<std::uint8_t, 4> mat_color {255U, 255U, 255U, 255U};
    std::array<std::uint8_t, 4> texture_color {255U, 255U, 255U, 255U};
    std::array<std::uint8_t, 4> font_color {255U, 255U, 255U, 255U};
    std::int32_t tev_stage_count {};
    std::vector<std::array<std::uint8_t, 4>> texture_coordinate_generators {};
    bool has_channel_control {};
    std::array<std::uint8_t, 4> channel_control {};
    bool has_tev_swap_mode {};
    std::array<std::uint8_t, 4> tev_swap_mode {};
    std::vector<std::array<std::uint8_t, 20>> indirect_texture_srts {};
    std::vector<std::array<std::uint8_t, 4>> indirect_stages {};
    std::vector<MaterialTevStageDefinition> tev_stages {};
    bool has_alpha_compare {};
    std::array<std::uint8_t, 4> alpha_compare {};
    MaterialBlendDefinition blend {};
    MaterialBlendMode blend_mode {MaterialBlendMode::Alpha};
};

struct PaneDefinition {
    PaneType type {PaneType::Pane};
    std::string name {};
    std::string user_data {};
    std::int32_t parent {-1};
    std::vector<std::int32_t> children {};

    bool visible {true};
    bool influenced_alpha {false};
    bool location_adjust {true};
    std::uint8_t base_position {};
    std::uint8_t alpha {255U};

    Vec3 translate {};
    Vec3 rotate {};
    Vec2 scale {1.0F, 1.0F};
    Vec2 size {};

    std::array<Color, 4> vertex_colors {};
    std::int32_t material_index {-1};
    std::array<float, 8> tex_coords {0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F};
    Insets window_content_inflation {};
    std::vector<WindowFrameDefinition> window_frames {};
    std::int32_t window_frame_material_index {-1};

    std::u16string text {};
    std::int32_t font_index {-1};
    std::uint8_t text_position {};
    std::uint8_t text_alignment {};
    std::array<Color, 2> text_colors {};
    Vec2 text_font_size {};
    float text_char_space {};
    float text_line_space {};
};

struct LayoutDefinition {
    bool center_origin {};
    Vec2 size {};
    std::vector<std::string> texture_names {};
    std::vector<std::string> font_names {};
    std::vector<MaterialDefinition> materials {};
    std::vector<PaneDefinition> panes {};
    std::int32_t root_pane {-1};
};

[[nodiscard]] AssetResult<LayoutDefinition> parse_brlyt(std::span<const std::byte> bytes);

}  // namespace smgpc::assets::layout
