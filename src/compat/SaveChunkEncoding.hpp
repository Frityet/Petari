#pragma once

#include <revolution/types.h>

class BinaryDataChunkBase;

namespace smgpc::compat {

// These original chunks produce native scalar bytes. Other chunk providers
// retain their existing wire format, including the BE-aware system config.
[[nodiscard]] bool is_native_save_chunk(u32 signature) noexcept;

// Validate Wii payloads before invoking any original deserializer. The holder
// should validate every recognized chunk in its first, non-mutating pass.
[[nodiscard]] bool validate_save_chunk_payload(u32 signature, const u8* payload, u32 size) noexcept;

// Invoke the actual chunk's virtual method and adapt its serialized bytes.
// A malformed input payload returns -1, outside the original 0/1 outcomes.
[[nodiscard]] s32 serialize_save_chunk(const BinaryDataChunkBase& chunk, u8* payload, u32 capacity);
[[nodiscard]] s32 deserialize_save_chunk(BinaryDataChunkBase& chunk, const u8* payload, u32 size);

} // namespace smgpc::compat
