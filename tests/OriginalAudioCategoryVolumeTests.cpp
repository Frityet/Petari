#include "Game/AudioLib/AudParams.hpp"
#include "compat/JAudioCategoryVolumeOwnership.hpp"
#include "runtime/JAudioPlaybackService.hpp"

#include <aurora/audio.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace {
    void near(float actual, float expected) {
        assert(std::abs(actual - expected) < 0.00001F);
    }

    void test_original_controller() {
        smgpc::compat::JAudioCategoryVolumeOwnership first;
        smgpc::compat::JAudioCategoryVolumeOwnership second;
        auto &controller = first.controller();
        assert(controller.mSystem == nullptr);
        assert(controller.mCurrVolumeSet == 0 && controller.mNumVolumeSets == 2);
        assert(controller.mVolumeSetDelay == -1);
        for (std::size_t i = 0; i < 16; ++i) {
            near(first.sound_gain(static_cast<u32>(i << 16)), AudParams::scCtgVolume[0][i]);
        }
        // Each of the eight authored presets, all sixteen actual category records.
        for (int preset = 0; preset < 8; ++preset) {
            controller.setSeVolumeSetTrig(preset, 0);
            for (std::size_t i = 0; i < 16; ++i) {
                near(first.category(i).mParams.mVolume, AudParams::scCtgVolume[preset][i]);
                assert(first.category(i).mTransition.mVolume.mRemainingSteps == 0);
                near(second.category(i).mParams.mVolume, AudParams::scCtgVolume[0][i]);
            }
            controller.recoverSeVolumeSet(0);
            assert(controller.mCurrVolumeSet == 0 && controller.mNumVolumeSets == 2);
        }
        controller.setSeVolumeSetTrig(1, 4);
        for (unsigned step = 1; step <= 4; ++step) {
            first.update();
            for (std::size_t i = 0; i < 16; ++i) {
                near(first.category(i).mParams.mVolume,
                     AudParams::scCtgVolume[0][i] + (AudParams::scCtgVolume[1][i] - AudParams::scCtgVolume[0][i]) * (step / 4.0F));
            }
        }
        controller.setSeVolumeSetTrig(5, 0);
        controller.recoverSeVolumeSet(0);
        assert(controller.mCurrVolumeSet == 1);
        controller.recoverSeVolumeSet(0);
        assert(controller.mCurrVolumeSet == 0);

        controller.setSeVolumeSetLevel(4);
        assert(controller.mVolumeSetDelay == 2 && controller.mNumVolumeSets == 3);
        first.update();
        assert(controller.mVolumeSetDelay == 1);
        const float after_one = first.category(6).mParams.mVolume;
        near(after_one, 1.2F + (0.3F - 1.2F) / 60.0F);
        controller.setSeVolumeSetLevel(6);
        assert(controller.mCurrVolumeSet == 4 && controller.mNumVolumeSets == 3);
        first.update();
        const float before_recover = first.category(6).mParams.mVolume;
        first.update();
        assert(controller.mVolumeSetDelay == -1 && controller.mCurrVolumeSet == 0 && controller.mNumVolumeSets == 2);
        near(first.category(6).mParams.mVolume, before_recover + (1.2F - before_recover) / 60.0F);
        for (int i = 1; i < 60; ++i) first.update();
        assert(first.category(6).mParams.mVolume == AudParams::scCtgVolume[0][6]);
        controller.setSeVolumeSetLevel(5);
        first.reset();
        assert(controller.mVolumeSetDelay == -1 && controller.mCurrVolumeSet == 0 && controller.mNumVolumeSets == 2);
        near(first.category(6).mParams.mVolume, 1.2F);

        AudSystemVolumeController unowned(nullptr);
        bool rejected = false;
        try { unowned.setSeVolumeSetTrig(0, 0); }
        catch (const std::logic_error &) { rejected = true; }
        assert(rejected);
    }

    void test_independent_bus_gain() {
        aurora::audio::PcmAudioMixer mixer(60);
        aurora::audio::PcmVoiceSpec spec;
        spec.layers.push_back({
            .samples = std::make_shared<const std::vector<float>>(4, 0.25F),
            .sample_rate = 60,
            .loop_end = 4,
            .pan = 0.0F,
        });
        spec.bus_gain_multiplier = 1.2F;
        auto token = mixer.start_voice(spec);
        std::array<float, 2> frame;
        mixer.render_interleaved(frame);
        near(frame[0], 0.3F);
        near(frame[1], 0.0F);
        mixer.fade_out_voice(token, 4.0 / 60.0);
        assert(mixer.try_set_voice_bus_gain(token, 0.5F));
        mixer.render_interleaved(frame);
        near(frame[0], 0.125F);
        near(*mixer.voice_gain_multiplier(token), 0.75F);
        assert(mixer.try_set_voice_bus_gain(token, 2.0F));
        mixer.render_interleaved(frame);
        near(frame[0], 0.375F);
        near(*mixer.voice_gain_multiplier(token), 0.5F);
        assert(mixer.try_set_voice_bus_gain(token, 0.0F));
        mixer.render_interleaved(frame);
        near(frame[0], 0.0F);
        mixer.render_interleaved(frame);
        assert(!mixer.is_voice_active(token));
        assert(!mixer.try_set_voice_bus_gain(token, 1.0F));
        assert(!mixer.voice_bus_gain_multiplier(token).has_value());
    }

    void test_playback_owner_without_resources() {
        smgpc::runtime::JAudioPlaybackService playback(
            []() -> std::unique_ptr<aurora::audio::JAudioSoundArchive> {
                throw std::logic_error("Category state does not require a loaded sound archive");
            },
            [](std::string_view) -> std::vector<std::uint8_t> {
                throw std::logic_error("Category state does not require a loaded stream");
            }, std::make_unique<aurora::audio::PcmAudioMixer>());
        near(playback.sound_category_gain(0x6001AU), 1.2F);
        playback.set_sound_volume_setting(1, 4);
        playback.begin_frame(0);
        near(playback.sound_category_gain(0x6001AU), 0.975F);
        playback.end_frame();
        playback.recover_sound_volume_setting(0);
        near(playback.sound_category_gain(0x6001AU), 1.2F);
        playback.set_sound_volume_setting_level(5);
        playback.begin_frame(1); playback.end_frame();
        playback.begin_frame(2); playback.end_frame();
        assert(playback.sound_category_gain(0x6001AU) > 1.18F);
        playback.reset_scene();
        near(playback.sound_category_gain(0x6001AU), 1.2F);
        assert(!playback.is_device_open() && playback.active_voice_count() == 0);
    }
}

int main() {
    test_original_controller();
    test_independent_bus_gain();
    test_playback_owner_without_resources();
    std::puts("PASS: original eight-preset/sixteen-category ramps, stack/recovery/level timeout, retained owners, and independent PCM gain during stop fade");
}
