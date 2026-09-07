#include "compat/PlayerStatusStorage.hpp"

#include <aurora/endian.hpp>
#include <algorithm>
#include <array>
#include <cstring>

namespace smgpc::compat {
namespace {
constexpr std::size_t cPayloadSize = 7;

std::size_t available_bytes(const void* buffer, u32 size) {
    // JSUMemoryStream takes a signed size and advances only for copied bytes.
    return buffer != nullptr && static_cast<s32>(size) > 0
               ? std::min<std::size_t>(size, cPayloadSize)
               : 0;
}
}  // namespace

s32 PlayerStatusStorage::serialize(u8* buffer, u32 size) const {
    std::array<u8, cPayloadSize> payload{};
    GameDataPlayerStatus::serialize(payload.data(), payload.size());
    u32 stocked;
    u16 lives;
    std::memcpy(&stocked, payload.data() + 1, sizeof(stocked));
    std::memcpy(&lives, payload.data() + 5, sizeof(lives));
    aurora::endian::write_big(payload.data() + 1, stocked);
    aurora::endian::write_u16(payload.data() + 5, lives);
    const auto copied = available_bytes(buffer, size);
    if (copied != 0) std::memcpy(buffer, payload.data(), copied);
    return static_cast<s32>(copied);
}

s32 PlayerStatusStorage::deserialize(const u8* buffer, u32 size) {
    // Original initializeData gives supply4. A truncated Wii read replaces
    // the most significant bytes first, leaving the remaining defaults.
    std::array<u8, cPayloadSize> payload{0, 0, 0, 0, 0, 0, 4};
    const auto copied = available_bytes(buffer, size);
    if (copied != 0) std::memcpy(payload.data(), buffer, copied);
    const u32 stocked = aurora::endian::read_u32(payload.data() + 1);
    const u16 supply = aurora::endian::read_u16(payload.data() + 5);
    std::memcpy(payload.data() + 1, &stocked, sizeof(stocked));
    std::memcpy(payload.data() + 5, &supply, sizeof(supply));
    // Supplying padded scalar fields lets the unchanged original body perform
    // initialization, saved-lives-to-supply assignment, and live-lives reset.
    return GameDataPlayerStatus::deserialize(payload.data(), payload.size());
}

}  // namespace smgpc::compat
