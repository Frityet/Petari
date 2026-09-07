#pragma once

#include <JSystem/JAudio2/JAISound.hpp>
#include <aurora/audio.hpp>
#include <aurora/j_audio_sound_archive.hpp>
#include <aurora/j_audio_stream.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::runtime {

    class DvdFileSystemService;

    // Owns the concrete PCM voices behind retail JAudio sound effects and
    // streams. Level sounds must be refreshed during every open frame;
    // one-shots and stage streams retire from their real backend lifetimes.
    class JAudioPlaybackService final {
    public:
        using ArchiveFactory =
            std::function<std::unique_ptr<aurora::audio::JAudioSoundArchive>()>;
        using StreamLoader =
            std::function<std::vector<std::uint8_t>(std::string_view)>;

        explicit JAudioPlaybackService(DvdFileSystemService &dvd);
        JAudioPlaybackService(
            ArchiveFactory archive_factory,
            StreamLoader stream_loader,
            std::unique_ptr<aurora::audio::PcmAudioMixer> mixer);
        ~JAudioPlaybackService();

        JAudioPlaybackService(const JAudioPlaybackService &) = delete;
        JAudioPlaybackService &operator=(const JAudioPlaybackService &) = delete;

        void begin_frame(std::uint64_t frame_index);
        void end_frame();

        [[nodiscard]] JAISoundHandle *start_level_sound(
            std::string_view name, std::int32_t parameter_1,
            std::int32_t parameter_2);
        [[nodiscard]] JAISoundHandle *start_sound_effect(
            std::string_view name, std::int32_t parameter_1,
            std::int32_t parameter_2);
        void stop_sound_effect(std::string_view name,
                               std::uint32_t delay_frames);
        [[nodiscard]] std::optional<std::uint32_t>
        find_sound_id(std::string_view name);
        void set_level_sound_permitted(bool permitted);
        [[nodiscard]] bool is_level_sound_permitted() const;
        [[nodiscard]] JAISoundHandle *start_stage_bgm(
            std::string_view name, bool prepared);
        [[nodiscard]] JAISoundHandle *start_stage_bgm(
            std::uint32_t sound_id, bool prepared);
        void unlock_stage_bgm();
        void stop_stage_bgm(std::uint32_t fade_frames);
        void pause_stage_bgm(bool paused);
        [[nodiscard]] bool is_stage_bgm_prepared() const;
        [[nodiscard]] bool is_stage_bgm_paused() const;
        [[nodiscard]] bool is_stage_bgm_stopping() const;
        [[nodiscard]] bool has_active_stage_bgm() const;
        [[nodiscard]] std::optional<std::uint32_t> stage_bgm_id() const;
        [[nodiscard]] std::string_view stage_bgm_name() const;
        [[nodiscard]] JAISoundHandle *stage_bgm_handle();
        [[nodiscard]] std::uint64_t stage_bgm_backend_token() const;
        [[nodiscard]] bool has_me() const;

        void reset_scene();

        [[nodiscard]] bool is_device_open() const;
        [[nodiscard]] std::size_t active_voice_count() const;
        [[nodiscard]] aurora::audio::PlaybackStats playback_stats() const;

    private:
        struct LevelVoiceEntry {
            std::string name;
            aurora::audio::JAudioPersistentSoundRecipe recipe;
            aurora::audio::VoiceToken token;
            JAISoundHandle handle;
            bool refreshed = false;
            bool releasing = false;
        };

        struct StageVoiceEntry {
            std::string name;
            aurora::audio::JAudioSoundMetadata metadata;
            aurora::audio::JAudioStreamRecipe recipe;
            aurora::audio::VoiceToken token;
            bool prepared = false;
            bool unlocked = false;
            bool host_paused = false;
            bool stopping = false;
        };

        struct SoundEffectVoiceEntry {
            std::string name;
            std::uint32_t sound_id = 0U;
            aurora::audio::VoiceToken token;
            JAISoundHandle handle;
        };

        void ensure_archive();
        void require_working_output() const;
        void retire_finished_voices();
        [[nodiscard]] JAISoundHandle *start_stage_bgm(
            aurora::audio::JAudioSoundMetadata metadata,
            std::string_view name, bool prepared);
        ArchiveFactory _archive_factory;
        StreamLoader _stream_loader;
        std::unique_ptr<aurora::audio::JAudioSoundArchive> _archive;
        std::unique_ptr<aurora::audio::PcmAudioMixer> _mixer;
        std::map<std::uint32_t, LevelVoiceEntry> _level_voices;
        std::map<std::uint32_t, aurora::audio::JAudioSoundEffectRecipe>
            _sound_effect_recipes;
        std::map<std::uint64_t, std::unique_ptr<SoundEffectVoiceEntry>>
            _sound_effect_voices;
        std::vector<std::unique_ptr<SoundEffectVoiceEntry>>
            _retired_sound_effect_voices;
        std::optional<StageVoiceEntry> _stage_voice;
        JAISoundHandle _stage_handle;
        std::uint64_t _frame_index = 0U;
        bool _frame_open = false;
        bool _level_sound_permitted = true;
    };

}  // namespace smgpc::runtime
