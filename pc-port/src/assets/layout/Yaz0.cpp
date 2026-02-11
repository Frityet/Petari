#include "Yaz0.hpp"

#include <string>

#include "Binary.hpp"

namespace smgpc::assets::layout {
namespace {

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message)
    };
}

}  // namespace

bool is_yaz0(std::span<const std::byte> bytes) {
    using namespace binary;
    return bytes.size() >= 16U and fourcc_equals(bytes, 0U, "Yaz0");
}

AssetResult<std::vector<std::byte>> decode_yaz0(std::span<const std::byte> bytes) {
    using namespace binary;

    if (not is_yaz0(bytes)) {
        return make_error("Input does not start with a Yaz0 header.");
    }

    const auto decompressed_size = static_cast<std::size_t>(read_u32_be(bytes, 4U));
    if (decompressed_size == 0U) {
        return std::vector<std::byte> {};
    }

    std::vector<std::byte> output {};
    output.reserve(decompressed_size);

    std::size_t source_cursor = 16U;
    std::uint8_t group_header = 0U;
    int bits_left = 0;

    while (output.size() < decompressed_size) {
        if (bits_left == 0) {
            if (source_cursor >= bytes.size()) {
                return make_error("Unexpected EOF in Yaz0 control stream.");
            }
            group_header = read_u8(bytes, source_cursor++);
            bits_left = 8;
        }

        if ((group_header & 0x80U) != 0U) {
            if (source_cursor >= bytes.size()) {
                return make_error("Unexpected EOF in Yaz0 literal stream.");
            }
            output.push_back(bytes[source_cursor++]);
        } else {
            if (source_cursor + 1U >= bytes.size()) {
                return make_error("Unexpected EOF in Yaz0 back-reference stream.");
            }

            const auto byte1 = read_u8(bytes, source_cursor++);
            const auto byte2 = read_u8(bytes, source_cursor++);
            const std::size_t back_distance = ((static_cast<std::size_t>(byte1 & 0x0FU) << 8U) | static_cast<std::size_t>(byte2)) + 1U;

            std::size_t run_length = static_cast<std::size_t>(byte1 >> 4U);
            if (run_length == 0U) {
                if (source_cursor >= bytes.size()) {
                    return make_error("Unexpected EOF in Yaz0 extended run length.");
                }
                run_length = static_cast<std::size_t>(read_u8(bytes, source_cursor++)) + 0x12U;
            } else {
                run_length += 2U;
            }

            if (back_distance > output.size()) {
                return make_error("Yaz0 back-reference points before output origin.");
            }

            for (std::size_t i = 0; i < run_length and output.size() < decompressed_size; ++i) {
                const auto value = output[output.size() - back_distance];
                output.push_back(value);
            }
        }

        group_header <<= 1U;
        --bits_left;
    }

    return output;
}

}  // namespace smgpc::assets::layout
