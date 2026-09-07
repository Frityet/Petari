#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "resource/TplTexture.hpp"

namespace smgpc::layout {

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

    struct BrfntBlockInfo {
        std::string magic;
        std::size_t offset = 0U;
        std::size_t size = 0U;
    };

    struct BrfntFont {
        std::uint32_t declared_file_size = 0U;
        std::uint16_t header_size = 0U;
        std::uint16_t block_count = 0U;
        std::uint8_t font_type = 0U;
        std::uint8_t encoding = 0U;
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
        std::uint32_t sheet_size = 0U;
        std::uint16_t sheet_count = 0U;
        std::uint32_t sheet_image_offset = 0U;
        smgpc::resource::TplTextureFormat sheet_format = smgpc::resource::TplTextureFormat::I4;
        std::vector<BrfntBlockInfo> blocks;
        std::vector<smgpc::resource::DecodedTexture> sheets;

        [[nodiscard]] std::optional<std::uint16_t> find_glyph_index_exact(std::uint16_t code) const;
        [[nodiscard]] std::uint16_t resolved_glyph_index(std::uint16_t code) const;
        [[nodiscard]] bool has_glyph(std::uint16_t code) const;
        [[nodiscard]] std::optional<BrfntGlyph> glyph_for_exact(std::uint16_t code) const;
        [[nodiscard]] std::optional<BrfntGlyph> glyph_for(std::uint16_t code) const;
        [[nodiscard]] std::optional<BrfntGlyph> glyph_for_resfont(std::uint16_t code) const;
        [[nodiscard]] BrfntCharWidths char_widths(std::uint16_t code) const;
        [[nodiscard]] int char_width(std::uint16_t code) const;
        [[nodiscard]] BrfntCharWidths widths_for_glyph(std::uint16_t glyph_index) const;

        struct WidthBlock {
            std::uint16_t begin = 0U;
            std::uint16_t end = 0U;
            std::vector<BrfntCharWidths> widths;
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
            std::vector<std::uint16_t> table;
            std::vector<std::pair<std::uint16_t, std::uint16_t>> scan_entries;
        };

        BrfntCharWidths default_width{};
        std::uint16_t alternate_char_index = 0U;
        std::uint16_t sheet_row = 0U;
        std::uint16_t sheet_line = 0U;
        std::vector<WidthBlock> width_blocks;
        std::vector<CodeMap> code_maps;
    };

    [[nodiscard]] BrfntFont parse_brfnt_font(std::span<const std::uint8_t> data);

}  // namespace smgpc::layout
