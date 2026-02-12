#include "Brfnt.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

#include "Binary.hpp"

namespace smgpc::assets::layout {
namespace {

struct FinfInfo {
    std::uint16_t alter_char_index {};
    BrfntCharWidths default_width {};
    std::uint8_t line_feed {};
    std::uint8_t height {};
    std::uint8_t width {};
    std::uint8_t ascent {};

    std::size_t glyph_offset {};
    std::size_t width_offset {};
    std::size_t map_offset {};
};

struct TglpInfo {
    std::uint8_t cell_width {};
    std::uint8_t cell_height {};
    std::uint8_t baseline_pos {};
    std::uint8_t max_char_width {};

    std::uint32_t sheet_size {};
    std::uint16_t sheet_num {};
    std::uint16_t sheet_format {};
    std::uint16_t sheet_row {};
    std::uint16_t sheet_line {};
    std::uint16_t sheet_width {};
    std::uint16_t sheet_height {};
    std::size_t sheet_image_offset {};
};

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

[[nodiscard]] AssetResult<FinfInfo> parse_finf(std::span<const std::byte> bytes, std::size_t finf_data_offset) {
    using namespace binary;

    if (not has_bytes(bytes, finf_data_offset, 24U)) {
        return make_error("BRFNT FINF payload is too small.");
    }

    FinfInfo finf {};
    finf.line_feed = read_u8(bytes, finf_data_offset + 1U);
    finf.alter_char_index = read_u16_be(bytes, finf_data_offset + 2U);
    finf.default_width.left = static_cast<std::int8_t>(read_u8(bytes, finf_data_offset + 4U));
    finf.default_width.glyph_width = read_u8(bytes, finf_data_offset + 5U);
    finf.default_width.char_width = static_cast<std::int8_t>(read_u8(bytes, finf_data_offset + 6U));

    finf.glyph_offset = static_cast<std::size_t>(read_u32_be(bytes, finf_data_offset + 8U));
    finf.width_offset = static_cast<std::size_t>(read_u32_be(bytes, finf_data_offset + 12U));
    finf.map_offset = static_cast<std::size_t>(read_u32_be(bytes, finf_data_offset + 16U));

    finf.height = read_u8(bytes, finf_data_offset + 20U);
    finf.width = read_u8(bytes, finf_data_offset + 21U);
    finf.ascent = read_u8(bytes, finf_data_offset + 22U);

    return finf;
}

[[nodiscard]] AssetResult<TglpInfo> parse_tglp(std::span<const std::byte> bytes, std::size_t tglp_data_offset) {
    using namespace binary;

    if (not has_bytes(bytes, tglp_data_offset, 28U)) {
        return make_error("BRFNT TGLP payload is too small.");
    }

    TglpInfo tglp {};
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
    tglp.sheet_image_offset = static_cast<std::size_t>(read_u32_be(bytes, tglp_data_offset + 20U));

    return tglp;
}

[[nodiscard]] AssetResult<void> parse_width_chain(
    std::span<const std::byte> bytes,
    std::size_t first_width_offset,
    std::vector<BrfntFont::WidthRange> *ranges) {
    using namespace binary;

    if (ranges == nullptr) {
        return AssetResult<void>(make_error("BRFNT width output pointer is null."));
    }

    std::size_t current_offset = first_width_offset;
    while (current_offset != 0U) {
        if (not has_bytes(bytes, current_offset, 8U)) {
            return AssetResult<void>(make_error("BRFNT CWDH payload header exceeds file bounds."));
        }

        const auto index_begin = read_u16_be(bytes, current_offset + 0U);
        const auto index_end = read_u16_be(bytes, current_offset + 2U);
        const auto next_offset = static_cast<std::size_t>(read_u32_be(bytes, current_offset + 4U));

        if (index_end < index_begin) {
            return AssetResult<void>(make_error("BRFNT CWDH index range is invalid."));
        }

        const auto width_count = static_cast<std::size_t>(index_end - index_begin + 1U);
        const auto width_table_offset = current_offset + 8U;
        if (not has_bytes(bytes, width_table_offset, width_count * 3U)) {
            return AssetResult<void>(make_error("BRFNT CWDH width table exceeds file bounds."));
        }

        BrfntFont::WidthRange range {};
        range.begin = index_begin;
        range.end = index_end;
        range.widths.reserve(width_count);
        for (std::size_t i = 0; i < width_count; ++i) {
            const auto row_offset = width_table_offset + i * 3U;
            BrfntCharWidths widths {};
            widths.left = static_cast<std::int8_t>(read_u8(bytes, row_offset + 0U));
            widths.glyph_width = read_u8(bytes, row_offset + 1U);
            widths.char_width = static_cast<std::int8_t>(read_u8(bytes, row_offset + 2U));
            range.widths.push_back(widths);
        }

        ranges->push_back(std::move(range));
        current_offset = next_offset;
    }

    return {};
}

[[nodiscard]] AssetResult<void> parse_cmap_chain(
    std::span<const std::byte> bytes,
    std::size_t first_map_offset,
    std::vector<BrfntFont::CodeMap> *maps) {
    using namespace binary;

    if (maps == nullptr) {
        return AssetResult<void>(make_error("BRFNT cmap output pointer is null."));
    }

    std::size_t current_offset = first_map_offset;
    while (current_offset != 0U) {
        if (not has_bytes(bytes, current_offset, 12U)) {
            return AssetResult<void>(make_error("BRFNT CMAP payload header exceeds file bounds."));
        }

        BrfntFont::CodeMap map {};
        map.begin = read_u16_be(bytes, current_offset + 0U);
        map.end = read_u16_be(bytes, current_offset + 2U);
        map.mapping_method = read_u16_be(bytes, current_offset + 4U);
        const auto next_offset = static_cast<std::size_t>(read_u32_be(bytes, current_offset + 8U));

        if (map.end < map.begin) {
            return AssetResult<void>(make_error("BRFNT CMAP range is invalid."));
        }

        const auto map_info_offset = current_offset + 12U;
        if (map.mapping_method == 0U) {
            if (not has_bytes(bytes, map_info_offset, 2U)) {
                return AssetResult<void>(make_error("BRFNT CMAP direct map is truncated."));
            }
            map.map_info.push_back(read_u16_be(bytes, map_info_offset));
        } else if (map.mapping_method == 1U) {
            const auto count = static_cast<std::size_t>(map.end - map.begin + 1U);
            if (not has_bytes(bytes, map_info_offset, count * 2U)) {
                return AssetResult<void>(make_error("BRFNT CMAP table map is truncated."));
            }
            map.map_info.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                map.map_info.push_back(read_u16_be(bytes, map_info_offset + i * 2U));
            }
        } else if (map.mapping_method == 2U) {
            if (not has_bytes(bytes, map_info_offset, 2U)) {
                return AssetResult<void>(make_error("BRFNT CMAP scan map header is truncated."));
            }
            const auto pair_count = static_cast<std::size_t>(read_u16_be(bytes, map_info_offset + 0U));
            if (not has_bytes(bytes, map_info_offset + 2U, pair_count * 4U)) {
                return AssetResult<void>(make_error("BRFNT CMAP scan map table is truncated."));
            }

            map.map_info.reserve(1U + pair_count * 2U);
            map.map_info.push_back(static_cast<std::uint16_t>(pair_count));
            for (std::size_t i = 0; i < pair_count; ++i) {
                const auto pair_offset = map_info_offset + 2U + i * 4U;
                map.map_info.push_back(read_u16_be(bytes, pair_offset + 0U));
                map.map_info.push_back(read_u16_be(bytes, pair_offset + 2U));
            }
        } else {
            return AssetResult<void>(make_error("BRFNT CMAP uses unsupported mapping method."));
        }

        maps->push_back(std::move(map));
        current_offset = next_offset;
    }

    return {};
}

}  // namespace

const std::string &BrfntFont::name() const {
    return _name;
}

std::uint8_t BrfntFont::line_feed() const {
    return _line_feed;
}

std::uint8_t BrfntFont::ascent() const {
    return _ascent;
}

std::uint8_t BrfntFont::cell_width() const {
    return _cell_width;
}

std::uint8_t BrfntFont::cell_height() const {
    return _cell_height;
}

const std::vector<tpl::DecodedImage> &BrfntFont::sheets() const {
    return _sheets;
}

bool BrfntFont::has_codepoint(std::uint16_t codepoint) const {
    return find_glyph_index(codepoint) != 0xFFFFU;
}

std::uint16_t BrfntFont::find_glyph_index(std::uint16_t codepoint) const {
    for (const auto &map : _code_maps) {
        if (codepoint < map.begin || codepoint > map.end) {
            continue;
        }

        if (map.mapping_method == 0U) {
            if (map.map_info.empty()) {
                return 0xFFFFU;
            }
            const auto base = map.map_info[0U];
            return static_cast<std::uint16_t>(base + (codepoint - map.begin));
        }

        if (map.mapping_method == 1U) {
            const auto idx = static_cast<std::size_t>(codepoint - map.begin);
            if (idx >= map.map_info.size()) {
                return 0xFFFFU;
            }
            return map.map_info[idx];
        }

        if (map.mapping_method == 2U) {
            if (map.map_info.empty()) {
                return 0xFFFFU;
            }

            const auto pair_count = static_cast<std::size_t>(map.map_info[0U]);
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
    for (const auto &range : _width_ranges) {
        if (index < range.begin || index > range.end) {
            continue;
        }

        const auto offset = static_cast<std::size_t>(index - range.begin);
        if (offset < range.widths.size()) {
            return range.widths[offset];
        }
        break;
    }

    return _default_width;
}

bool BrfntFont::get_glyph(std::uint16_t codepoint, BrfntGlyph *out) const {
    if (out == nullptr) {
        return false;
    }

    if (_sheet_row == 0U || _sheet_line == 0U) {
        return false;
    }

    const auto glyph_index = map_codepoint_to_glyph(codepoint);
    const auto cells_in_sheet = static_cast<std::uint32_t>(_sheet_row) * static_cast<std::uint32_t>(_sheet_line);
    if (cells_in_sheet == 0U) {
        return false;
    }

    const auto glyph_cell = static_cast<std::uint32_t>(glyph_index) % cells_in_sheet;
    const auto glyph_sheet = static_cast<std::uint32_t>(glyph_index) / cells_in_sheet;
    if (glyph_sheet >= _sheets.size()) {
        return false;
    }

    const auto unit_x = glyph_cell % _sheet_row;
    const auto unit_y = glyph_cell / _sheet_row;

    out->glyph_index = glyph_index;
    out->sheet_index = static_cast<std::uint16_t>(glyph_sheet);
    out->cell_x = static_cast<std::uint16_t>(unit_x * (static_cast<std::uint32_t>(_cell_width) + 1U) + 1U);
    out->cell_y = static_cast<std::uint16_t>(unit_y * (static_cast<std::uint32_t>(_cell_height) + 1U) + 1U);
    out->widths = width_for_index(glyph_index);

    return true;
}

AssetResult<BrfntFont> parse_brfnt(std::span<const std::byte> bytes, std::string name_hint) {
    using namespace binary;

    if (bytes.size() < 0x10U) {
        return make_error("BRFNT file is too small.");
    }
    if (not fourcc_equals(bytes, 0U, "RFNT")) {
        return make_error("BRFNT signature mismatch.");
    }

    const auto header_size = static_cast<std::size_t>(read_u16_be(bytes, 0x0CU));
    const auto block_count = static_cast<std::size_t>(read_u16_be(bytes, 0x0EU));
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

        const auto block_size = static_cast<std::size_t>(read_u32_be(bytes, block_offset + 4U));
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

    const auto &finf = *finf_result;
    const auto &tglp = *tglp_result;

    BrfntFont font {};
    font._name = std::move(name_hint);
    font._alter_char_index = finf.alter_char_index;
    font._default_width = finf.default_width;
    font._line_feed = finf.line_feed;
    font._ascent = finf.ascent;

    font._cell_width = tglp.cell_width;
    font._cell_height = tglp.cell_height;
    font._max_char_width = tglp.max_char_width;

    font._sheet_num = tglp.sheet_num;
    font._sheet_format = tglp.sheet_format;
    font._sheet_row = tglp.sheet_row;
    font._sheet_line = tglp.sheet_line;
    font._sheet_width = tglp.sheet_width;
    font._sheet_height = tglp.sheet_height;

    if (font._sheet_num == 0U || tglp.sheet_size == 0U) {
        return make_error("BRFNT sheet metadata is invalid.");
    }

    const auto total_sheet_bytes = static_cast<std::size_t>(font._sheet_num) * static_cast<std::size_t>(tglp.sheet_size);
    if (not has_bytes(bytes, tglp.sheet_image_offset, total_sheet_bytes)) {
        return make_error("BRFNT sheet image table exceeds file bounds.");
    }

    font._sheets.reserve(font._sheet_num);
    for (std::size_t sheet_index = 0; sheet_index < font._sheet_num; ++sheet_index) {
        const auto sheet_offset = tglp.sheet_image_offset + sheet_index * static_cast<std::size_t>(tglp.sheet_size);
        const auto sheet_bytes = subspan(bytes, sheet_offset, static_cast<std::size_t>(tglp.sheet_size));
        const auto decoded = tpl::decode_gx_tiled_texture(
            sheet_bytes,
            font._sheet_width,
            font._sheet_height,
            font._sheet_format);
        if (not decoded) {
            return decoded.failure();
        }

        font._sheets.push_back(*decoded);
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

}  // namespace smgpc::assets::layout
