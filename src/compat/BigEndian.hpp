#pragma once

#include <cstdint>

namespace smgpc::compat {
    // GX command streams and packed byte fields retain their Wii byte order.
    inline std::uint32_t read_be_u32(const void* source) {
        const auto* bytes = static_cast<const std::uint8_t*>(source);
        return (std::uint32_t(bytes[0]) << 24) | (std::uint32_t(bytes[1]) << 16) |
               (std::uint32_t(bytes[2]) << 8) | std::uint32_t(bytes[3]);
    }
}
