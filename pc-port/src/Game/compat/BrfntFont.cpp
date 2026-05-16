#include "BrfntFont.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace smgpc::game {
    namespace {

        [[nodiscard]] std::uint16_t read_be16(std::span< const std::uint8_t > data, std::size_t offset) {
            if (offset + 2U > data.size()) {
                throw std::runtime_error("BRFNT read_be16 out of range");
            }

            return static_cast< std::uint16_t >((static_cast< std::uint16_t >(data[offset]) << 8U) | data[offset + 1U]);
        }

        [[nodiscard]] std::uint32_t read_be32(std::span< const std::uint8_t > data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("BRFNT read_be32 out of range");
            }

            return (static_cast< std::uint32_t >(data[offset]) << 24U) | (static_cast< std::uint32_t >(data[offset + 1U]) << 16U) |
                   (static_cast< std::uint32_t >(data[offset + 2U]) << 8U) | data[offset + 3U];
        }

        [[nodiscard]] std::int8_t read_s8(std::span< const std::uint8_t > data, std::size_t offset) {
            if (offset >= data.size()) {
                throw std::runtime_error("BRFNT read_s8 out of range");
            }

            return std::bit_cast< std::int8_t >(data[offset]);
        }

        [[nodiscard]] bool has_magic(std::span< const std::uint8_t > data, std::size_t offset, const char (&magic)[5]) {
            return offset + 4U <= data.size() && data[offset] == static_cast< std::uint8_t >(magic[0]) &&
                   data[offset + 1U] == static_cast< std::uint8_t >(magic[1]) && data[offset + 2U] == static_cast< std::uint8_t >(magic[2]) &&
                   data[offset + 3U] == static_cast< std::uint8_t >(magic[3]);
        }

        [[nodiscard]] std::size_t checked_offset(std::span< const std::uint8_t > data, std::uint32_t offset) {
            if (offset == 0U || offset >= data.size()) {
                throw std::runtime_error("BRFNT resource offset is invalid");
            }

            return offset;
        }

        [[nodiscard]] BrfntCharWidths read_width(std::span< const std::uint8_t > data, std::size_t offset) {
            if (offset + 3U > data.size()) {
                throw std::runtime_error("BRFNT char width out of range");
            }

            return BrfntCharWidths{
                .left = read_s8(data, offset),
                .glyph_width = data[offset + 1U],
                .char_width = read_s8(data, offset + 2U),
            };
        }

        void parse_width_blocks(BrfntFont& font, std::span< const std::uint8_t > data, std::uint32_t first_offset) {
            auto offset = checked_offset(data, first_offset);
            while (offset != 0U) {
                const auto begin = read_be16(data, offset);
                const auto end = read_be16(data, offset + 2U);
                const auto next = read_be32(data, offset + 4U);
                if (end < begin) {
                    throw std::runtime_error("BRFNT width block has invalid range");
                }

                auto block = BrfntFont::WidthBlock{
                    .begin = begin,
                    .end = end,
                    .widths = {},
                };
                block.widths.reserve(static_cast< std::size_t >(end - begin) + 1U);

                auto width_offset = offset + 8U;
                for (auto index = begin; index <= end; ++index) {
                    block.widths.push_back(read_width(data, width_offset));
                    width_offset += 3U;
                    if (index == UINT16_MAX) {
                        break;
                    }
                }
                font.width_blocks.push_back(std::move(block));

                if (next == 0U) {
                    break;
                }
                offset = checked_offset(data, next);
            }
        }

        void parse_code_maps(BrfntFont& font, std::span< const std::uint8_t > data, std::uint32_t first_offset) {
            auto offset = checked_offset(data, first_offset);
            while (offset != 0U) {
                const auto begin = read_be16(data, offset);
                const auto end = read_be16(data, offset + 2U);
                const auto method = static_cast< BrfntFont::MapMethod >(read_be16(data, offset + 4U));
                const auto next = read_be32(data, offset + 8U);
                if (end < begin) {
                    throw std::runtime_error("BRFNT code map has invalid range");
                }

                auto map = BrfntFont::CodeMap{
                    .begin = begin,
                    .end = end,
                    .method = method,
                    .direct_start = 0U,
                    .table = {},
                    .scan_entries = {},
                };

                switch (method) {
                case BrfntFont::MapMethod::Direct:
                    map.direct_start = read_be16(data, offset + 12U);
                    break;
                case BrfntFont::MapMethod::Table: {
                    const auto count = static_cast< std::size_t >(end - begin) + 1U;
                    map.table.reserve(count);
                    for (auto i = 0U; i < count; ++i) {
                        map.table.push_back(read_be16(data, offset + 12U + static_cast< std::size_t >(i) * 2U));
                    }
                    break;
                }
                case BrfntFont::MapMethod::Scan: {
                    const auto count = read_be16(data, offset + 12U);
                    map.scan_entries.reserve(count);
                    for (auto i = 0U; i < count; ++i) {
                        const auto entry_offset = offset + 14U + static_cast< std::size_t >(i) * 4U;
                        map.scan_entries.emplace_back(read_be16(data, entry_offset), read_be16(data, entry_offset + 2U));
                    }
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported BRFNT code map method");
                }

                font.code_maps.push_back(std::move(map));

                if (next == 0U) {
                    break;
                }
                offset = checked_offset(data, next);
            }
        }

        void parse_sheet_textures(BrfntFont& font, std::span< const std::uint8_t > data, std::size_t glyph_offset) {
            if (glyph_offset + 24U > data.size()) {
                throw std::runtime_error("BRFNT glyph block is truncated");
            }

            font.cell_width = data[glyph_offset];
            font.cell_height = data[glyph_offset + 1U];
            font.baseline_position = read_s8(data, glyph_offset + 2U);
            font.max_char_width = data[glyph_offset + 3U];

            const auto sheet_size = read_be32(data, glyph_offset + 4U);
            const auto sheet_count = read_be16(data, glyph_offset + 8U);
            font.sheet_format = static_cast< TplTextureFormat >(read_be16(data, glyph_offset + 10U));
            font.sheet_row = read_be16(data, glyph_offset + 12U);
            font.sheet_line = read_be16(data, glyph_offset + 14U);
            font.sheet_width = read_be16(data, glyph_offset + 16U);
            font.sheet_height = read_be16(data, glyph_offset + 18U);
            const auto sheet_image_offset = checked_offset(data, read_be32(data, glyph_offset + 20U));
            if (sheet_size == 0U) {
                throw std::runtime_error("BRFNT sheet size is zero");
            }

            font.sheets.reserve(sheet_count);
            for (auto sheet = 0U; sheet < sheet_count; ++sheet) {
                const auto offset = sheet_image_offset + static_cast< std::size_t >(sheet) * sheet_size;
                if (offset + sheet_size > data.size()) {
                    throw std::runtime_error("BRFNT sheet data is truncated");
                }

                font.sheets.push_back(
                    decode_raw_gx_texture(data.subspan(offset, sheet_size), font.sheet_width, font.sheet_height, font.sheet_format));
            }
        }

        [[nodiscard]] std::optional< std::uint16_t > find_glyph_index(const BrfntFont::CodeMap& map, std::uint16_t code) {
            if (code < map.begin || code > map.end) {
                return std::nullopt;
            }

            switch (map.method) {
            case BrfntFont::MapMethod::Direct:
                return static_cast< std::uint16_t >(map.direct_start + (code - map.begin));
            case BrfntFont::MapMethod::Table: {
                const auto offset = static_cast< std::size_t >(code - map.begin);
                if (offset < map.table.size() && map.table[offset] != UINT16_MAX) {
                    return map.table[offset];
                }
                return std::nullopt;
            }
            case BrfntFont::MapMethod::Scan: {
                const auto it = std::ranges::find_if(map.scan_entries, [code](const auto& entry) { return entry.first == code; });
                return it == map.scan_entries.end() ? std::nullopt : std::optional< std::uint16_t >(it->second);
            }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional< std::uint16_t > find_glyph_index(const std::vector< BrfntFont::CodeMap >& maps, std::uint16_t code) {
            for (const auto& map : maps) {
                if (auto glyph_index = find_glyph_index(map, code)) {
                    return glyph_index;
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional< std::uint16_t > fullwidth_ascii_to_ascii(std::uint16_t code) {
            if (code >= 0xff01U && code <= 0xff5eU) {
                return static_cast< std::uint16_t >(code - 0xfee0U);
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional< BrfntGlyph > glyph_for_index(const BrfntFont& font, std::uint16_t glyph_index) {
            const auto cells_per_sheet = static_cast< std::uint32_t >(font.sheet_row) * font.sheet_line;
            if (cells_per_sheet == 0U) {
                return std::nullopt;
            }

            const auto sheet_index = static_cast< std::uint16_t >(glyph_index / cells_per_sheet);
            if (sheet_index >= font.sheets.size()) {
                return std::nullopt;
            }

            const auto glyph_cell = static_cast< std::uint32_t >(glyph_index % cells_per_sheet);
            const auto unit_x = static_cast< std::uint16_t >(glyph_cell % font.sheet_row);
            const auto unit_y = static_cast< std::uint16_t >(glyph_cell / font.sheet_row);
            return BrfntGlyph{
                .sheet_index = sheet_index,
                .x = static_cast< std::uint16_t >(unit_x * (font.cell_width + 1U) + 1U),
                .y = static_cast< std::uint16_t >(unit_y * (font.cell_height + 1U) + 1U),
                .width = font.cell_width,
                .height = font.cell_height,
                .widths = font.widths_for_glyph(glyph_index),
            };
        }

    }  // namespace

    std::optional< BrfntGlyph > BrfntFont::glyph_for_exact(std::uint16_t code) const {
        const auto glyph_index = find_glyph_index(code_maps, code);
        if (!glyph_index.has_value()) {
            return std::nullopt;
        }

        return glyph_for_index(*this, *glyph_index);
    }

    std::optional< BrfntGlyph > BrfntFont::glyph_for(std::uint16_t code) const {
        if (auto glyph = glyph_for_exact(code)) {
            return glyph;
        }
        if (const auto normalized = fullwidth_ascii_to_ascii(code)) {
            if (auto glyph = glyph_for_exact(*normalized)) {
                return glyph;
            }
        }

        return glyph_for_index(*this, alternate_char_index);
    }

    BrfntCharWidths BrfntFont::widths_for_glyph(std::uint16_t glyph_index) const {
        for (const auto& block : width_blocks) {
            if (block.begin <= glyph_index && glyph_index <= block.end) {
                return block.widths.at(glyph_index - block.begin);
            }
        }

        return default_width;
    }

    BrfntFont parse_brfnt_font(std::span< const std::uint8_t > data) {
        if (!has_magic(data, 0U, "RFNT")) {
            throw std::runtime_error("BRFNT file is missing RFNT magic");
        }
        if (read_be16(data, 4U) != 0xFEFFU) {
            throw std::runtime_error("BRFNT file is not big-endian");
        }

        const auto header_size = read_be16(data, 12U);
        const auto block_count = read_be16(data, 14U);
        auto cursor = static_cast< std::size_t >(header_size);
        auto font = BrfntFont{};

        auto finf_offset = std::optional< std::size_t >{};
        for (auto i = 0U; i < block_count; ++i) {
            if (cursor + 8U > data.size()) {
                throw std::runtime_error("BRFNT block header is truncated");
            }

            const auto block_size = read_be32(data, cursor + 4U);
            if (block_size < 8U || cursor + block_size > data.size()) {
                throw std::runtime_error("BRFNT block size is invalid");
            }

            if (has_magic(data, cursor, "FINF")) {
                finf_offset = cursor + 8U;
                break;
            }

            cursor += block_size;
        }

        if (!finf_offset.has_value()) {
            throw std::runtime_error("BRFNT missing FINF block");
        }

        const auto finf = *finf_offset;
        font.line_feed = read_s8(data, finf + 1U);
        font.alternate_char_index = read_be16(data, finf + 2U);
        font.default_width = read_width(data, finf + 4U);
        const auto glyph_offset = checked_offset(data, read_be32(data, finf + 8U));
        const auto width_offset = read_be32(data, finf + 12U);
        const auto map_offset = read_be32(data, finf + 16U);
        font.height = data[finf + 20U];
        font.width = data[finf + 21U];
        font.ascent = data[finf + 22U];

        parse_sheet_textures(font, data, glyph_offset);
        if (width_offset != 0U) {
            parse_width_blocks(font, data, width_offset);
        }
        if (map_offset != 0U) {
            parse_code_maps(font, data, map_offset);
        }

        return font;
    }

}  // namespace smgpc::game
