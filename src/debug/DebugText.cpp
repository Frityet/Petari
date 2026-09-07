#include "DebugText.hpp"

#include <algorithm>
#include <cctype>

namespace smgpc::debug {

    std::string lowercase(std::string_view text) {
        auto lowered = std::string(text);
        std::ranges::transform(lowered, lowered.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return lowered;
    }

    bool ends_with_ignore_case(std::string_view text, std::string_view suffix) {
        if (text.size() < suffix.size()) {
            return false;
        }

        return lowercase(text.substr(text.size() - suffix.size())) == lowercase(suffix);
    }

    std::string sanitize_filename(std::string_view text, std::string_view fallback) {
        auto sanitized = std::string {};
        sanitized.reserve(text.size());

        for (const auto c : text) {
            const auto value = static_cast<unsigned char>(c);
            if (std::isalnum(value) != 0 || c == '-' || c == '_') {
                sanitized.push_back(c);
            } else {
                sanitized.push_back('_');
            }
        }

        return sanitized.empty() ? std::string(fallback) : sanitized;
    }

    std::string tag_string(std::uint32_t value) {
        auto text = std::string {};
        text.push_back(static_cast<char>((value >> 24U) & 0xffU));
        text.push_back(static_cast<char>((value >> 16U) & 0xffU));
        text.push_back(static_cast<char>((value >> 8U) & 0xffU));
        text.push_back(static_cast<char>(value & 0xffU));
        return text;
    }

}  // namespace smgpc::debug
