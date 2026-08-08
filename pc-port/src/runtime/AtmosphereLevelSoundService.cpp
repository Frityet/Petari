#include "runtime/AtmosphereLevelSoundService.hpp"

#include "compat/JAudioSoundParameterSemantics.hpp"
#include "resource/Yaz0.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace smgpc::runtime {
    namespace {

        [[nodiscard]] std::unique_ptr<smgpc::compat::JAudioLevelSoundArchive>
        load_retail_archive(DvdFileSystemService &dvd) {
            const auto smr_path = dvd.find_first({
                std::filesystem::path("KrKorean") / "AudioRes" / "SMR.szs",
                std::filesystem::path("AudioRes") / "SMR.szs",
            });
            if (!smr_path.has_value()) {
                throw std::runtime_error(
                    "Atmosphere level playback requires the retail AudioRes/SMR.szs archive");
            }

            const auto compressed = dvd.read_file(smr_path->generic_string());
            auto baa = smgpc::resource::decompress_yaz0(compressed);
            const auto waves_path = smr_path->parent_path() / "Waves";
            return std::make_unique<smgpc::compat::JAudioLevelSoundArchive>(
                baa,
                [&dvd, waves_path](std::string_view archive_name) {
                    const auto name_path = std::filesystem::path(archive_name);
                    if (name_path.empty() || name_path.is_absolute() ||
                        name_path.filename() != name_path) {
                        throw std::runtime_error(
                            "WSYS wave archive name is not a plain filename");
                    }
                    return dvd.read_file((waves_path / name_path).generic_string());
                });
        }

    } // namespace

    AtmosphereLevelSoundService::AtmosphereLevelSoundService(
        DvdFileSystemService &dvd)
        : AtmosphereLevelSoundService(
              [&dvd] { return load_retail_archive(dvd); },
              std::make_unique<aurora::audio::LoopingAudioMixer>()) {
    }

    AtmosphereLevelSoundService::AtmosphereLevelSoundService(
        ArchiveFactory archive_factory,
        std::unique_ptr<aurora::audio::LoopingAudioMixer> mixer)
        : _archive_factory(std::move(archive_factory)),
          _mixer(std::move(mixer)) {
        if (!_archive_factory) {
            throw std::invalid_argument(
                "Atmosphere level playback requires a JAudio archive factory");
        }
        if (_mixer == nullptr) {
            throw std::invalid_argument(
                "Atmosphere level playback requires a concrete audio mixer");
        }
    }

    AtmosphereLevelSoundService::~AtmosphereLevelSoundService() {
        reset_scene();
    }

    void AtmosphereLevelSoundService::begin_frame(std::uint64_t frame_index) {
        if (_frame_open) {
            throw std::logic_error(
                "Atmosphere level playback frame was begun before the previous frame ended");
        }

        _frame_index = frame_index;
        _frame_open = true;
        require_working_output();
        retire_finished_voices();
        for (auto &[sound_id, voice] : _voices) {
            (void)sound_id;
            voice.refreshed = false;
        }
    }

    void AtmosphereLevelSoundService::end_frame() {
        if (!_frame_open) {
            throw std::logic_error(
                "Atmosphere level playback frame ended without a matching begin");
        }

        require_working_output();
        retire_finished_voices();
        for (auto &[sound_id, voice] : _voices) {
            (void)sound_id;
            if (voice.token && !voice.refreshed && !voice.releasing) {
                _mixer->release_voice(voice.token);
                voice.releasing = true;
            }
        }
        retire_finished_voices();
        _frame_open = false;
    }

    JAISoundHandle *AtmosphereLevelSoundService::start_level_sound(
        std::string_view name, std::int32_t parameter_1,
        std::int32_t parameter_2) {
        if (name.empty()) {
            throw std::invalid_argument(
                "Atmosphere level playback requires a nonempty sound name");
        }

        ensure_archive();
        const auto sound_id = _archive->find_sound_id(name);
        if (!sound_id.has_value()) {
            throw std::invalid_argument(
                "Atmosphere level sound is absent from the retail JAudio name table: " +
                std::string(name));
        }
        const auto adjustment =
            smgpc::compat::resolve_jaudio_sound_parameter_adjustment(
                *sound_id, parameter_1, parameter_2);
        if (!adjustment.has_value()) {
            throw std::logic_error(
                "Atmosphere level sound parameter semantics have not been proven for sound ID " +
                std::to_string(*sound_id));
        }

        // A handle is never returned unless SDL has opened and resumed a real
        // playback stream. A failed callback is detected on this path too.
        _mixer->open_default_playback();
        require_working_output();
        retire_finished_voices();

        auto existing = _voices.find(*sound_id);
        if (existing != _voices.end() && existing->second.token) {
            auto &voice = existing->second;
            if (!voice.handle.isBackendAttached(this, voice.token.value)) {
                throw std::logic_error(
                    "Atmosphere level handle detached before its backend voice ended");
            }
            if (_mixer->try_update_voice(voice.token,
                                         adjustment->gain_multiplier,
                                         adjustment->pitch_multiplier)) {
                voice.refreshed = true;
                return &voice.handle;
            }

            // The audio callback can retire a release-complete voice after
            // retire_finished_voices() but before this update. Treat that as
            // the same detach boundary JAudio reaches at sound-handle release,
            // then attach the stable logical handle to a fresh backend token.
            voice.token = {};
            voice.handle.releaseSound();
            voice.refreshed = false;
            voice.releasing = false;
        }

        if (existing == _voices.end()) {
            const auto recipe = _archive->resolve_level_sound(name);
            if (!recipe.has_value()) {
                throw std::logic_error(
                    "JAudio sound disappeared between name lookup and recipe resolution");
            }
            existing = _voices.emplace(
                *sound_id,
                VoiceEntry{
                    .name = std::string(name),
                    .recipe = *recipe,
                    .token = {},
                    .handle = {},
                    .refreshed = false,
                    .releasing = false,
                }).first;
        }

        auto &voice = existing->second;
        if (voice.name != name) {
            throw std::logic_error(
                "Two JAudio names unexpectedly resolved to one atmosphere sound ID");
        }
        auto spec = voice.recipe.voice;
        spec.gain_multiplier = adjustment->gain_multiplier;
        spec.pitch_multiplier = adjustment->pitch_multiplier;
        voice.token = _mixer->start_voice(spec);
        voice.handle.attachBackend(this, voice.token.value);
        voice.refreshed = true;
        voice.releasing = false;
        return &voice.handle;
    }

    void AtmosphereLevelSoundService::reset_scene() {
        _mixer->stop_all_voices();
        for (auto &[sound_id, voice] : _voices) {
            (void)sound_id;
            voice.token = {};
            voice.handle.releaseSound();
            voice.refreshed = false;
            voice.releasing = false;
        }
    }

    bool AtmosphereLevelSoundService::is_device_open() const {
        return _mixer->is_device_open();
    }

    std::size_t AtmosphereLevelSoundService::active_voice_count() const {
        auto count = std::size_t{0};
        for (const auto &[sound_id, voice] : _voices) {
            (void)sound_id;
            if (voice.token && _mixer->is_voice_active(voice.token)) {
                ++count;
            }
        }
        return count;
    }

    aurora::audio::PlaybackStats
    AtmosphereLevelSoundService::playback_stats() const {
        return _mixer->stats();
    }

    void AtmosphereLevelSoundService::ensure_archive() {
        if (_archive == nullptr) {
            _archive = _archive_factory();
            if (_archive == nullptr) {
                throw std::runtime_error(
                    "Atmosphere level JAudio archive factory returned no archive");
            }
        }
    }

    void AtmosphereLevelSoundService::require_working_output() const {
        const auto has_backend_voice = std::ranges::any_of(
            _voices, [this](const auto &entry) {
                return entry.second.token &&
                       _mixer->is_voice_active(entry.second.token);
            });
        if (has_backend_voice && !_mixer->is_device_open()) {
            throw std::runtime_error(
                "SDL atmosphere level playback device stopped accepting audio");
        }
    }

    void AtmosphereLevelSoundService::retire_finished_voices() {
        for (auto &[sound_id, voice] : _voices) {
            (void)sound_id;
            if (voice.token && !_mixer->is_voice_active(voice.token)) {
                voice.token = {};
                voice.handle.releaseSound();
                voice.refreshed = false;
                voice.releasing = false;
            }
        }
    }

} // namespace smgpc::runtime
