#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace smgpc::resource {

// AudRemixMgr builds its native pointer tables over these packed 32-bit words.
// Keep this storage alive for as long as the original manager and sequencer.
[[nodiscard]] std::vector<std::uint32_t> decode_remix_sequence(std::span<const std::uint8_t>);

}
