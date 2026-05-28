#include "Yaz0.hpp"

#include <stdexcept>

namespace smgpc::compat {
    namespace {

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
            if (offset + 4U > data.size()) {
                throw std::runtime_error("Yaz0 read past end of buffer");
            }

            return (static_cast<std::uint32_t>(data[offset]) << 24U) | (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) | (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) | static_cast<std::uint32_t>(data[offset + 3U]);
        }

    }  // namespace

    bool is_yaz0(std::span<const std::uint8_t> data) {
        return data.size() >= 16U && data[0] == 'Y' && data[1] == 'a' && data[2] == 'z' && data[3] == '0';
    }

    std::vector<std::uint8_t> decompress_yaz0(std::span<const std::uint8_t> data) {
        if (!is_yaz0(data)) {
            return {data.begin(), data.end()};
        }

        const auto decompressed_size = read_be32(data, 4U);
        auto output = std::vector<std::uint8_t>(decompressed_size);

        std::size_t src_offset = 16U;
        std::size_t dst_offset = 0U;
        std::uint8_t code = 0U;
        auto valid_bits = 0U;

        while (dst_offset < output.size()) {
            if (valid_bits == 0U) {
                if (src_offset >= data.size()) {
                    throw std::runtime_error("Yaz0 code byte is outside compressed data");
                }
                code = data[src_offset++];
                valid_bits = 8U;
            }

            if ((code & 0x80U) != 0U) {
                if (src_offset >= data.size()) {
                    throw std::runtime_error("Yaz0 literal byte is outside compressed data");
                }
                output[dst_offset++] = data[src_offset++];
            } else {
                if (src_offset + 2U > data.size()) {
                    throw std::runtime_error("Yaz0 copy command is outside compressed data");
                }

                const auto byte1 = data[src_offset++];
                const auto byte2 = data[src_offset++];
                auto copy_count = static_cast<std::size_t>(byte1 >> 4U);
                const auto distance = static_cast<std::size_t>(((byte1 & 0x0FU) << 8U) | byte2) + 1U;

                if (copy_count == 0U) {
                    if (src_offset >= data.size()) {
                        throw std::runtime_error("Yaz0 long copy count is outside compressed data");
                    }
                    copy_count = static_cast<std::size_t>(data[src_offset++]) + 0x12U;
                } else {
                    copy_count += 2U;
                }

                if (distance > dst_offset) {
                    throw std::runtime_error("Yaz0 copy command references data before output start");
                }

                for (std::size_t i = 0U; i < copy_count && dst_offset < output.size(); ++i) {
                    output[dst_offset] = output[dst_offset - distance];
                    ++dst_offset;
                }
            }

            code <<= 1U;
            --valid_bits;
        }

        return output;
    }

}  // namespace smgpc::compat
