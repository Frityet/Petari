#include "Game/Util/SoundUtil.hpp"
#include "runtime/JAudioPlaybackService.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

int main() {
    using smgpc::runtime::JAudioPlaybackService;
    unsigned archive_requests = 0;
    unsigned stream_requests = 0;
    JAudioPlaybackService playback(
        [&]() -> std::unique_ptr<aurora::audio::JAudioSoundArchive> {
            ++archive_requests;
            throw std::logic_error("Permission queries must not load sound resources");
        },
        [&](std::string_view) -> std::vector<std::uint8_t> {
            ++stream_requests;
            throw std::logic_error("Permission queries must not load streams");
        },
        std::make_unique<aurora::audio::PcmAudioMixer>());

    assert(playback.is_trigger_sound_permitted());
    assert(playback.is_level_sound_permitted());
    assert(playback.is_sound_permitted());
    for (bool trigger : {false, true}) {
        for (bool level : {false, true}) {
            playback.set_trigger_sound_permitted(trigger);
            playback.set_level_sound_permitted(level);
            assert(playback.is_trigger_sound_permitted() == trigger);
            assert(playback.is_level_sound_permitted() == level);
            assert(playback.is_sound_permitted() == (trigger && level));
            for (auto id : {0x0006001aU, 0x00020001U, 0x00120001U}) {
                assert(playback.is_trigger_sound_permitted(id) == trigger);
                assert(playback.is_level_sound_permitted(id) == level);
            }
            // AudSystem tests the group byte, independently of section/wave.
            for (auto id : {0x00000001U, 0x000d0001U, 0x01000001U, 0x020d0020U}) {
                assert(playback.is_trigger_sound_permitted(id));
                assert(playback.is_level_sound_permitted(id));
            }
        }
    }
    playback.set_trigger_sound_permitted(false);
    playback.set_level_sound_permitted(false);
    playback.reset_scene();
    assert(playback.is_sound_permitted());
    assert(archive_requests == 0 && stream_requests == 0);
    assert(!playback.is_device_open() && playback.active_voice_count() == 0);

    const std::array<void (*)(), 6> operations{
        MR::submitTrigSE, MR::permitTrigSE, MR::submitLevelSE,
        MR::permitLevelSE, MR::submitSE, MR::permitSE};
    for (auto operation : operations) {
        bool rejected = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            rejected = true;
        }
        assert(rejected);
    }
    bool rejected = false;
    try {
        (void)MR::isPermitSE();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    assert(rejected);
    std::puts("PASS: trigger/level independence, combined permission, original exempt groups, scene reset and absent owner");
}
