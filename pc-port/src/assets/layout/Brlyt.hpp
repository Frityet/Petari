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

enum class PaneType {
    Pane, Picture, Text
};

struct MaterialDefinition {
    std::string name {};
    std::int32_t texture_index {-1};
    std::vector<std::int32_t> texture_indices {};
    std::array<std::uint8_t, 4> mat_color {255U, 255U, 255U, 255U};
    std::int32_t tev_stage_count {};
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
