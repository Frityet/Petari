#pragma once

#include <cstdint>
#include <span>

namespace smgpc::common {

    // Check the byte extents before handing an external file to the original
    // unsized chunk reader. Chunk hashes, payloads and error policy belong to it.
    [[nodiscard]] bool has_bounded_binary_chunks(std::span<const std::uint8_t> bytes);

}  // namespace smgpc::common
