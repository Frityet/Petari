#include "compat/JAudioSoundParameterSemantics.hpp"

#include <algorithm>
#include <array>

namespace smgpc::compat {
    namespace {

        enum class ParameterPolicy {
            KawamuraQuadraticPitch,
            Identity,
        };

        struct AuditedParameterRecord {
            std::uint32_t sound_id;
            ParameterPolicy policy;
        };

        // These IDs are the currently recovered subset of the three retail
        // AudSoundObject modifier functions. Extending the table requires an
        // assembly-backed policy, not a generic identity default.
        constexpr auto cAuditedParameters = std::array{
            AuditedParameterRecord{0x0006001aU,
                                   ParameterPolicy::KawamuraQuadraticPitch},
            AuditedParameterRecord{0x0006001bU, ParameterPolicy::Identity},
        };

    } // namespace

    std::optional<JAudioSoundParameterAdjustment>
    resolve_jaudio_sound_parameter_adjustment(
        std::uint32_t sound_id, std::int32_t parameter_1,
        std::int32_t parameter_2) {
        (void)parameter_2;
        const auto record = std::ranges::find(
            cAuditedParameters, sound_id, &AuditedParameterRecord::sound_id);
        if (record == cAuditedParameters.end()) {
            return std::nullopt;
        }

        switch (record->policy) {
        case ParameterPolicy::KawamuraQuadraticPitch: {
            // RMGK02 AudSoundObject::modifySe_Kawamura clamps parameter 1 to
            // 0..100, squares it, and calls movePitch(1 + 0.5*x^2, 0).
            const auto clamped = std::clamp(parameter_1, 0, 100);
            const auto normalized = static_cast<float>(clamped) / 100.0F;
            return JAudioSoundParameterAdjustment{
                .gain_multiplier = 1.0F,
                .pitch_multiplier =
                    1.0F + 0.5F * normalized * normalized,
            };
        }
        case ParameterPolicy::Identity:
            // No branch in any of the three retail modifier functions matches.
            return JAudioSoundParameterAdjustment{};
        }
        return std::nullopt;
    }

} // namespace smgpc::compat
