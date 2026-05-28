#include "BinaryChunkFile.hpp"

#include <algorithm>
#include <limits>

namespace smgpc::common {

    std::uint32_t fourcc(char a, char b, char c, char d) {
        return (static_cast<std::uint32_t>(static_cast<std::uint8_t>(a)) << 24U) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 16U) |
               (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c)) << 8U) |
               static_cast<std::uint32_t>(static_cast<std::uint8_t>(d));
    }

    std::uint32_t hash_code_31(std::string_view text) {
        auto hash = std::uint32_t{};
        for (const auto ch : text) {
            hash = static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch) + hash * 31U);
        }
        return hash;
    }

    std::uint16_t read_be16(std::span<const std::uint8_t> bytes, std::size_t offset) {
        if (offset + sizeof(std::uint16_t) > bytes.size()) {
            return 0U;
        }

        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1U]);
    }

    std::uint32_t read_be32(std::span<const std::uint8_t> bytes, std::size_t offset) {
        if (offset + sizeof(std::uint32_t) > bytes.size()) {
            return 0U;
        }

        return (static_cast<std::uint32_t>(bytes[offset]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) | static_cast<std::uint32_t>(bytes[offset + 3U]);
    }

    std::uint64_t read_be64(std::span<const std::uint8_t> bytes, std::size_t offset) {
        if (offset + sizeof(std::uint64_t) > bytes.size()) {
            return 0U;
        }

        auto value = std::uint64_t{};
        for (auto index = std::size_t{}; index < sizeof(std::uint64_t); ++index) {
            value = (value << 8U) | bytes[offset + index];
        }
        return value;
    }

    void append_be16(std::vector<std::uint8_t> &bytes, std::uint16_t value) {
        bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
        bytes.push_back(static_cast<std::uint8_t>(value));
    }

    void append_be32(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
        bytes.push_back(static_cast<std::uint8_t>(value >> 24U));
        bytes.push_back(static_cast<std::uint8_t>(value >> 16U));
        bytes.push_back(static_cast<std::uint8_t>(value >> 8U));
        bytes.push_back(static_cast<std::uint8_t>(value));
    }

    void append_be64(std::vector<std::uint8_t> &bytes, std::uint64_t value) {
        for (auto shift = 56; shift >= 0; shift -= 8) {
            bytes.push_back(static_cast<std::uint8_t>(value >> static_cast<unsigned>(shift)));
        }
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value) {
        if (offset + sizeof(value) > bytes.size()) {
            return;
        }

        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
        if (offset + sizeof(value) > bytes.size()) {
            return;
        }

        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be64(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint64_t value) {
        if (offset + sizeof(value) > bytes.size()) {
            return;
        }

        for (auto index = std::size_t{}; index < sizeof(value); ++index) {
            const auto shift = static_cast<unsigned>((sizeof(value) - 1U - index) * 8U);
            bytes[offset + index] = static_cast<std::uint8_t>(value >> shift);
        }
    }

    std::vector<std::uint8_t> encode_binary_chunk_file(std::span<const BinaryChunk> chunks, std::size_t minimum_size) {
        auto bytes = std::vector<std::uint8_t>{1U, static_cast<std::uint8_t>(std::min<std::size_t>(chunks.size(), 0xffU)), 0U, 0U};
        for (const auto &chunk : chunks) {
            if (chunk.data.size() > std::numeric_limits<std::uint32_t>::max() - 12U) {
                continue;
            }

            append_be32(bytes, chunk.signature);
            append_be32(bytes, chunk.hash);
            append_be32(bytes, static_cast<std::uint32_t>(12U + chunk.data.size()));
            bytes.insert(bytes.end(), chunk.data.begin(), chunk.data.end());
        }

        if (bytes.size() < minimum_size) {
            bytes.resize(minimum_size);
        }
        return bytes;
    }

    std::optional<std::vector<BinaryChunk>> decode_binary_chunk_file(std::span<const std::uint8_t> bytes) {
        if (bytes.size() < 4U || bytes[0U] != 1U) {
            return std::nullopt;
        }

        auto chunks = std::vector<BinaryChunk>{};
        chunks.reserve(bytes[1U]);
        auto offset = std::size_t{4U};
        for (auto index = std::uint8_t{}; index < bytes[1U]; ++index) {
            if (offset + 12U > bytes.size()) {
                return std::nullopt;
            }

            const auto signature = read_be32(bytes, offset);
            const auto hash = read_be32(bytes, offset + 4U);
            const auto chunk_size = read_be32(bytes, offset + 8U);
            if (chunk_size < 12U || offset + chunk_size > bytes.size()) {
                return std::nullopt;
            }

            chunks.push_back(BinaryChunk{
                .signature = signature,
                .hash = hash,
                .data = std::vector<std::uint8_t>(bytes.begin() + static_cast<std::ptrdiff_t>(offset + 12U),
                                                  bytes.begin() + static_cast<std::ptrdiff_t>(offset + chunk_size)),
            });
            offset += chunk_size;
        }
        return chunks;
    }

}  // namespace smgpc::common
