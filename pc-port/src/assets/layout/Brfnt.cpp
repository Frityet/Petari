#include "Brfnt.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include "Binary.hpp"

namespace smgpc::assets::layout {
    namespace {

        struct FinfInfo {
            std::uint8_t font_type{};
            std::uint16_t alter_char_index{};
            BrfntCharWidths default_width{};
            std::uint8_t encoding{};
            std::uint8_t line_feed{};
            std::uint8_t height{};
            std::uint8_t width{};
            std::uint8_t ascent{};
            std::uint8_t reserved{};

            std::size_t glyph_offset{};
            std::size_t width_offset{};
            std::size_t map_offset{};
        };

        struct TglpInfo {
            std::uint8_t cell_width{};
            std::uint8_t cell_height{};
            std::uint8_t baseline_pos{};
            std::uint8_t max_char_width{};

            std::uint32_t sheet_size{};
            std::uint16_t sheet_num{};
            std::uint16_t sheet_format{};
            std::uint16_t sheet_row{};
            std::uint16_t sheet_line{};
            std::uint16_t sheet_width{};
            std::uint16_t sheet_height{};
            std::size_t sheet_image_offset{};
            std::uint32_t reserved{};
        };

        [[nodiscard]] AssetError make_error(std::string message) {
            return AssetError{
                .code = AssetErrorCode::InvalidFormat,
                .message = std::move(message),
            };
        }

        [[nodiscard]] AssetResult< FinfInfo > parse_finf(std::span< const std::byte > bytes, std::size_t finf_data_offset) {
            using namespace binary;

            if (not has_bytes(bytes, finf_data_offset, 24U)) {
                return make_error("BRFNT FINF payload is too small.");
            }

            FinfInfo finf{};
            finf.font_type = read_u8(bytes, finf_data_offset + 0U);
            finf.line_feed = read_u8(bytes, finf_data_offset + 1U);
            finf.alter_char_index = read_u16_be(bytes, finf_data_offset + 2U);
            finf.default_width.left = static_cast< std::int8_t >(read_u8(bytes, finf_data_offset + 4U));
            finf.default_width.glyph_width = read_u8(bytes, finf_data_offset + 5U);
            finf.default_width.char_width = static_cast< std::int8_t >(read_u8(bytes, finf_data_offset + 6U));
            finf.encoding = read_u8(bytes, finf_data_offset + 7U);

            finf.glyph_offset = static_cast< std::size_t >(read_u32_be(bytes, finf_data_offset + 8U));
            finf.width_offset = static_cast< std::size_t >(read_u32_be(bytes, finf_data_offset + 12U));
            finf.map_offset = static_cast< std::size_t >(read_u32_be(bytes, finf_data_offset + 16U));

            finf.height = read_u8(bytes, finf_data_offset + 20U);
            finf.width = read_u8(bytes, finf_data_offset + 21U);
            finf.ascent = read_u8(bytes, finf_data_offset + 22U);
            finf.reserved = read_u8(bytes, finf_data_offset + 23U);

            return finf;
        }

        [[nodiscard]] AssetResult< TglpInfo > parse_tglp(std::span< const std::byte > bytes, std::size_t tglp_data_offset) {
            using namespace binary;

            if (not has_bytes(bytes, tglp_data_offset, 28U)) {
                return make_error("BRFNT TGLP payload is too small.");
            }

            TglpInfo tglp{};
            tglp.cell_width = read_u8(bytes, tglp_data_offset + 0U);
            tglp.cell_height = read_u8(bytes, tglp_data_offset + 1U);
            tglp.baseline_pos = read_u8(bytes, tglp_data_offset + 2U);
            tglp.max_char_width = read_u8(bytes, tglp_data_offset + 3U);
            tglp.sheet_size = read_u32_be(bytes, tglp_data_offset + 4U);
            tglp.sheet_num = read_u16_be(bytes, tglp_data_offset + 8U);
            tglp.sheet_format = read_u16_be(bytes, tglp_data_offset + 10U);
            tglp.sheet_row = read_u16_be(bytes, tglp_data_offset + 12U);
            tglp.sheet_line = read_u16_be(bytes, tglp_data_offset + 14U);
            tglp.sheet_width = read_u16_be(bytes, tglp_data_offset + 16U);
            tglp.sheet_height = read_u16_be(bytes, tglp_data_offset + 18U);
            tglp.sheet_image_offset = static_cast< std::size_t >(read_u32_be(bytes, tglp_data_offset + 20U));
            tglp.reserved = read_u32_be(bytes, tglp_data_offset + 24U);

            return tglp;
        }

        void normalize_font_sheet_to_alpha_mask(tpl::DecodedImage* image) {
            if (image == nullptr) {
                return;
            }

            for (std::size_t index = 0U; index + 3U < image->rgba8.size(); index += 4U) {
                image->rgba8[index + 0U] = 255U;
                image->rgba8[index + 1U] = 255U;
                image->rgba8[index + 2U] = 255U;
            }
        }

        [[nodiscard]] AssetResult< void > parse_width_chain(std::span< const std::byte > bytes, std::size_t first_width_offset,
                                                            std::vector< BrfntFont::WidthRange >* ranges) {
            using namespace binary;

            if (ranges == nullptr) {
                return AssetResult< void >(make_error("BRFNT width output pointer is null."));
            }

            std::size_t current_offset = first_width_offset;
            while (current_offset != 0U) {
                if (not has_bytes(bytes, current_offset, 8U)) {
                    return AssetResult< void >(make_error("BRFNT CWDH payload header exceeds file bounds."));
                }

                const auto index_begin = read_u16_be(bytes, current_offset + 0U);
                const auto index_end = read_u16_be(bytes, current_offset + 2U);
                const auto next_offset = static_cast< std::size_t >(read_u32_be(bytes, current_offset + 4U));

                if (index_end < index_begin) {
                    return AssetResult< void >(make_error("BRFNT CWDH index range is invalid."));
                }

                const auto width_count = static_cast< std::size_t >(index_end - index_begin + 1U);
                const auto width_table_offset = current_offset + 8U;
                if (not has_bytes(bytes, width_table_offset, width_count * 3U)) {
                    return AssetResult< void >(make_error("BRFNT CWDH width table exceeds file bounds."));
                }

                BrfntFont::WidthRange range{};
                range.begin = index_begin;
                range.end = index_end;
                range.widths.reserve(width_count);
                for (std::size_t i = 0; i < width_count; ++i) {
                    const auto row_offset = width_table_offset + i * 3U;
                    BrfntCharWidths widths{};
                    widths.left = static_cast< std::int8_t >(read_u8(bytes, row_offset + 0U));
                    widths.glyph_width = read_u8(bytes, row_offset + 1U);
                    widths.char_width = static_cast< std::int8_t >(read_u8(bytes, row_offset + 2U));
                    range.widths.push_back(widths);
                }

                ranges->push_back(std::move(range));
                current_offset = next_offset;
            }

            return {};
        }

        [[nodiscard]] AssetResult< void > parse_cmap_chain(std::span< const std::byte > bytes, std::size_t first_map_offset,
                                                           std::vector< BrfntFont::CodeMap >* maps) {
            using namespace binary;

            if (maps == nullptr) {
                return AssetResult< void >(make_error("BRFNT cmap output pointer is null."));
            }

            std::size_t current_offset = first_map_offset;
            while (current_offset != 0U) {
                if (not has_bytes(bytes, current_offset, 12U)) {
                    return AssetResult< void >(make_error("BRFNT CMAP payload header exceeds file bounds."));
                }

                BrfntFont::CodeMap map{};
                map.begin = read_u16_be(bytes, current_offset + 0U);
                map.end = read_u16_be(bytes, current_offset + 2U);
                map.mapping_method = read_u16_be(bytes, current_offset + 4U);
                map.reserved = read_u16_be(bytes, current_offset + 6U);
                const auto next_offset = static_cast< std::size_t >(read_u32_be(bytes, current_offset + 8U));

                if (map.end < map.begin) {
                    return AssetResult< void >(make_error("BRFNT CMAP range is invalid."));
                }

                const auto map_info_offset = current_offset + 12U;
                if (map.mapping_method == 0U) {
                    if (not has_bytes(bytes, map_info_offset, 2U)) {
                        return AssetResult< void >(make_error("BRFNT CMAP direct map is truncated."));
                    }
                    map.map_info.push_back(read_u16_be(bytes, map_info_offset));
                } else if (map.mapping_method == 1U) {
                    const auto count = static_cast< std::size_t >(map.end - map.begin + 1U);
                    if (not has_bytes(bytes, map_info_offset, count * 2U)) {
                        return AssetResult< void >(make_error("BRFNT CMAP table map is truncated."));
                    }
                    map.map_info.reserve(count);
                    for (std::size_t i = 0; i < count; ++i) {
                        map.map_info.push_back(read_u16_be(bytes, map_info_offset + i * 2U));
                    }
                } else if (map.mapping_method == 2U) {
                    if (not has_bytes(bytes, map_info_offset, 2U)) {
                        return AssetResult< void >(make_error("BRFNT CMAP scan map header is truncated."));
                    }
                    const auto pair_count = static_cast< std::size_t >(read_u16_be(bytes, map_info_offset + 0U));
                    if (not has_bytes(bytes, map_info_offset + 2U, pair_count * 4U)) {
                        return AssetResult< void >(make_error("BRFNT CMAP scan map table is truncated."));
                    }

                    map.map_info.reserve(1U + pair_count * 2U);
                    map.map_info.push_back(static_cast< std::uint16_t >(pair_count));
                    for (std::size_t i = 0; i < pair_count; ++i) {
                        const auto pair_offset = map_info_offset + 2U + i * 4U;
                        map.map_info.push_back(read_u16_be(bytes, pair_offset + 0U));
                        map.map_info.push_back(read_u16_be(bytes, pair_offset + 2U));
                    }
                } else {
                    return AssetResult< void >(make_error("BRFNT CMAP uses unsupported mapping method."));
                }

                maps->push_back(std::move(map));
                current_offset = next_offset;
            }

            return {};
        }

        void append_u8(std::vector< std::byte >* bytes, std::uint8_t value) {
            bytes->push_back(static_cast< std::byte >(value));
        }

        void append_u16_be(std::vector< std::byte >* bytes, std::uint16_t value) {
            append_u8(bytes, static_cast< std::uint8_t >((value >> 8U) & 0xFFU));
            append_u8(bytes, static_cast< std::uint8_t >(value & 0xFFU));
        }

        void append_u32_be(std::vector< std::byte >* bytes, std::uint32_t value) {
            append_u8(bytes, static_cast< std::uint8_t >((value >> 24U) & 0xFFU));
            append_u8(bytes, static_cast< std::uint8_t >((value >> 16U) & 0xFFU));
            append_u8(bytes, static_cast< std::uint8_t >((value >> 8U) & 0xFFU));
            append_u8(bytes, static_cast< std::uint8_t >(value & 0xFFU));
        }

        void append_ascii(std::vector< std::byte >* bytes, std::string_view text) {
            for (const auto ch : text) {
                append_u8(bytes, static_cast< std::uint8_t >(ch));
            }
        }

        void append_bytes(std::vector< std::byte >* bytes, std::span< const std::byte > values) {
            bytes->insert(bytes->end(), values.begin(), values.end());
        }

        void append_bytes(std::vector< std::byte >* bytes, const std::vector< std::byte >& values) {
            bytes->insert(bytes->end(), values.begin(), values.end());
        }

        void patch_u32_be(std::vector< std::byte >* bytes, std::size_t offset, std::uint32_t value) {
            (*bytes)[offset + 0U] = static_cast< std::byte >((value >> 24U) & 0xFFU);
            (*bytes)[offset + 1U] = static_cast< std::byte >((value >> 16U) & 0xFFU);
            (*bytes)[offset + 2U] = static_cast< std::byte >((value >> 8U) & 0xFFU);
            (*bytes)[offset + 3U] = static_cast< std::byte >(value & 0xFFU);
        }

        [[nodiscard]] std::size_t align4(std::size_t value) {
            return (value + 3U) & ~static_cast< std::size_t >(3U);
        }

        [[nodiscard]] AssetResult< std::uint32_t > checked_u32(std::size_t value, std::string_view field_name) {
            if (value > static_cast< std::size_t >(std::numeric_limits< std::uint32_t >::max())) {
                return make_error("BRFNT " + std::string(field_name) + " exceeds 32-bit file offset range.");
            }
            return static_cast< std::uint32_t >(value);
        }

        struct LinkedBlockOffsets {
            std::uint32_t payload_offset{};
            std::size_t next_patch_offset{};
        };

        [[nodiscard]] AssetResult< LinkedBlockOffsets > append_cwdh_block(std::vector< std::byte >* out,
                                                                          const BrfntFont::WidthRange& range) {
            if (out == nullptr) {
                return make_error("BRFNT compiler output pointer is null.");
            }

            if (range.end < range.begin) {
                return make_error("BRFNT CWDH range is invalid.");
            }

            const auto width_count = static_cast< std::size_t >(range.end - range.begin + 1U);
            if (range.widths.size() != width_count) {
                return make_error("BRFNT CWDH range width count does not match its index span.");
            }

            const auto block_size = align4(8U + 8U + width_count * 3U);
            const auto block_size_u32 = checked_u32(block_size, "CWDH block size");
            if (not block_size_u32) {
                return block_size_u32.failure();
            }

            const auto block_start = out->size();
            const auto payload_offset = checked_u32(block_start + 8U, "CWDH payload offset");
            if (not payload_offset) {
                return payload_offset.failure();
            }

            append_ascii(out, "CWDH");
            append_u32_be(out, *block_size_u32);
            append_u16_be(out, range.begin);
            append_u16_be(out, range.end);
            const auto next_patch = out->size();
            append_u32_be(out, 0U);
            for (const auto& widths : range.widths) {
                append_u8(out, static_cast< std::uint8_t >(widths.left));
                append_u8(out, widths.glyph_width);
                append_u8(out, static_cast< std::uint8_t >(widths.char_width));
            }

            out->resize(block_start + block_size, static_cast< std::byte >(0U));
            return LinkedBlockOffsets{.payload_offset = *payload_offset, .next_patch_offset = next_patch};
        }

        [[nodiscard]] AssetResult< std::size_t > cmap_payload_value_count(const BrfntFont::CodeMap& map) {
            if (map.end < map.begin) {
                return make_error("BRFNT CMAP range is invalid.");
            }

            if (map.mapping_method == 0U) {
                if (map.map_info.size() != 1U) {
                    return make_error("BRFNT CMAP direct map is missing its glyph base.");
                }
                return 1U;
            }

            if (map.mapping_method == 1U) {
                const auto count = static_cast< std::size_t >(map.end - map.begin + 1U);
                if (map.map_info.size() != count) {
                    return make_error("BRFNT CMAP table value count does not match its codepoint span.");
                }
                return count;
            }

            if (map.mapping_method == 2U) {
                if (map.map_info.empty()) {
                    return make_error("BRFNT CMAP scan map is missing its pair count.");
                }
                const auto pair_count = static_cast< std::size_t >(map.map_info[0U]);
                const auto value_count = 1U + pair_count * 2U;
                if (map.map_info.size() != value_count) {
                    return make_error("BRFNT CMAP scan map pair table is truncated.");
                }
                return value_count;
            }

            return make_error("BRFNT CMAP uses unsupported mapping method.");
        }

        [[nodiscard]] AssetResult< LinkedBlockOffsets > append_cmap_block(std::vector< std::byte >* out,
                                                                          const BrfntFont::CodeMap& map) {
            if (out == nullptr) {
                return make_error("BRFNT compiler output pointer is null.");
            }

            const auto value_count = cmap_payload_value_count(map);
            if (not value_count) {
                return value_count.failure();
            }

            const auto block_size = align4(8U + 12U + (*value_count * 2U));
            const auto block_size_u32 = checked_u32(block_size, "CMAP block size");
            if (not block_size_u32) {
                return block_size_u32.failure();
            }

            const auto block_start = out->size();
            const auto payload_offset = checked_u32(block_start + 8U, "CMAP payload offset");
            if (not payload_offset) {
                return payload_offset.failure();
            }

            append_ascii(out, "CMAP");
            append_u32_be(out, *block_size_u32);
            append_u16_be(out, map.begin);
            append_u16_be(out, map.end);
            append_u16_be(out, map.mapping_method);
            append_u16_be(out, map.reserved);
            const auto next_patch = out->size();
            append_u32_be(out, 0U);

            for (std::size_t i = 0; i < *value_count; ++i) {
                append_u16_be(out, map.map_info[i]);
            }

            out->resize(block_start + block_size, static_cast< std::byte >(0U));
            return LinkedBlockOffsets{.payload_offset = *payload_offset, .next_patch_offset = next_patch};
        }

    }  // namespace

    const std::string& BrfntFont::name() const {
        return _name;
    }

    std::uint8_t BrfntFont::line_feed() const {
        return _line_feed;
    }

    std::uint8_t BrfntFont::ascent() const {
        return _ascent;
    }

    std::uint8_t BrfntFont::baseline_position() const {
        return _baseline_position;
    }

    std::uint8_t BrfntFont::font_width() const {
        return _font_width;
    }

    std::uint8_t BrfntFont::font_height() const {
        return _font_height;
    }

    std::uint8_t BrfntFont::cell_width() const {
        return _cell_width;
    }

    std::uint8_t BrfntFont::cell_height() const {
        return _cell_height;
    }

    const std::vector< tpl::DecodedImage >& BrfntFont::sheets() const {
        return _sheets;
    }

    std::uint16_t BrfntFont::sheet_format() const {
        return _sheet_format;
    }

    bool BrfntFont::has_codepoint(std::uint16_t codepoint) const {
        return find_glyph_index(codepoint) != 0xFFFFU;
    }

    std::uint16_t BrfntFont::find_glyph_index(std::uint16_t codepoint) const {
        for (const auto& map : _code_maps) {
            if (codepoint < map.begin || codepoint > map.end) {
                continue;
            }

            if (map.mapping_method == 0U) {
                if (map.map_info.empty()) {
                    return 0xFFFFU;
                }
                const auto base = map.map_info[0U];
                return static_cast< std::uint16_t >(base + (codepoint - map.begin));
            }

            if (map.mapping_method == 1U) {
                const auto idx = static_cast< std::size_t >(codepoint - map.begin);
                if (idx >= map.map_info.size()) {
                    return 0xFFFFU;
                }
                return map.map_info[idx];
            }

            if (map.mapping_method == 2U) {
                if (map.map_info.empty()) {
                    return 0xFFFFU;
                }

                const auto pair_count = static_cast< std::size_t >(map.map_info[0U]);
                std::size_t begin = 0U;
                std::size_t end = pair_count;
                while (begin < end) {
                    const auto middle = begin + (end - begin) / 2U;
                    const auto key_index = 1U + middle * 2U;
                    const auto pair_codepoint = map.map_info[key_index + 0U];
                    const auto pair_glyph = map.map_info[key_index + 1U];
                    if (pair_codepoint < codepoint) {
                        begin = middle + 1U;
                    } else if (codepoint < pair_codepoint) {
                        end = middle;
                    } else {
                        return pair_glyph;
                    }
                }

                return 0xFFFFU;
            }
        }

        return 0xFFFFU;
    }

    std::uint16_t BrfntFont::map_codepoint_to_glyph(std::uint16_t codepoint) const {
        const auto found = find_glyph_index(codepoint);
        if (found != 0xFFFFU) {
            return found;
        }
        return _alter_char_index;
    }

    BrfntCharWidths BrfntFont::width_for_index(std::uint16_t index) const {
        for (const auto& range : _width_ranges) {
            if (index < range.begin || index > range.end) {
                continue;
            }

            const auto offset = static_cast< std::size_t >(index - range.begin);
            if (offset < range.widths.size()) {
                return range.widths[offset];
            }
            break;
        }

        return _default_width;
    }

    bool BrfntFont::get_glyph(std::uint16_t codepoint, BrfntGlyph* out) const {
        if (out == nullptr) {
            return false;
        }

        if (_sheet_row == 0U || _sheet_line == 0U) {
            return false;
        }

        const auto glyph_index = map_codepoint_to_glyph(codepoint);
        const auto cells_in_sheet = static_cast< std::uint32_t >(_sheet_row) * static_cast< std::uint32_t >(_sheet_line);
        if (cells_in_sheet == 0U) {
            return false;
        }

        const auto glyph_cell = static_cast< std::uint32_t >(glyph_index) % cells_in_sheet;
        const auto glyph_sheet = static_cast< std::uint32_t >(glyph_index) / cells_in_sheet;
        if (glyph_sheet >= _sheets.size()) {
            return false;
        }

        const auto unit_x = glyph_cell % _sheet_row;
        const auto unit_y = glyph_cell / _sheet_row;

        out->glyph_index = glyph_index;
        out->sheet_index = static_cast< std::uint16_t >(glyph_sheet);
        out->cell_x = static_cast< std::uint16_t >(unit_x * (static_cast< std::uint32_t >(_cell_width) + 1U) + 1U);
        out->cell_y = static_cast< std::uint16_t >(unit_y * (static_cast< std::uint32_t >(_cell_height) + 1U) + 1U);
        out->widths = width_for_index(glyph_index);

        return true;
    }

    AssetResult< BrfntFont > parse_brfnt(std::span< const std::byte > bytes, std::string name_hint) {
        using namespace binary;

        if (bytes.size() < 0x10U) {
            return make_error("BRFNT file is too small.");
        }
        if (not fourcc_equals(bytes, 0U, "RFNT")) {
            return make_error("BRFNT signature mismatch.");
        }

        const auto bom = read_u16_be(bytes, 0x04U);
        const auto version = read_u16_be(bytes, 0x06U);
        const auto header_size = static_cast< std::size_t >(read_u16_be(bytes, 0x0CU));
        const auto block_count = static_cast< std::size_t >(read_u16_be(bytes, 0x0EU));
        if (header_size < 0x10U || header_size > bytes.size()) {
            return make_error("BRFNT header size is invalid.");
        }

        std::size_t finf_offset = 0U;
        std::size_t tglp_offset = 0U;

        std::size_t block_offset = header_size;
        for (std::size_t block_index = 0; block_index < block_count; ++block_index) {
            if (not has_bytes(bytes, block_offset, 8U)) {
                return make_error("BRFNT block header exceeds file bounds.");
            }

            const auto block_size = static_cast< std::size_t >(read_u32_be(bytes, block_offset + 4U));
            if (block_size < 8U || not has_bytes(bytes, block_offset, block_size)) {
                return make_error("BRFNT block size is invalid.");
            }

            if (fourcc_equals(bytes, block_offset, "FINF")) {
                finf_offset = block_offset + 8U;
            } else if (fourcc_equals(bytes, block_offset, "TGLP")) {
                tglp_offset = block_offset + 8U;
            }

            block_offset += block_size;
        }

        if (finf_offset == 0U || tglp_offset == 0U) {
            return make_error("BRFNT is missing required FINF or TGLP block.");
        }

        const auto finf_result = parse_finf(bytes, finf_offset);
        if (not finf_result) {
            return finf_result.failure();
        }
        const auto tglp_result = parse_tglp(bytes, tglp_offset);
        if (not tglp_result) {
            return tglp_result.failure();
        }

        const auto& finf = *finf_result;
        const auto& tglp = *tglp_result;

        BrfntFont font{};
        font._name = std::move(name_hint);
        font._bom = bom;
        font._version = version;
        if (header_size > 0x10U) {
            const auto padding = subspan(bytes, 0x10U, header_size - 0x10U);
            font._header_padding.assign(padding.begin(), padding.end());
        }
        font._font_type = finf.font_type;
        font._alter_char_index = finf.alter_char_index;
        font._default_width = finf.default_width;
        font._encoding = finf.encoding;
        font._finf_reserved = finf.reserved;
        font._line_feed = finf.line_feed;
        font._ascent = finf.ascent;
        font._font_width = finf.width;
        font._font_height = finf.height;

        font._cell_width = tglp.cell_width;
        font._cell_height = tglp.cell_height;
        font._baseline_position = tglp.baseline_pos;
        font._max_char_width = tglp.max_char_width;

        font._sheet_num = tglp.sheet_num;
        font._sheet_size = tglp.sheet_size;
        font._sheet_format = tglp.sheet_format;
        font._sheet_row = tglp.sheet_row;
        font._sheet_line = tglp.sheet_line;
        font._sheet_width = tglp.sheet_width;
        font._sheet_height = tglp.sheet_height;
        font._tglp_reserved = tglp.reserved;

        if (font._sheet_num == 0U || tglp.sheet_size == 0U) {
            return make_error("BRFNT sheet metadata is invalid.");
        }

        const auto tglp_sheet_header_end = tglp_offset + 28U;
        if (tglp.sheet_image_offset < tglp_sheet_header_end) {
            return make_error("BRFNT sheet image offset points into the TGLP header.");
        }
        if (not has_bytes(bytes, tglp_sheet_header_end, tglp.sheet_image_offset - tglp_sheet_header_end)) {
            return make_error("BRFNT TGLP sheet padding exceeds file bounds.");
        }
        const auto sheet_padding = subspan(bytes, tglp_sheet_header_end, tglp.sheet_image_offset - tglp_sheet_header_end);
        font._tglp_sheet_padding.assign(sheet_padding.begin(), sheet_padding.end());

        const auto total_sheet_bytes = static_cast< std::size_t >(font._sheet_num) * static_cast< std::size_t >(tglp.sheet_size);
        if (not has_bytes(bytes, tglp.sheet_image_offset, total_sheet_bytes)) {
            return make_error("BRFNT sheet image table exceeds file bounds.");
        }
        const auto raw_sheet_bytes = subspan(bytes, tglp.sheet_image_offset, total_sheet_bytes);
        font._sheet_bytes.assign(raw_sheet_bytes.begin(), raw_sheet_bytes.end());

        font._sheets.reserve(font._sheet_num);
        const auto sheet_data = std::span< const std::byte >(font._sheet_bytes.data(), font._sheet_bytes.size());
        for (std::size_t sheet_index = 0; sheet_index < font._sheet_num; ++sheet_index) {
            const auto sheet_offset = sheet_index * static_cast< std::size_t >(tglp.sheet_size);
            const auto sheet_bytes = subspan(sheet_data, sheet_offset, static_cast< std::size_t >(tglp.sheet_size));
            const auto decoded = tpl::decode_gx_tiled_texture(sheet_bytes, font._sheet_width, font._sheet_height, font._sheet_format);
            if (not decoded) {
                return decoded.failure();
            }

            auto decoded_sheet = *decoded;
            if (font._sheet_format <= 3U) {
                normalize_font_sheet_to_alpha_mask(&decoded_sheet);
            }
            font._sheets.push_back(std::move(decoded_sheet));
        }

        const auto width_chain_result = parse_width_chain(bytes, finf.width_offset, &font._width_ranges);
        if (not width_chain_result) {
            return width_chain_result.failure();
        }

        const auto cmap_chain_result = parse_cmap_chain(bytes, finf.map_offset, &font._code_maps);
        if (not cmap_chain_result) {
            return cmap_chain_result.failure();
        }

        return font;
    }

    AssetResult< std::vector< std::byte > > compile_brfnt(const BrfntFont& font) {
        if (font._sheet_num == 0U || font._sheet_size == 0U) {
            return make_error("BRFNT compiler requires sheet metadata.");
        }

        const auto expected_sheet_bytes = static_cast< std::size_t >(font._sheet_num) * static_cast< std::size_t >(font._sheet_size);
        if (font._sheet_bytes.size() != expected_sheet_bytes) {
            return make_error("BRFNT compiler requires original GX sheet bytes matching the sheet metadata.");
        }

        const auto block_count = 2U + font._width_ranges.size() + font._code_maps.size();
        if (block_count > static_cast< std::size_t >(std::numeric_limits< std::uint16_t >::max())) {
            return make_error("BRFNT compiler block count exceeds the RFNT header range.");
        }

        const auto header_size = 0x10U + font._header_padding.size();
        if (header_size > static_cast< std::size_t >(std::numeric_limits< std::uint16_t >::max())) {
            return make_error("BRFNT compiler header size exceeds the RFNT header range.");
        }

        std::vector< std::byte > bytes{};
        bytes.reserve(0x10U + 0x20U + 0x30U + font._sheet_bytes.size() + font._width_ranges.size() * 0x40U +
                      font._code_maps.size() * 0x40U);

        append_ascii(&bytes, "RFNT");
        append_u16_be(&bytes, font._bom);
        append_u16_be(&bytes, font._version);
        const auto file_size_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        append_u16_be(&bytes, static_cast< std::uint16_t >(header_size));
        append_u16_be(&bytes, static_cast< std::uint16_t >(block_count));
        append_bytes(&bytes, font._header_padding);

        append_ascii(&bytes, "FINF");
        append_u32_be(&bytes, 0x20U);
        append_u8(&bytes, font._font_type);
        append_u8(&bytes, font._line_feed);
        append_u16_be(&bytes, font._alter_char_index);
        append_u8(&bytes, static_cast< std::uint8_t >(font._default_width.left));
        append_u8(&bytes, font._default_width.glyph_width);
        append_u8(&bytes, static_cast< std::uint8_t >(font._default_width.char_width));
        append_u8(&bytes, font._encoding);
        const auto tglp_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        const auto cwdh_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        const auto cmap_offset_patch = bytes.size();
        append_u32_be(&bytes, 0U);
        append_u8(&bytes, font._font_height);
        append_u8(&bytes, font._font_width);
        append_u8(&bytes, font._ascent);
        append_u8(&bytes, font._finf_reserved);

        const auto tglp_block_start = bytes.size();
        const auto tglp_payload_offset = checked_u32(tglp_block_start + 8U, "TGLP payload offset");
        if (not tglp_payload_offset) {
            return tglp_payload_offset.failure();
        }

        const auto tglp_block_size = 8U + 28U + font._tglp_sheet_padding.size() + font._sheet_bytes.size();
        const auto tglp_block_size_u32 = checked_u32(tglp_block_size, "TGLP block size");
        if (not tglp_block_size_u32) {
            return tglp_block_size_u32.failure();
        }
        const auto sheet_image_offset = checked_u32(tglp_block_start + 8U + 28U + font._tglp_sheet_padding.size(), "TGLP sheet image offset");
        if (not sheet_image_offset) {
            return sheet_image_offset.failure();
        }

        append_ascii(&bytes, "TGLP");
        append_u32_be(&bytes, *tglp_block_size_u32);
        append_u8(&bytes, font._cell_width);
        append_u8(&bytes, font._cell_height);
        append_u8(&bytes, font._baseline_position);
        append_u8(&bytes, font._max_char_width);
        append_u32_be(&bytes, font._sheet_size);
        append_u16_be(&bytes, font._sheet_num);
        append_u16_be(&bytes, font._sheet_format);
        append_u16_be(&bytes, font._sheet_row);
        append_u16_be(&bytes, font._sheet_line);
        append_u16_be(&bytes, font._sheet_width);
        append_u16_be(&bytes, font._sheet_height);
        append_u32_be(&bytes, *sheet_image_offset);
        append_u32_be(&bytes, font._tglp_reserved);
        append_bytes(&bytes, font._tglp_sheet_padding);
        append_bytes(&bytes, font._sheet_bytes);

        std::vector< LinkedBlockOffsets > width_offsets{};
        width_offsets.reserve(font._width_ranges.size());
        for (const auto& range : font._width_ranges) {
            auto appended = append_cwdh_block(&bytes, range);
            if (not appended) {
                return appended.failure();
            }
            width_offsets.push_back(*appended);
        }
        for (std::size_t i = 0; i < width_offsets.size(); ++i) {
            const auto next = i + 1U < width_offsets.size() ? width_offsets[i + 1U].payload_offset : 0U;
            patch_u32_be(&bytes, width_offsets[i].next_patch_offset, next);
        }

        std::vector< LinkedBlockOffsets > cmap_offsets{};
        cmap_offsets.reserve(font._code_maps.size());
        for (const auto& map : font._code_maps) {
            auto appended = append_cmap_block(&bytes, map);
            if (not appended) {
                return appended.failure();
            }
            cmap_offsets.push_back(*appended);
        }
        for (std::size_t i = 0; i < cmap_offsets.size(); ++i) {
            const auto next = i + 1U < cmap_offsets.size() ? cmap_offsets[i + 1U].payload_offset : 0U;
            patch_u32_be(&bytes, cmap_offsets[i].next_patch_offset, next);
        }

        patch_u32_be(&bytes, tglp_offset_patch, *tglp_payload_offset);
        patch_u32_be(&bytes, cwdh_offset_patch, width_offsets.empty() ? 0U : width_offsets.front().payload_offset);
        patch_u32_be(&bytes, cmap_offset_patch, cmap_offsets.empty() ? 0U : cmap_offsets.front().payload_offset);

        const auto file_size = checked_u32(bytes.size(), "file size");
        if (not file_size) {
            return file_size.failure();
        }
        patch_u32_be(&bytes, file_size_patch, *file_size);

        return bytes;
    }

}  // namespace smgpc::assets::layout
