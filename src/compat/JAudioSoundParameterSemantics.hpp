#pragma once

#include <cstdint>
#include <optional>

namespace smgpc::compat {

    struct JAudioSoundParameterAdjustment {
        float gain_multiplier = 1.0F;
        float pitch_multiplier = 1.0F;
    };

    // Returns an adjustment only for sound IDs whose complete Kawamura,
    // Takezawa, and Gohara modifier behavior has been audited. Absence means
    // playback must stop at this compatibility boundary rather than guess.
    [[nodiscard]] std::optional<JAudioSoundParameterAdjustment>
    resolve_jaudio_sound_parameter_adjustment(
        std::uint32_t sound_id, std::int32_t parameter_1,
        std::int32_t parameter_2);

} // namespace smgpc::compat
