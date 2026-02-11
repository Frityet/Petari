#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "AssetServices.hpp"
#include "Tpl.hpp"

namespace smgpc::assets::layout {

struct BrfntCharWidths {
    std::int8_t left {};
    std::uint8_t glyph_width {};
    std::int8_t char_width {};
};

struct BrfntGlyph {
    std::uint16_t glyph_index {};
    std::uint16_t sheet_index {};
    std::uint16_t cell_x {};
    std::uint16_t cell_y {};
    BrfntCharWidths widths {};
};

class BrfntFont {
public:
    struct WidthRange {
        std::uint16_t begin {};
        std::uint16_t end {};
        std::vector<BrfntCharWidths> widths {};
    };

    struct CodeMap {
        std::uint16_t begin {};
        std::uint16_t end {};
        std::uint16_t mapping_method {};
        std::vector<std::uint16_t> map_info {};
    };

    [[nodiscard]] const std::string &name() const;
    [[nodiscard]] std::uint8_t line_feed() const;
    [[nodiscard]] std::uint8_t ascent() const;
    [[nodiscard]] std::uint8_t cell_width() const;
    [[nodiscard]] std::uint8_t cell_height() const;

    [[nodiscard]] const std::vector<tpl::DecodedImage> &sheets() const;

    [[nodiscard]] std::uint16_t map_codepoint_to_glyph(std::uint16_t codepoint) const;
    [[nodiscard]] bool get_glyph(std::uint16_t codepoint, BrfntGlyph *out) const;

private:
    friend AssetResult<BrfntFont> parse_brfnt(std::span<const std::byte> bytes, std::string name_hint);

    [[nodiscard]] std::uint16_t find_glyph_index(std::uint16_t codepoint) const;
    [[nodiscard]] BrfntCharWidths width_for_index(std::uint16_t index) const;

    std::string _name {};

    std::uint16_t _alter_char_index {};
    BrfntCharWidths _default_width {};

    std::uint8_t _line_feed {};
    std::uint8_t _ascent {};
    std::uint8_t _cell_width {};
    std::uint8_t _cell_height {};
    std::uint8_t _max_char_width {};

    std::uint16_t _sheet_num {};
    std::uint16_t _sheet_format {};
    std::uint16_t _sheet_row {};
    std::uint16_t _sheet_line {};
    std::uint16_t _sheet_width {};
    std::uint16_t _sheet_height {};

    std::vector<tpl::DecodedImage> _sheets {};
    std::vector<WidthRange> _width_ranges {};
    std::vector<CodeMap> _code_maps {};
};

[[nodiscard]] AssetResult<BrfntFont> parse_brfnt(std::span<const std::byte> bytes, std::string name_hint = {});

}  // namespace smgpc::assets::layout
