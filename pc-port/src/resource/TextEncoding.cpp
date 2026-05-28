#include "TextEncoding.hpp"

#include <algorithm>
#include <cstdint>

namespace smgpc::resource {
    namespace {

        constexpr auto kReplacementCharacter = char32_t{0xfffdU};

        void append_utf8(std::string &out, char32_t code) {
            if (code <= 0x7fU) {
                out.push_back(static_cast<char>(code));
                return;
            }

            if (code <= 0x7ffU) {
                out.push_back(static_cast<char>(0xc0U | (code >> 6U)));
                out.push_back(static_cast<char>(0x80U | (code & 0x3fU)));
                return;
            }

            if (code <= 0xffffU) {
                out.push_back(static_cast<char>(0xe0U | (code >> 12U)));
                out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3fU)));
                out.push_back(static_cast<char>(0x80U | (code & 0x3fU)));
                return;
            }

            out.push_back(static_cast<char>(0xf0U | (code >> 18U)));
            out.push_back(static_cast<char>(0x80U | ((code >> 12U) & 0x3fU)));
            out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3fU)));
            out.push_back(static_cast<char>(0x80U | (code & 0x3fU)));
        }

        [[nodiscard]] bool is_low_surrogate(char16_t code) {
            return code >= 0xdc00U && code <= 0xdfffU;
        }

        [[nodiscard]] bool is_high_surrogate(char16_t code) {
            return code >= 0xd800U && code <= 0xdbffU;
        }

        [[nodiscard]] bool is_continuation_byte(unsigned char ch) {
            return (ch & 0xc0U) == 0x80U;
        }

    }  // namespace

    std::u16string utf16_from_utf8_lossy(std::string_view text) {
        auto out = std::u16string{};
        out.reserve(text.size());

        for (std::size_t i = 0U; i < text.size();) {
            const auto first = static_cast<unsigned char>(text[i]);
            auto code = char32_t{};
            auto length = std::size_t{0U};

            if (first < 0x80U) {
                code = first;
                length = 1U;
            }
            else if ((first & 0xe0U) == 0xc0U) {
                code = first & 0x1fU;
                length = 2U;
            }
            else if ((first & 0xf0U) == 0xe0U) {
                code = first & 0x0fU;
                length = 3U;
            }
            else if ((first & 0xf8U) == 0xf0U) {
                code = first & 0x07U;
                length = 4U;
            }
            else {
                out.push_back(static_cast<char16_t>(kReplacementCharacter));
                ++i;
                continue;
            }

            if (i + length > text.size()) {
                out.push_back(static_cast<char16_t>(kReplacementCharacter));
                break;
            }

            auto valid = true;
            for (std::size_t byte = 1U; byte < length; ++byte) {
                const auto next = static_cast<unsigned char>(text[i + byte]);
                if (!is_continuation_byte(next)) {
                    valid = false;
                    break;
                }
                code = (code << 6U) | (next & 0x3fU);
            }

            const auto overlong = (length == 2U && code < 0x80U) || (length == 3U && code < 0x800U) || (length == 4U && code < 0x10000U);
            if (!valid || overlong || code > 0x10ffffU || (code >= 0xd800U && code <= 0xdfffU)) {
                out.push_back(static_cast<char16_t>(kReplacementCharacter));
                ++i;
                continue;
            }

            if (code <= 0xffffU) {
                out.push_back(static_cast<char16_t>(code));
            }
            else {
                const auto scalar = code - 0x10000U;
                out.push_back(static_cast<char16_t>(0xd800U + (scalar >> 10U)));
                out.push_back(static_cast<char16_t>(0xdc00U + (scalar & 0x3ffU)));
            }

            i += length;
        }

        return out;
    }

    std::string utf8_from_utf16_lossy(std::u16string_view text) {
        auto out = std::string{};
        out.reserve(text.size());

        for (std::size_t i = 0U; i < text.size(); ++i) {
            const auto code = text[i];
            if (is_high_surrogate(code)) {
                if (i + 1U < text.size() && is_low_surrogate(text[i + 1U])) {
                    const auto high = static_cast<char32_t>(code - 0xd800U);
                    const auto low = static_cast<char32_t>(text[++i] - 0xdc00U);
                    append_utf8(out, 0x10000U + ((high << 10U) | low));
                }
                else {
                    append_utf8(out, kReplacementCharacter);
                }
                continue;
            }

            if (is_low_surrogate(code)) {
                append_utf8(out, kReplacementCharacter);
                continue;
            }

            append_utf8(out, code);
        }

        return out;
    }

}  // namespace smgpc::resource
