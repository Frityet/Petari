#pragma once

#include "Game/AudioLib/AudLimitedSound.hpp"
#include <array>

namespace smgpc::compat {

    // Actual original limiter records. The owning output service stops matching
    // sounds before registration, including duplicate and full-table requests.
    class JAudioLimitedSoundOwnership final {
    public:
        JAudioLimitedSoundOwnership();
        void register_sound(JAISoundID sound_id, s32 delay);
        [[nodiscard]] bool contains(JAISoundID sound_id) const;
        void update();
        void clear();

    private:
        std::array<AudLimitedSoundInfo, 2> _sounds;
    };

}  // namespace smgpc::compat
