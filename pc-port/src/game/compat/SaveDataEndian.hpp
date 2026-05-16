#pragma once

#include "compat/Types.hpp"

namespace SaveDataEndian {

inline u16 read_u16(const u8 *bytes) {
    return static_cast<u16>((static_cast<u16>(bytes[0]) << 8U) | bytes[1]);
}

inline u32 read_u32(const u8 *bytes) {
    return (static_cast<u32>(bytes[0]) << 24U) |
           (static_cast<u32>(bytes[1]) << 16U) |
           (static_cast<u32>(bytes[2]) << 8U) |
           static_cast<u32>(bytes[3]);
}

inline u64 read_u64(const u8 *bytes) {
    return (static_cast<u64>(read_u32(bytes)) << 32U) | read_u32(bytes + 4U);
}

inline void write_u16(u8 *bytes, u16 value) {
    bytes[0] = static_cast<u8>(value >> 8U);
    bytes[1] = static_cast<u8>(value);
}

inline void write_u32(u8 *bytes, u32 value) {
    bytes[0] = static_cast<u8>(value >> 24U);
    bytes[1] = static_cast<u8>(value >> 16U);
    bytes[2] = static_cast<u8>(value >> 8U);
    bytes[3] = static_cast<u8>(value);
}

inline void write_u64(u8 *bytes, u64 value) {
    write_u32(bytes, static_cast<u32>(value >> 32U));
    write_u32(bytes + 4U, static_cast<u32>(value));
}

}  // namespace SaveDataEndian
