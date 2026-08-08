#pragma once

#include <JSystem/JAudio2/JAISound.hpp>
#include <aurora/audio.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "compat/JAudioLevelSoundArchive.hpp"

namespace smgpc::runtime {

    class DvdFileSystemService;

    // Owns the concrete PCM voices behind AudSoundObject-style atmosphere
    // level sounds. A level sound must be refreshed during every open frame;
    // missing a refresh starts the release envelope parsed from retail JAudio
    // data, and the handle remains attached until that voice really ends.
    class AtmosphereLevelSoundService final {
    public:
        using ArchiveFactory =
            std::function<std::unique_ptr<smgpc::compat::JAudioLevelSoundArchive>()>;

        explicit AtmosphereLevelSoundService(DvdFileSystemService &dvd);
        AtmosphereLevelSoundService(
            ArchiveFactory archive_factory,
            std::unique_ptr<aurora::audio::LoopingAudioMixer> mixer);
        ~AtmosphereLevelSoundService();

        AtmosphereLevelSoundService(const AtmosphereLevelSoundService &) = delete;
        AtmosphereLevelSoundService &operator=(const AtmosphereLevelSoundService &) = delete;

        void begin_frame(std::uint64_t frame_index);
        void end_frame();

        [[nodiscard]] JAISoundHandle *start_level_sound(
            std::string_view name, std::int32_t parameter_1,
            std::int32_t parameter_2);

        void reset_scene();

        [[nodiscard]] bool is_device_open() const;
        [[nodiscard]] std::size_t active_voice_count() const;
        [[nodiscard]] aurora::audio::PlaybackStats playback_stats() const;

    private:
        struct VoiceEntry {
            std::string name;
            smgpc::compat::JAudioLevelSoundRecipe recipe;
            aurora::audio::VoiceToken token;
            JAISoundHandle handle;
            bool refreshed = false;
            bool releasing = false;
        };

        void ensure_archive();
        void require_working_output() const;
        void retire_finished_voices();
        ArchiveFactory _archive_factory;
        std::unique_ptr<smgpc::compat::JAudioLevelSoundArchive> _archive;
        std::unique_ptr<aurora::audio::LoopingAudioMixer> _mixer;
        std::map<std::uint32_t, VoiceEntry> _voices;
        std::uint64_t _frame_index = 0U;
        bool _frame_open = false;
    };

} // namespace smgpc::runtime
