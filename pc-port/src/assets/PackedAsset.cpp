#include "PackedAsset.hpp"

#include <array>
#include <string>

#include "layout/Binary.hpp"

namespace smgpc::assets {
namespace {

[[nodiscard]] AssetError make_error(std::string message) {
    return AssetError {
        .code = AssetErrorCode::InvalidFormat,
        .message = std::move(message)
    };
}

}  // namespace

AssetResult<std::vector<std::byte>> unpack_packed_asset(std::span<const std::byte> packed_bytes) {
    using namespace layout::binary;

    constexpr std::size_t MAGIC_OFFSET = 0U;
    constexpr std::size_t VERSION_OFFSET = 8U;
    constexpr std::size_t LOGICAL_PATH_SIZE_OFFSET = 12U;
    constexpr std::size_t SOURCE_SIZE_OFFSET = 16U;
    constexpr std::size_t SOURCE_HASH_OFFSET = 24U;
    constexpr std::size_t HEADER_SIZE = 32U;
    constexpr std::array<char, 8> MAGIC {'S', 'M', 'G', 'P', 'C', 'A', 'S', '1'};

    if (packed_bytes.size() < HEADER_SIZE) {
        return make_error("Packed asset is shorter than the SMGPCAS1 header.");
    }

    for (std::size_t i = 0; i < MAGIC.size(); ++i) {
        if (static_cast<char>(read_u8(packed_bytes, MAGIC_OFFSET + i)) != MAGIC[i]) {
            return make_error("Packed asset magic mismatch.");
        }
    }

    const auto version = read_u32_le(packed_bytes, VERSION_OFFSET);
    if (version != 1U) {
        return make_error("Unsupported packed asset version.");
    }

    const auto logical_path_size = static_cast<std::size_t>(read_u32_le(packed_bytes, LOGICAL_PATH_SIZE_OFFSET));
    const auto source_size = static_cast<std::size_t>(read_u64_le(packed_bytes, SOURCE_SIZE_OFFSET));
    const auto _source_hash = read_u64_le(packed_bytes, SOURCE_HASH_OFFSET);
    (void)_source_hash;

    if (logical_path_size > packed_bytes.size() - HEADER_SIZE) {
        return make_error("Packed asset logical path size exceeds payload bounds.");
    }

    const std::size_t source_offset = HEADER_SIZE + logical_path_size;
    if (source_size > packed_bytes.size() - source_offset) {
        return make_error("Packed asset source payload exceeds file bounds.");
    }

    const auto source_span = subspan(packed_bytes, source_offset, source_size);
    if (source_span.empty() and source_size > 0U) {
        return make_error("Packed asset payload extraction failed.");
    }

    return std::vector<std::byte>(source_span.begin(), source_span.end());
}

}  // namespace smgpc::assets
