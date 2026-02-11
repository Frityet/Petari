#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::assets::layout::binary {

[[nodiscard]] inline bool has_bytes(std::span<const std::byte> bytes, std::size_t offset, std::size_t size) {
    return offset <= bytes.size() and size <= (bytes.size() - offset);
}

[[nodiscard]] inline std::uint8_t read_u8(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint8_t>(bytes[offset]);
}

[[nodiscard]] inline std::uint16_t read_u16_be(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(read_u8(bytes, offset)) << 8U) |
                                      static_cast<std::uint16_t>(read_u8(bytes, offset + 1U)));
}

[[nodiscard]] inline std::uint32_t read_u24_be(std::span<const std::byte> bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(read_u8(bytes, offset)) << 16U) |
           (static_cast<std::uint32_t>(read_u8(bytes, offset + 1U)) << 8U) |
           static_cast<std::uint32_t>(read_u8(bytes, offset + 2U));
}

[[nodiscard]] inline std::uint32_t read_u32_be(std::span<const std::byte> bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(read_u8(bytes, offset)) << 24U) |
           (static_cast<std::uint32_t>(read_u8(bytes, offset + 1U)) << 16U) |
           (static_cast<std::uint32_t>(read_u8(bytes, offset + 2U)) << 8U) |
           static_cast<std::uint32_t>(read_u8(bytes, offset + 3U));
}

[[nodiscard]] inline std::int32_t read_s32_be(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::int32_t>(read_u32_be(bytes, offset));
}

[[nodiscard]] inline float read_f32_be(std::span<const std::byte> bytes, std::size_t offset) {
    std::uint32_t bits = read_u32_be(bytes, offset);
    float result {};
    static_assert(sizeof(result) == sizeof(bits));
    std::memcpy(&result, &bits, sizeof(float));
    return result;
}

[[nodiscard]] inline std::uint16_t read_u16_le(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(read_u8(bytes, offset)) |
                                      (static_cast<std::uint16_t>(read_u8(bytes, offset + 1U)) << 8U));
}

[[nodiscard]] inline std::uint32_t read_u32_le(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(read_u8(bytes, offset)) |
           (static_cast<std::uint32_t>(read_u8(bytes, offset + 1U)) << 8U) |
           (static_cast<std::uint32_t>(read_u8(bytes, offset + 2U)) << 16U) |
           (static_cast<std::uint32_t>(read_u8(bytes, offset + 3U)) << 24U);
}

[[nodiscard]] inline std::uint64_t read_u64_le(std::span<const std::byte> bytes, std::size_t offset) {
    return static_cast<std::uint64_t>(read_u32_le(bytes, offset)) |
           (static_cast<std::uint64_t>(read_u32_le(bytes, offset + 4U)) << 32U);
}

[[nodiscard]] inline std::array<char, 4> read_fourcc(std::span<const std::byte> bytes, std::size_t offset) {
    return {
        static_cast<char>(read_u8(bytes, offset)),
        static_cast<char>(read_u8(bytes, offset + 1U)),
        static_cast<char>(read_u8(bytes, offset + 2U)),
        static_cast<char>(read_u8(bytes, offset + 3U)),
    };
}

[[nodiscard]] inline bool fourcc_equals(std::span<const std::byte> bytes, std::size_t offset, std::string_view expected) {
    if (expected.size() != 4U or not has_bytes(bytes, offset, 4U)) {
        return false;
    }
    return static_cast<char>(read_u8(bytes, offset + 0U)) == expected[0] and
           static_cast<char>(read_u8(bytes, offset + 1U)) == expected[1] and
           static_cast<char>(read_u8(bytes, offset + 2U)) == expected[2] and
           static_cast<char>(read_u8(bytes, offset + 3U)) == expected[3];
}

[[nodiscard]] inline std::string read_c_string(std::span<const std::byte> bytes, std::size_t offset) {
    if (offset >= bytes.size()) {
        return {};
    }

    std::string text {};
    for (std::size_t i = offset; i < bytes.size(); ++i) {
        const auto character = static_cast<char>(read_u8(bytes, i));
        if (character == '\0') {
            break;
        }
        text.push_back(character);
    }
    return text;
}

[[nodiscard]] inline std::string read_fixed_string(std::span<const std::byte> bytes, std::size_t offset, std::size_t size) {
    if (not has_bytes(bytes, offset, size)) {
        return {};
    }

    std::string text {};
    text.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        const auto character = static_cast<char>(read_u8(bytes, offset + i));
        if (character == '\0') {
            break;
        }
        text.push_back(character);
    }
    return text;
}

[[nodiscard]] inline std::string to_lower_ascii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char value) {
        if (value >= 'A' and value <= 'Z') {
            return static_cast<char>(value - 'A' + 'a');
        }
        return static_cast<char>(value);
    });
    return text;
}

[[nodiscard]] inline std::span<const std::byte> subspan(std::span<const std::byte> bytes, std::size_t offset, std::size_t size) {
    if (not has_bytes(bytes, offset, size)) {
        return {};
    }
    return bytes.subspan(offset, size);
}

}  // namespace smgpc::assets::layout::binary
