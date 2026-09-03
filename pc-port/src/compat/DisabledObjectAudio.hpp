#pragma once

#include <JSystem/JAudio2/JAISound.hpp>
#include <cstddef>

namespace aurora::audio {
// Explicit policy for the entire JAU/Aud object-sound subsystem. BGM uses a
// separate existing PCM backend. This service never fabricates a JAISound.
class DisabledObjectAudio final {
public:
    [[nodiscard]] static constexpr bool enabled() noexcept { return false; }
    [[nodiscard]] static JAISoundHandle* request() noexcept;
    [[nodiscard]] static std::size_t declined_requests() noexcept;
};
}
