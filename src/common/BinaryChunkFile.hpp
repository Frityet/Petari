#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace smgpc::common {

    struct BinaryChunk {
        std::uint32_t signature = 0U;
        std::uint32_t hash = 0U;
        std::vector<std::uint8_t> data;
    };

    [[nodiscard]] std::uint32_t fourcc(char a, char b, char c, char d);
    [[nodiscard]] std::uint32_t hash_code_31(std::string_view text);
    [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> bytes, std::size_t offset);
    [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> bytes, std::size_t offset);
    [[nodiscard]] std::uint64_t read_be64(std::span<const std::uint8_t> bytes, std::size_t offset);
    void append_be16(std::vector<std::uint8_t> &bytes, std::uint16_t value);
    void append_be32(std::vector<std::uint8_t> &bytes, std::uint32_t value);
    void append_be64(std::vector<std::uint8_t> &bytes, std::uint64_t value);
    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value);
    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value);
    void write_be64(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint64_t value);
    [[nodiscard]] std::vector<std::uint8_t> encode_binary_chunk_file(std::span<const BinaryChunk> chunks, std::size_t minimum_size = 0U);
    [[nodiscard]] std::optional<std::vector<BinaryChunk>> decode_binary_chunk_file(std::span<const std::uint8_t> bytes);

}  // namespace smgpc::common
