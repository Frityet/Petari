#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "Game/compat/TplTexture.hpp"

namespace smgpc::game {

    struct BrfntCharWidths {
        std::int8_t left = 0;
        std::uint8_t glyph_width = 0U;
        std::int8_t char_width = 0;
    };

    struct BrfntGlyph {
        std::uint16_t sheet_index = 0U;
        std::uint16_t x = 0U;
        std::uint16_t y = 0U;
        std::uint8_t width = 0U;
        std::uint8_t height = 0U;
        BrfntCharWidths widths{};
    };

    struct BrfntFont {
        std::uint8_t height = 0U;
        std::uint8_t width = 0U;
        std::uint8_t ascent = 0U;
        std::int8_t line_feed = 0;
        std::uint8_t cell_width = 0U;
        std::uint8_t cell_height = 0U;
        std::int8_t baseline_position = 0;
        std::uint8_t max_char_width = 0U;
        std::uint16_t sheet_width = 0U;
        std::uint16_t sheet_height = 0U;
        TplTextureFormat sheet_format = TplTextureFormat::I4;
        std::vector< DecodedTexture > sheets;

        [[nodiscard]] std::optional< BrfntGlyph > glyph_for_exact(std::uint16_t code) const;
        [[nodiscard]] std::optional< BrfntGlyph > glyph_for(std::uint16_t code) const;
        [[nodiscard]] BrfntCharWidths widths_for_glyph(std::uint16_t glyph_index) const;

        struct WidthBlock {
            std::uint16_t begin = 0U;
            std::uint16_t end = 0U;
            std::vector< BrfntCharWidths > widths;
        };

        enum class MapMethod : std::uint16_t {
            Direct = 0U,
            Table = 1U,
            Scan = 2U,
        };

        struct CodeMap {
            std::uint16_t begin = 0U;
            std::uint16_t end = 0U;
            MapMethod method = MapMethod::Direct;
            std::uint16_t direct_start = 0U;
            std::vector< std::uint16_t > table;
            std::vector< std::pair< std::uint16_t, std::uint16_t > > scan_entries;
        };

        BrfntCharWidths default_width{};
        std::uint16_t alternate_char_index = 0U;
        std::uint16_t sheet_row = 0U;
        std::uint16_t sheet_line = 0U;
        std::vector< WidthBlock > width_blocks;
        std::vector< CodeMap > code_maps;
    };

    [[nodiscard]] BrfntFont parse_brfnt_font(std::span< const std::uint8_t > data);

}  // namespace smgpc::game
