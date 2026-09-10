#pragma once

#include <JSystem/JAudio2/JAISound.hpp>
#include <aurora/audio.hpp>
#include <aurora/j_audio_sound_archive.hpp>
#include <aurora/j_audio_stream.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::compat { class JAudioCategoryVolumeOwnership; class NativePcmSound; class JaiStreamPlayback; }

namespace smgpc::runtime {

    class DvdFileSystemService;

    enum class BgmLane : std::size_t { Stage, Sub };

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
        void set_trigger_sound_permitted(bool permitted);
        [[nodiscard]] bool is_trigger_sound_permitted() const;
        [[nodiscard]] bool is_trigger_sound_permitted(std::uint32_t sound_id) const;
        void set_level_sound_permitted(bool permitted);
        [[nodiscard]] bool is_level_sound_permitted() const;
        [[nodiscard]] bool is_level_sound_permitted(std::uint32_t sound_id) const;
        [[nodiscard]] bool is_sound_permitted() const;
        void set_sound_volume_setting(std::int32_t volume_set, std::uint32_t steps);
        void recover_sound_volume_setting(std::uint32_t steps);
        void set_sound_volume_setting_level(std::int32_t volume_set);
        [[nodiscard]] float sound_category_gain(std::uint32_t sound_id) const;
        [[nodiscard]] JAISoundHandle *start_bgm(BgmLane lane,
            std::string_view name, bool prepared);
        [[nodiscard]] JAISoundHandle *start_bgm(BgmLane lane,
            std::uint32_t sound_id, bool prepared);
        void unlock_bgm(BgmLane lane);
        void stop_bgm(BgmLane lane, std::uint32_t fade_frames);
        void pause_bgm(BgmLane lane, bool paused);
        [[nodiscard]] bool is_bgm_prepared(BgmLane lane) const;
        [[nodiscard]] bool is_bgm_paused(BgmLane lane) const;
        [[nodiscard]] bool is_bgm_stopping(BgmLane lane) const;
        [[nodiscard]] bool has_active_bgm(BgmLane lane) const;
        [[nodiscard]] std::optional<std::uint32_t> bgm_id(BgmLane lane) const;
        [[nodiscard]] std::string_view bgm_name(BgmLane lane) const;
        [[nodiscard]] JAISoundHandle *bgm_handle(BgmLane lane);
        [[nodiscard]] std::uint64_t bgm_backend_token(BgmLane lane) const;
        void set_bgm_bus_gain(BgmLane lane, float volume);
        void bind_bgm_handle(BgmLane lane, JAISoundHandle& handle);
        [[nodiscard]] std::uint64_t sound_backend_token(const JAISound*) const;
        [[nodiscard]] bool owns_sound(const JAISound*) const;
        [[nodiscard]] bool has_me() const;

        void reset_scene();

        [[nodiscard]] bool is_device_open() const;
        [[nodiscard]] std::size_t active_voice_count() const;
        [[nodiscard]] aurora::audio::PlaybackStats playback_stats() const;

    private:
        struct LevelVoiceEntry {
            std::string name;
            aurora::audio::JAudioPersistentSoundRecipe recipe;
            JAISoundHandle handle;
            std::unique_ptr<compat::NativePcmSound> sound;
        };
        struct BgmVoiceEntry {
            std::string name;
            aurora::audio::JAudioSoundMetadata metadata;
            JAISound* sound = nullptr; // Observed only through the original manager's live list.
        };
        struct SoundEffectVoiceEntry {
            std::string name;
            std::uint32_t sound_id = 0U;
            JAISoundHandle handle;
            std::unique_ptr<compat::NativePcmSound> sound;
        };
        [[nodiscard]] JAISound* bgm_sound(BgmLane lane) const;
        void apply_category_gains();
        void ensure_archive();
        void require_working_output() const;
        void retire_finished_voices();
        [[nodiscard]] JAISoundHandle *start_bgm(BgmLane lane,
            aurora::audio::JAudioSoundMetadata metadata,
            std::string_view name, bool prepared);
        ArchiveFactory _archive_factory;
        StreamLoader _stream_loader;
        std::unique_ptr<aurora::audio::JAudioSoundArchive> _archive;
        std::unique_ptr<aurora::audio::PcmAudioMixer> _mixer;
        std::unique_ptr<compat::JAudioCategoryVolumeOwnership> _category_volume;
        std::unique_ptr<compat::JaiStreamPlayback> _stream_playback;
        std::map<std::uint32_t, LevelVoiceEntry> _level_voices;
        std::map<std::uint32_t, aurora::audio::JAudioSoundEffectRecipe>
            _sound_effect_recipes;
        std::map<std::uint64_t, std::unique_ptr<SoundEffectVoiceEntry>>
            _sound_effect_voices;
        std::vector<std::unique_ptr<SoundEffectVoiceEntry>>
            _retired_sound_effect_voices;
        std::array<std::optional<BgmVoiceEntry>, 2> _bgm_voices;
        std::array<JAISoundHandle, 2> _bgm_handles;
        std::uint64_t _frame_index = 0U;
        bool _frame_open = false;
        bool _trigger_sound_permitted = true;
        bool _level_sound_permitted = true;
    };

}  // namespace smgpc::runtime
