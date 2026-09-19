#include "compat/JAudioLimitedSoundOwnership.hpp"

namespace smgpc::compat {

    JAudioLimitedSoundOwnership::JAudioLimitedSoundOwnership() {
        clear();
    }

    void JAudioLimitedSoundOwnership::register_sound(JAISoundID sound_id, s32 delay) {
        // AudSystem::registerLimitedSound does not refresh existing entries or
        // evict a sound when both of its original slots are occupied.
        if (contains(sound_id)) return;
        for (auto& sound : _sounds) {
            if (sound.isFree()) {
                sound.set(sound_id, delay);
                break;
            }
        }
    }

    bool JAudioLimitedSoundOwnership::contains(JAISoundID sound_id) const {
        for (const auto& sound : _sounds) {
            if (sound.mSoundID == sound_id) return true;
        }
        return false;
    }

    void JAudioLimitedSoundOwnership::update() {
        for (auto& sound : _sounds) sound.update();
    }

    void JAudioLimitedSoundOwnership::clear() {
        for (auto& sound : _sounds) sound.init();
    }

}  // namespace smgpc::compat
