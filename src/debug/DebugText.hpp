#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace smgpc::debug {

    [[nodiscard]] std::string lowercase(std::string_view text);
    [[nodiscard]] bool ends_with_ignore_case(std::string_view text, std::string_view suffix);
    [[nodiscard]] std::string sanitize_filename(std::string_view text, std::string_view fallback = "debug");
    [[nodiscard]] std::string tag_string(std::uint32_t value);

}  // namespace smgpc::debug
