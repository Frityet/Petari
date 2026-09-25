#include "JSystem/JAudio2/JAISound.hpp"
#include "JSystem/JAudio2/JAIStreamMgr.hpp"
#include <cassert>
#include <array>
#include <cstdio>
#include <cstring>

static void testStreamLifetime() {
    auto mixer = std::make_shared< aurora::audio::PcmAudioMixer >(48000);
    JAISoundHandle handle;
    {
        JAIStreamMgr manager(false);
        manager.bindNativeOutput(mixer, [](JAISoundID) {
            aurora::audio::JAudioStreamRecipe recipe;
            recipe.sample_rate = 48000;
            recipe.sample_count = 1024;
            recipe.channel_count = 1;
            recipe.voice.layers.push_back({
                .samples = std::make_shared< const std::vector< float > >(1024, 0.5f),
                .sample_rate = 48000,
            });
            return recipe;
        });
        std::array< float, 256 > output{};
        assert(manager.startSound(JAISoundID(0x02000001), &handle, nullptr));
        assert(handle.isSoundAttached() && !handle->isPrepared());
        handle->lockWhenPrepared();
        manager.calc();
        manager.mixOut();
        assert(handle->isPrepared() && !handle->isPlaying());
        assert(mixer->active_voice_count() == 0);
        mixer->render_interleaved(output);
        for (float sample : output) assert(sample == 0.0f);
        handle->unlockIfLocked();
        handle->pause(true);
        manager.mixOut();
        assert(handle->isPlaying() && mixer->active_voice_count() == 1);
        mixer->render_interleaved(output);
        for (float sample : output) assert(sample == 0.0f);
        handle->pause(false);
        manager.mixOut();
        mixer->render_interleaved(output);
        assert(output[0] > 0.0f && output[1] > 0.0f);
        handle->pause(true);
        manager.mixOut();
        mixer->render_interleaved(output);
        for (float sample : output) assert(sample == 0.0f);
        handle->pause(false);
        manager.mixOut();
        for (int i = 0; i < 8; ++i) mixer->render_interleaved(output);
        manager.calc();
        assert(!handle.isSoundAttached() && !manager.isActive());

        assert(manager.startSound(JAISoundID(0x02000002), &handle, nullptr));
        manager.mixOut();
        handle->stop(2);
        manager.calc();
        manager.mixOut();
        assert(handle.isSoundAttached());
        manager.calc();
        manager.mixOut();
        manager.calc();
        assert(!handle.isSoundAttached() && mixer->active_voice_count() == 0);

        assert(manager.startSound(JAISoundID(0x02000003), &handle, nullptr));
        manager.mixOut();
        assert(mixer->active_voice_count() == 1);
    }
    assert(!handle.isSoundAttached() && mixer->active_voice_count() == 0);
    std::puts("[pass] JAI stream preparation, lock, audible PCM, pause, EOF, fade and owner retirement");
}

int main() {
    static_assert(sizeof(JAISoundHandle) == sizeof(JAISound*));
    JAISoundID id(2, 13, 0x1234);
    assert(u32(id) == 0x020D1234 && id.getGroupID() == 13 && id.getWaveID() == 0x1234);
    JAISoundStatus_ status;
    status.init();
    status.pause(true);
    assert(status._0.value == 0x40);
    status._0.value = 0x80;
    assert(status.isMute() && !status.isPaused());
    status.setAnimationState(3);
    u8 flags = 0;
    std::memcpy(&flags, &status.mState.flags, sizeof(flags));
    assert(flags == 0x30 && status.getAnimationState() == 3);
    status.init();
    assert(status.lockWhenPrepared() == 1 && status.getState() == JAISoundStatus_::State_LOCK_PREPARE);
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_PREPARE);
    status.setReadyLocked();
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_READY);
    std::puts("[pass] original JAISoundID layout and JAISoundStatus flags/prepare transitions");
    testStreamLifetime();
}
