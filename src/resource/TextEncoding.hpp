#pragma once

#include "resource/detail/Cp932Table.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace smgpc::resource {

    [[nodiscard]] std::u16string utf16_from_utf8_lossy(std::string_view text);
    [[nodiscard]] std::string utf8_from_utf16_lossy(std::u16string_view text);

    // Nintendo-authored BCSV/JMap strings use Windows-31J (CP932). Preserve
    // those bytes for Game identity; decode explicitly for UTF-8 presentation.
    [[nodiscard]] std::string decode_cp932(std::string_view value);

    // Convert host UTF-8 text explicitly before passing it to Game APIs.
    // Invalid UTF-8 and characters without a CP932 representation throw;
    // replacement characters and best-fit substitutions would change identity.
    [[nodiscard]] std::string encode_cp932(std::string_view value);

}  // namespace smgpc::resource

namespace smgpc::resource::cp932 {
    // Structural UTF-8 input for a class non-type template parameter. Preserve
    // the complete array, including embedded NULs and its final terminator.
    template <std::size_t N>
    struct Utf8Literal {
        char bytes[N];

        consteval Utf8Literal(const char (&text)[N]) {
            for (std::size_t i = 0; i != N; ++i) bytes[i] = text[i];
            if (bytes[N - 1] != '\0') throw "CP932 input must have a terminating NUL";
        }

        consteval Utf8Literal(const char8_t (&text)[N]) {
            for (std::size_t i = 0; i != N; ++i) bytes[i] = static_cast<char>(text[i]);
            if (bytes[N - 1] != '\0') throw "CP932 input must have a terminating NUL";
        }
    };

    template <std::size_t N> Utf8Literal(const char (&)[N]) -> Utf8Literal<N>;
    template <std::size_t N> Utf8Literal(const char8_t (&)[N]) -> Utf8Literal<N>;

    namespace detail {
        template <Utf8Literal Text>
        consteval std::uint32_t next_codepoint(std::size_t& cursor) {
            constexpr auto length = sizeof(Text.bytes) - 1;
            const auto first = static_cast<unsigned char>(Text.bytes[cursor++]);
            if (first < 0x80) return first;
            std::uint32_t value;
            std::uint32_t minimum;
            std::size_t trailing;
            if (first >= 0xC2 && first <= 0xDF) {
                value = first & 0x1F;
                minimum = 0x80;
                trailing = 1;
            } else if (first >= 0xE0 && first <= 0xEF) {
                value = first & 0x0F;
                minimum = 0x800;
                trailing = 2;
            } else if (first >= 0xF0 && first <= 0xF4) {
                value = first & 0x07;
                minimum = 0x10000;
                trailing = 3;
            } else {
                throw "CP932 input is not valid UTF-8: invalid leading byte";
            }
            if (trailing > length - cursor) throw "CP932 input is not valid UTF-8: truncated sequence";
            for (std::size_t i = 0; i != trailing; ++i) {
                const auto byte = static_cast<unsigned char>(Text.bytes[cursor++]);
                if ((byte & 0xC0) != 0x80) throw "CP932 input is not valid UTF-8: invalid continuation byte";
                value = (value << 6) | (byte & 0x3F);
            }
            if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
                throw "CP932 input is not valid UTF-8: invalid scalar value";
            return value;
        }

        consteval std::uint16_t encode_codepoint(std::uint32_t value) {
            if (value < 0x80) return static_cast<std::uint16_t>(value);
            std::size_t first = 0;
            std::size_t last = sizeof(mapping) / sizeof(mapping[0]);
            while (first < last) {
                const auto middle = first + (last - first) / 2;
                if (mapping[middle].unicode < value) first = middle + 1;
                else last = middle;
            }
            if (first == sizeof(mapping) / sizeof(mapping[0]) || mapping[first].unicode != value)
                throw "Unicode character is not representable in CP932";
            return mapping[first].encoded;
        }

        template <Utf8Literal Text>
        consteval std::size_t encoded_length() {
            std::size_t result = 0;
            std::size_t cursor = 0;
            while (cursor != sizeof(Text.bytes) - 1) {
                const auto code = encode_codepoint(next_codepoint<Text>(cursor));
                result += code > 0xFF ? 2 : 1;
            }
            return result;
        }

        template <std::size_t N>
        struct EncodedBytes {
            char bytes[N]{};
        };

        template <Utf8Literal Text>
        consteval auto encode() {
            EncodedBytes<encoded_length<Text>() + 1> result;
            std::size_t cursor = 0;
            std::size_t output = 0;
            while (cursor != sizeof(Text.bytes) - 1) {
                const auto code = encode_codepoint(next_codepoint<Text>(cursor));
                if (code > 0xFF) result.bytes[output++] = static_cast<char>(code >> 8);
                result.bytes[output++] = static_cast<char>(code & 0xFF);
            }
            return result;
        }
    }

    template <Utf8Literal Text>
    struct EncodedLiteral {
        inline static constexpr auto value = detail::encode<Text>();
    };
}

// Returns a const char array lvalue with static storage and the exact encoded
// extent, like an ordinary narrow literal. The compiler concatenates adjacent
// input literals and resolves escapes before this UTF-8-to-CP932 conversion.
#define CP932(text) (::smgpc::resource::cp932::EncodedLiteral<::smgpc::resource::cp932::Utf8Literal{text}>::value.bytes)
