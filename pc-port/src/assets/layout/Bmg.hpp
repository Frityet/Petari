#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "AssetServices.hpp"

namespace smgpc::assets::layout {

using BmgMessageMap = std::unordered_map<std::string, std::u16string>;

inline constexpr char16_t BMG_PICTURE_FONT_TAG_BASE = static_cast<char16_t>(0xF000U);
inline constexpr std::uint16_t BMG_PICTURE_FONT_TAG_MAX_PAYLOAD = 0x0FFFU;

[[nodiscard]] constexpr bool is_bmg_picture_font_tag(char16_t codepoint) {
    return codepoint >= BMG_PICTURE_FONT_TAG_BASE &&
           codepoint <= static_cast<char16_t>(BMG_PICTURE_FONT_TAG_BASE + BMG_PICTURE_FONT_TAG_MAX_PAYLOAD);
}

[[nodiscard]] constexpr char16_t make_bmg_picture_font_tag(std::uint16_t payload) {
    return static_cast<char16_t>(BMG_PICTURE_FONT_TAG_BASE + payload);
}

[[nodiscard]] constexpr std::uint16_t bmg_picture_font_tag_payload(char16_t codepoint) {
    return static_cast<std::uint16_t>(codepoint - BMG_PICTURE_FONT_TAG_BASE);
}

[[nodiscard]] constexpr std::uint16_t bmg_picture_font_codepoint(std::uint16_t payload) {
    return static_cast<std::uint16_t>(0x30U + payload);
}

[[nodiscard]] AssetResult<BmgMessageMap> parse_bmg_messages(
    std::span<const std::byte> bmg_bytes,
    std::span<const std::byte> message_id_table_bytes);

}  // namespace smgpc::assets::layout
