#include "TextEncoding.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <iconv.h>
#endif

namespace smgpc::resource {
    namespace {

        constexpr auto kReplacementCharacter = char32_t {0xfffdU};

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
        auto out = std::u16string {};
        out.reserve(text.size());

        for (std::size_t i = 0U; i < text.size();) {
            const auto first = static_cast<unsigned char>(text[i]);
            auto code = char32_t {};
            auto length = std::size_t {0U};

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
        auto out = std::string {};
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

    std::string decode_cp932(std::string_view value) {
        if (value.empty()) {
            return {};
        }

#if defined(_WIN32)
        if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("CP932 string is too large to convert");
        }

        const auto input_size = static_cast<int>(value.size());
        const auto wide_size = MultiByteToWideChar(932U, MB_ERR_INVALID_CHARS, value.data(), input_size, nullptr, 0);
        if (wide_size <= 0) {
            throw std::runtime_error("invalid CP932 string (Windows error " + std::to_string(GetLastError()) + ")");
        }
        auto wide = std::wstring(static_cast<std::size_t>(wide_size), L'\0');
        if (MultiByteToWideChar(932U, MB_ERR_INVALID_CHARS, value.data(), input_size, wide.data(), wide_size) != wide_size) {
            throw std::runtime_error("cannot decode CP932 string (Windows error " + std::to_string(GetLastError()) + ")");
        }

        const auto output_size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), wide_size, nullptr, 0, nullptr, nullptr);
        if (output_size <= 0) {
            throw std::runtime_error("cannot size UTF-8 string (Windows error " + std::to_string(GetLastError()) + ")");
        }
        auto output = std::string(static_cast<std::size_t>(output_size), '\0');
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), wide_size, output.data(), output_size, nullptr, nullptr) != output_size) {
            throw std::runtime_error("cannot encode UTF-8 string (Windows error " + std::to_string(GetLastError()) + ")");
        }
        return output;
#else
        auto converter = iconv_open("UTF-8", "CP932");
        if (converter == reinterpret_cast<iconv_t>(-1)) {
            converter = iconv_open("UTF-8", "SHIFT-JIS");
        }
        if (converter == reinterpret_cast<iconv_t>(-1)) {
            throw std::runtime_error("CP932 text conversion is unavailable");
        }

        auto input = std::string(value);
        auto output = std::string(input.size() * 4U + 1U, '\0');
        auto *input_cursor = input.data();
        auto input_remaining = input.size();
        auto *output_cursor = output.data();
        auto output_remaining = output.size();
        errno = 0;
        const auto result = iconv(converter, &input_cursor, &input_remaining, &output_cursor, &output_remaining);
        const auto conversion_error = errno;
        iconv_close(converter);
        if (result == static_cast<std::size_t>(-1) || input_remaining != 0U) {
            throw std::runtime_error("invalid CP932 string (iconv error " + std::to_string(conversion_error) + ")");
        }

        output.resize(static_cast<std::size_t>(output_cursor - output.data()));
        return output;
#endif
    }

}  // namespace smgpc::resource
