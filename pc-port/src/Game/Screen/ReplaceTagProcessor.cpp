#include "Game/Screen/ReplaceTagProcessor.hpp"

#include <algorithm>
#include <cstdarg>
#include <cwchar>
#include <string>

namespace {
    constexpr auto cTagMarker = wchar_t{0x001a};
    constexpr auto cNumericTagType = u16{0x06};
    constexpr auto cZeroPadTwoDigits = u16{0x0005};

    [[nodiscard]] u16 tag_size(wchar_t packed_size_type) {
        return static_cast<u16>((static_cast<u32>(packed_size_type) >> 8U) & 0xffU);
    }

    [[nodiscard]] u16 tag_type(wchar_t packed_size_type) {
        return static_cast<u16>(static_cast<u32>(packed_size_type) & 0xffU);
    }

    [[nodiscard]] s32 tag_word_count(wchar_t packed_size_type) {
        const auto size = tag_size(packed_size_type);
        return size >= 2U ? static_cast<s32>((size - 2U) / 2U) : 0;
    }

    [[nodiscard]] s32 numeric_arg_at(va_list args, s32 index) {
        va_list copy;
        va_copy(copy, args);
        auto value = s32{};
        for (auto i = s32{}; i <= index; ++i) {
            value = va_arg(copy, int);
        }
        va_end(copy);
        return value;
    }

    void append_number(std::wstring& out, s32 value, bool zero_pad_two_digits) {
        wchar_t buffer[32]{};
        if (zero_pad_two_digits) {
            std::swprintf(buffer, std::size(buffer), L"%02d", value);
        } else {
            std::swprintf(buffer, std::size(buffer), L"%d", value);
        }
        out.append(buffer);
    }

    void copy_out(wchar_t* pDst, s32 size, std::wstring_view text) {
        if (pDst == nullptr || size <= 0) {
            return;
        }

        const auto writable = static_cast<std::size_t>(size - 1);
        const auto count = std::min(writable, text.size());
        std::wmemcpy(pDst, text.data(), count);
        pDst[count] = L'\0';
    }
}  // namespace

namespace ReplaceTagFunction {
    u32 ReplaceArgs(wchar_t* pDst, s32 size, const wchar_t* pMessage, ...) {
        if (pDst == nullptr || size <= 0) {
            return 0U;
        }

        auto result = std::wstring{};
        va_list args;
        va_start(args, pMessage);

        for (auto cursor = pMessage; cursor != nullptr && *cursor != L'\0';) {
            if (*cursor != cTagMarker || cursor[1] == L'\0') {
                result.push_back(*cursor++);
                continue;
            }

            const auto word_count = tag_word_count(cursor[1]);
            if (word_count <= 0) {
                ++cursor;
                continue;
            }

            const auto type = tag_type(cursor[1]);
            if (type == cNumericTagType && word_count >= 6) {
                const auto format = static_cast<u16>(cursor[2]);
                const auto arg_index = static_cast<s32>(static_cast<u16>(cursor[6]));
                append_number(result, numeric_arg_at(args, arg_index), format == cZeroPadTwoDigits);
            }

            cursor += 1 + word_count;
        }

        va_end(args);
        copy_out(pDst, size, result);
        return static_cast<u32>(result.size());
    }
}  // namespace ReplaceTagFunction
