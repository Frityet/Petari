#include "Bmg.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "Binary.hpp"

namespace smgpc::assets::layout {
namespace {

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

[[nodiscard]] std::string read_string(std::span<const std::byte> bytes, std::size_t offset) {
    return binary::read_c_string(bytes, offset);
}

[[nodiscard]] std::u16string read_utf16be_message(std::span<const std::byte> bytes, std::size_t offset) {
    std::u16string text {};
    for (std::size_t cursor = offset; cursor + 1U < bytes.size();) {
        const auto code_unit = static_cast<char16_t>(binary::read_u16_be(bytes, cursor));
        if (code_unit == 0U) {
            break;
        }
        if (code_unit == 0x001AU) {
            if (cursor + 3U >= bytes.size()) {
                break;
            }

            const auto command_header = binary::read_u16_be(bytes, cursor + 2U);
            const auto command_size = static_cast<std::size_t>(command_header >> 8U);
            if (command_size < 4U || cursor + command_size > bytes.size()) {
                break;
            }

            const auto command_type = command_header & 0xFFU;
            if (command_type == 0x03U && command_size >= 6U) {
                const auto payload = binary::read_u16_be(bytes, cursor + 4U);
                if (payload <= BMG_PICTURE_FONT_TAG_MAX_PAYLOAD) {
                    text.push_back(make_bmg_picture_font_tag(payload));
                }
            } else if (command_type == 0x06U && command_size >= 5U) {
                const auto argument_index = binary::read_u8(bytes, cursor + command_size - 1U);
                text.push_back(u'{');
                if (argument_index >= 10U) {
                    text.push_back(static_cast<char16_t>(u'0' + (argument_index / 10U)));
                }
                text.push_back(static_cast<char16_t>(u'0' + (argument_index % 10U)));
                text.push_back(u'}');
            }

            cursor += command_size;
            continue;
        }

        text.push_back(code_unit);
        cursor += 2U;
    }
    return text;
}

[[nodiscard]] AssetResult<std::unordered_map<std::string, std::uint32_t>> parse_message_id_table(std::span<const std::byte> bytes) {
    if (not binary::has_bytes(bytes, 0U, 0x10U)) {
        return make_error("MessageId table is too small.");
    }

    const auto entry_count = static_cast<std::size_t>(binary::read_u32_be(bytes, 0U));
    const auto field_count = static_cast<std::size_t>(binary::read_u32_be(bytes, 4U));
    const auto data_offset = static_cast<std::size_t>(binary::read_u32_be(bytes, 8U));
    const auto entry_size = static_cast<std::size_t>(binary::read_u32_be(bytes, 0x0CU));

    if (field_count < 2U || entry_size < 8U) {
        return make_error("MessageId table schema is unsupported.");
    }
    if (not binary::has_bytes(bytes, data_offset, entry_count * entry_size)) {
        return make_error("MessageId table entries exceed bounds.");
    }

    const auto string_table_offset = data_offset + entry_count * entry_size;
    if (string_table_offset > bytes.size()) {
        return make_error("MessageId string table exceeds bounds.");
    }

    std::unordered_map<std::string, std::uint32_t> index_by_id {};
    index_by_id.reserve(entry_count);
    for (std::size_t i = 0; i < entry_count; ++i) {
        const auto entry_offset = data_offset + i * entry_size;
        const auto message_index = binary::read_u32_be(bytes, entry_offset + 0U);
        const auto string_offset = static_cast<std::size_t>(binary::read_u32_be(bytes, entry_offset + 4U));
        if (string_table_offset + string_offset >= bytes.size()) {
            continue;
        }
        index_by_id.emplace(read_string(bytes, string_table_offset + string_offset), message_index);
    }

    return index_by_id;
}

}  // namespace

AssetResult<BmgMessageMap> parse_bmg_messages(std::span<const std::byte> bmg_bytes, std::span<const std::byte> message_id_table_bytes) {
    if (not binary::has_bytes(bmg_bytes, 0U, 0x20U) || not binary::fourcc_equals(bmg_bytes, 0U, "MESG")) {
        return make_error("BMG header is invalid.");
    }

    const auto id_table = parse_message_id_table(message_id_table_bytes);
    if (not id_table) {
        return id_table.failure();
    }

    std::size_t inf_offset = 0U;
    std::size_t dat_offset = 0U;
    for (std::size_t offset = 0x20U; offset + 8U <= bmg_bytes.size();) {
        const auto block_size = static_cast<std::size_t>(binary::read_u32_be(bmg_bytes, offset + 4U));
        if (block_size < 8U || not binary::has_bytes(bmg_bytes, offset, block_size)) {
            return make_error("BMG block size is invalid.");
        }

        if (binary::fourcc_equals(bmg_bytes, offset, "INF1")) {
            inf_offset = offset;
        } else if (binary::fourcc_equals(bmg_bytes, offset, "DAT1")) {
            dat_offset = offset;
        }

        if (inf_offset != 0U && dat_offset != 0U) {
            break;
        }

        offset += block_size;
    }

    if (inf_offset == 0U || dat_offset == 0U) {
        return make_error("BMG is missing INF1 or DAT1.");
    }

    const auto item_count = static_cast<std::size_t>(binary::read_u16_be(bmg_bytes, inf_offset + 8U));
    const auto item_size = static_cast<std::size_t>(binary::read_u16_be(bmg_bytes, inf_offset + 0x0AU));
    if (item_size < 4U || not binary::has_bytes(bmg_bytes, inf_offset + 0x10U, item_count * item_size)) {
        return make_error("BMG INF1 entries exceed bounds.");
    }

    BmgMessageMap messages {};
    messages.reserve(id_table->size());
    for (const auto &[message_id, index] : *id_table) {
        if (index >= item_count) {
            continue;
        }

        const auto item_offset = inf_offset + 0x10U + static_cast<std::size_t>(index) * item_size;
        const auto message_offset = static_cast<std::size_t>(binary::read_u32_be(bmg_bytes, item_offset));
        const auto text_offset = dat_offset + 8U + message_offset;
        if (text_offset >= bmg_bytes.size()) {
            continue;
        }

        messages.emplace(message_id, read_utf16be_message(bmg_bytes, text_offset));
    }

    return messages;
}

}  // namespace smgpc::assets::layout
