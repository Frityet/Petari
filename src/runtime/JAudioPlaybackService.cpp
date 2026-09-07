#include "runtime/JAudioPlaybackService.hpp"

#include "compat/JAudioSoundParameterSemantics.hpp"
#include "resource/Yaz0.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace smgpc::runtime {
    namespace {

        [[nodiscard]] std::unique_ptr<aurora::audio::JAudioSoundArchive>
        load_retail_archive(DvdFileSystemService &dvd) {
            const auto smr_path = dvd.find_first({
                std::filesystem::path("KrKorean") / "AudioRes" / "SMR.szs",
                std::filesystem::path("AudioRes") / "SMR.szs",
            });
            if (!smr_path.has_value()) {
                throw std::runtime_error(
                    "JAudio playback requires the retail AudioRes/SMR.szs archive");
            }

            const auto compressed = dvd.read_file(smr_path->generic_string());
            auto baa = smgpc::resource::decompress_yaz0(compressed);
            const auto localized_waves_path = smr_path->parent_path() / "Waves";
            return std::make_unique<aurora::audio::JAudioSoundArchive>(
                baa,
                [&dvd, localized_waves_path](std::string_view archive_name) {
                    const auto name_path = std::filesystem::path(archive_name);
                    if (name_path.empty() || name_path.is_absolute() ||
                        name_path.filename() != name_path) {
                        throw std::runtime_error(
                            "WSYS wave archive name is not a plain filename");
                    }
                    const auto retail_path = dvd.find_first({
                        localized_waves_path / name_path,
                        std::filesystem::path("AudioRes") / "Waves" / name_path,
                    });
                    if (!retail_path.has_value()) {
                        throw std::runtime_error(
                            "Retail JAudio wave archive is absent from localized/base AudioRes overlays: " +
                            std::string(archive_name));
                    }
                    return dvd.read_file(retail_path->generic_string());
                });
        }

        [[nodiscard]] std::vector<std::uint8_t>
        load_retail_stream(DvdFileSystemService &dvd, std::string_view path) {
            auto stream_path = std::filesystem::path(path);
            if (stream_path.empty()) {
                throw std::invalid_argument(
                    "JAudio stream playback requires a retail stream path");
            }
            if (stream_path.is_absolute()) {
                stream_path = stream_path.relative_path();
            }
            if (std::ranges::any_of(stream_path, [](const auto &component) {
                    return component == "..";
                })) {
                throw std::runtime_error(
                    "BST stream path escapes the retail disc root");
            }
            return dvd.read_file(stream_path.generic_string());
        }

    }  // namespace

    JAudioPlaybackService::JAudioPlaybackService(
        DvdFileSystemService &dvd)
        : JAudioPlaybackService(
              [&dvd] { return load_retail_archive(dvd); },
              [&dvd](std::string_view path) {
                  return load_retail_stream(dvd, path);
              },
              std::make_unique<aurora::audio::PcmAudioMixer>()) {
    }

    JAudioPlaybackService::JAudioPlaybackService(
        ArchiveFactory archive_factory,
        StreamLoader stream_loader,
        std::unique_ptr<aurora::audio::PcmAudioMixer> mixer)
        : _archive_factory(std::move(archive_factory)),
          _stream_loader(std::move(stream_loader)),
          _mixer(std::move(mixer)) {
        if (!_archive_factory) {
            throw std::invalid_argument(
                "JAudio playback requires an archive factory");
        }
        if (_mixer == nullptr) {
            throw std::invalid_argument(
                "JAudio playback requires a concrete audio mixer");
        }
        if (!_stream_loader) {
            throw std::invalid_argument(
                "JAudio playback requires a retail stream loader");
        }
    }

    JAudioPlaybackService::~JAudioPlaybackService() {
        reset_scene();
    }

    void JAudioPlaybackService::begin_frame(std::uint64_t frame_index) {
        if (_frame_open) {
            throw std::logic_error(
                "JAudio playback frame was begun before the previous frame ended");
        }

        _frame_index = frame_index;
        _frame_open = true;
        require_working_output();
        retire_finished_voices();
        for (auto &[sound_id, voice] : _level_voices) {
            (void)sound_id;
            voice.refreshed = false;
        }
    }

    void JAudioPlaybackService::end_frame() {
        if (!_frame_open) {
            throw std::logic_error(
                "JAudio playback frame ended without a matching begin");
        }

        require_working_output();
        retire_finished_voices();
        for (auto &[sound_id, voice] : _level_voices) {
            (void)sound_id;
            if (voice.token && !voice.refreshed && !voice.releasing) {
                _mixer->release_voice(voice.token);
                voice.releasing = true;
            }
        }
        retire_finished_voices();
        _frame_open = false;
    }

    JAISoundHandle *JAudioPlaybackService::start_level_sound(
        std::string_view name, std::int32_t parameter_1,
        std::int32_t parameter_2) {
        if (name.empty()) {
            throw std::invalid_argument(
                "JAudio level playback requires a nonempty sound name");
        }
        if (!_level_sound_permitted) {
            // AudSystem::_82C rejects level-sound allocation while submitted.
            // This is an exact retail absence, not a logical playback event.
            return nullptr;
        }

        ensure_archive();
        const auto sound_id = _archive->find_sound_id(name);
        if (!sound_id.has_value()) {
            throw std::invalid_argument(
                "Level sound is absent from the retail JAudio name table: " +
                std::string(name));
        }
        const auto adjustment =
            smgpc::compat::resolve_jaudio_sound_parameter_adjustment(
                *sound_id, parameter_1, parameter_2);
        if (!adjustment.has_value()) {
            throw std::logic_error(
                "Level-sound parameter semantics have not been proven for sound ID " +
                std::to_string(*sound_id));
        }

        // A handle is never returned unless SDL has opened and resumed a real
        // playback stream. A failed callback is detected on this path too.
        _mixer->open_default_playback();
        require_working_output();
        retire_finished_voices();

        auto existing = _level_voices.find(*sound_id);
        if (existing != _level_voices.end() && existing->second.token) {
            auto &voice = existing->second;
            if (!voice.handle.isBackendAttached(this, voice.token.value)) {
                throw std::logic_error(
                    "JAudio level handle detached before its backend voice ended");
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

        if (existing == _level_voices.end()) {
            const auto recipe = _archive->resolve_persistent_sound(name);
            if (!recipe.has_value()) {
                throw std::logic_error(
                    "JAudio sound disappeared between name lookup and recipe resolution");
            }
            existing = _level_voices.emplace(
                                        *sound_id,
                                        LevelVoiceEntry{
                                            .name = std::string(name),
                                            .recipe = *recipe,
                                            .token = {},
                                            .handle = {},
                                            .refreshed = false,
                                            .releasing = false,
                                        })
                           .first;
        }

        auto &voice = existing->second;
        if (voice.name != name) {
            throw std::logic_error(
                "Two JAudio names unexpectedly resolved to one level-sound ID");
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

    JAISoundHandle *JAudioPlaybackService::start_sound_effect(
        std::string_view name, std::int32_t parameter_1,
        std::int32_t parameter_2) {
        if (name.empty()) {
            throw std::invalid_argument(
                "JAudio sound-effect playback requires a nonempty sound name");
        }
        if (parameter_1 != -1) {
            throw std::logic_error(
                "Parameterized JAudio one-shot semantics are unavailable for " +
                std::string(name) + " (parameter 1=" +
                std::to_string(parameter_1) + ", parameter 2=" +
                std::to_string(parameter_2) + ")");
        }

        ensure_archive();
        const auto sound_id = _archive->find_sound_id(name);
        if (!sound_id.has_value()) {
            throw std::invalid_argument(
                "Sound effect is absent from the retail JAudio name table: " +
                std::string(name));
        }
        auto recipe = _sound_effect_recipes.find(*sound_id);
        if (recipe == _sound_effect_recipes.end()) {
            const auto resolved = _archive->resolve_sound_effect(name);
            if (!resolved.has_value()) {
                throw std::logic_error(
                    "JAudio sound disappeared between name lookup and recipe resolution");
            }
            recipe = _sound_effect_recipes.emplace(*sound_id, *resolved).first;
        }

        _mixer->open_default_playback();
        require_working_output();
        retire_finished_voices();
        const auto token = _mixer->start_voice(recipe->second.voice);
        auto voice = std::make_unique<SoundEffectVoiceEntry>();
        voice->name = std::string(name);
        voice->sound_id = *sound_id;
        voice->token = token;
        voice->handle.attachBackend(this, token.value);
        auto *handle = &voice->handle;
        const auto [position, inserted] =
            _sound_effect_voices.emplace(token.value, std::move(voice));
        (void)position;
        if (!inserted) {
            _mixer->stop_voice(token);
            throw std::logic_error(
                "JAudio mixer reused an active backend token");
        }
        return handle;
    }

    void JAudioPlaybackService::stop_sound_effect(
        std::string_view name, std::uint32_t delay_frames) {
        if (name.empty()) {
            throw std::invalid_argument(
                "Stopping a JAudio sound effect requires a nonempty sound name");
        }
        ensure_archive();
        const auto sound_id = _archive->find_sound_id(name);
        if (!sound_id.has_value()) {
            throw std::invalid_argument(
                "Sound effect is absent from the retail JAudio name table: " +
                std::string(name));
        }
        retire_finished_voices();
        for (auto &[token_value, voice] : _sound_effect_voices) {
            (void)token_value;
            if (voice->sound_id != *sound_id) {
                continue;
            }
            if (delay_frames == 0U) {
                _mixer->stop_voice(voice->token);
            } else {
                _mixer->fade_out_voice(
                    voice->token,
                    static_cast<double>(delay_frames) / 60.0);
            }
        }
        retire_finished_voices();
    }

    std::optional<std::uint32_t> JAudioPlaybackService::find_sound_id(
        std::string_view name) {
        if (name.empty()) {
            return std::nullopt;
        }
        ensure_archive();
        return _archive->find_sound_id(name);
    }

    void JAudioPlaybackService::set_level_sound_permitted(bool permitted) {
        _level_sound_permitted = permitted;
    }

    bool JAudioPlaybackService::is_level_sound_permitted() const {
        return _level_sound_permitted;
    }

    JAISoundHandle *JAudioPlaybackService::start_stage_bgm(
        std::string_view name, bool prepared) {
        if (name.empty()) {
            throw std::invalid_argument(
                "JAudio stage BGM playback requires a nonempty sound name");
        }
        ensure_archive();
        const auto metadata = _archive->resolve_sound(name);
        if (!metadata.has_value()) {
            throw std::invalid_argument(
                "Stage BGM is absent from the retail JAudio name table: " +
                std::string(name));
        }
        return start_stage_bgm(*metadata, name, prepared);
    }

    JAISoundHandle *JAudioPlaybackService::start_stage_bgm(
        std::uint32_t sound_id, bool prepared) {
        ensure_archive();
        return start_stage_bgm(_archive->resolve_sound(sound_id), {}, prepared);
    }

    JAISoundHandle *JAudioPlaybackService::start_stage_bgm(
        aurora::audio::JAudioSoundMetadata metadata,
        std::string_view name, bool prepared) {
        if (metadata.kind != aurora::audio::JAudioSoundKind::Stream) {
            throw std::logic_error(
                "The requested stage BGM is not a retail JAudio stream");
        }
        if (metadata.stream_path.empty()) {
            throw std::logic_error(
                "The retail JAudio stream has no concrete disc path");
        }

        auto recipe = aurora::audio::decode_jaudio_stream(
            _stream_loader(metadata.stream_path), metadata.channel_control);
        recipe.voice.gain_multiplier =
            static_cast<float>(metadata.volume) / 255.0F;

        _mixer->open_default_playback();
        require_working_output();

        if (_stage_voice.has_value()) {
            _mixer->stop_voice(_stage_voice->token);
            _stage_voice.reset();
            _stage_handle.releaseSound();
        }

        const auto token = _mixer->start_voice(recipe.voice);
        try {
            if (prepared) {
                _mixer->set_voice_paused(token, true);
            }
            _stage_handle.attachBackend(this, token.value);
            _stage_voice = StageVoiceEntry{
                .name = std::string(name),
                .metadata = std::move(metadata),
                .recipe = std::move(recipe),
                .token = token,
                .prepared = prepared,
                .unlocked = !prepared,
                .host_paused = false,
                .stopping = false,
            };
        } catch (...) {
            _mixer->stop_voice(token);
            _stage_handle.releaseSound();
            throw;
        }
        return &_stage_handle;
    }

    void JAudioPlaybackService::unlock_stage_bgm() {
        retire_finished_voices();
        if (!_stage_voice.has_value()) {
            throw std::logic_error(
                "Cannot unlock stage BGM without a concrete backend voice");
        }
        if (_stage_voice->prepared && !_stage_voice->unlocked) {
            _stage_voice->unlocked = true;
            _mixer->set_voice_paused(_stage_voice->token,
                                     _stage_voice->host_paused);
        }
    }

    void JAudioPlaybackService::stop_stage_bgm(std::uint32_t fade_frames) {
        retire_finished_voices();
        if (!_stage_voice.has_value()) {
            return;
        }
        if (fade_frames == 0U) {
            _mixer->stop_voice(_stage_voice->token);
            _stage_voice.reset();
            _stage_handle.releaseSound();
            return;
        }
        // A prepared or explicitly paused host voice cannot advance its gain
        // ramp. Release the mixer pause as the host mechanism for preserving
        // JAudio's stop/fade retirement lifecycle, including a stop issued
        // before the first unlock.
        _mixer->set_voice_paused(_stage_voice->token, false);
        _stage_voice->unlocked = true;
        _stage_voice->host_paused = false;
        _mixer->fade_out_voice(
            _stage_voice->token, static_cast<double>(fade_frames) / 60.0);
        _stage_voice->stopping = true;
    }

    void JAudioPlaybackService::pause_stage_bgm(bool paused) {
        retire_finished_voices();
        if (!_stage_voice.has_value()) {
            throw std::logic_error(
                "Cannot change stage-BGM pause state without a concrete backend voice");
        }
        if (_stage_voice->stopping) {
            throw std::logic_error(
                "Cannot change stage-BGM pause state while its concrete voice is stopping");
        }
        _stage_voice->host_paused = paused;
        const auto preparation_locked =
            _stage_voice->prepared && !_stage_voice->unlocked;
        _mixer->set_voice_paused(_stage_voice->token,
                                 paused || preparation_locked);
    }

    bool JAudioPlaybackService::is_stage_bgm_prepared() const {
        return _stage_voice.has_value() && _stage_voice->prepared &&
               !_stage_voice->unlocked &&
               _mixer->is_voice_active(_stage_voice->token);
    }

    bool JAudioPlaybackService::is_stage_bgm_paused() const {
        if (!has_active_stage_bgm()) {
            return false;
        }
        const auto paused = _mixer->voice_paused(_stage_voice->token);
        if (!paused.has_value()) {
            throw std::logic_error(
                "Active stage-BGM token disappeared during its pause query");
        }
        return *paused;
    }

    bool JAudioPlaybackService::is_stage_bgm_stopping() const {
        return has_active_stage_bgm() && _stage_voice->stopping;
    }

    bool JAudioPlaybackService::has_active_stage_bgm() const {
        return _stage_voice.has_value() &&
               _mixer->is_voice_active(_stage_voice->token);
    }

    std::optional<std::uint32_t> JAudioPlaybackService::stage_bgm_id() const {
        if (!has_active_stage_bgm()) {
            return std::nullopt;
        }
        return _stage_voice->metadata.sound_id;
    }

    std::string_view JAudioPlaybackService::stage_bgm_name() const {
        if (!has_active_stage_bgm()) {
            return {};
        }
        return _stage_voice->name;
    }

    JAISoundHandle *JAudioPlaybackService::stage_bgm_handle() {
        if (!has_active_stage_bgm()) {
            return nullptr;
        }
        if (!_stage_handle.isBackendAttached(this, _stage_voice->token.value)) {
            throw std::logic_error(
                "Stage-BGM handle is detached from its concrete backend voice");
        }
        return &_stage_handle;
    }

    std::uint64_t JAudioPlaybackService::stage_bgm_backend_token() const {
        if (!has_active_stage_bgm()) {
            return 0U;
        }
        return _stage_voice->token.value;
    }

    bool JAudioPlaybackService::has_me() const {
        return false;
    }

    void JAudioPlaybackService::reset_scene() {
        _mixer->stop_all_voices();
        for (auto &[sound_id, voice] : _level_voices) {
            (void)sound_id;
            voice.token = {};
            voice.handle.releaseSound();
            voice.refreshed = false;
            voice.releasing = false;
        }
        for (auto &[token, voice] : _sound_effect_voices) {
            (void)token;
            voice->token = {};
            voice->handle.releaseSound();
        }
        _sound_effect_voices.clear();
        _retired_sound_effect_voices.clear();
        _level_sound_permitted = true;
        _stage_voice.reset();
        _stage_handle.releaseSound();
    }

    bool JAudioPlaybackService::is_device_open() const {
        return _mixer->is_device_open();
    }

    std::size_t JAudioPlaybackService::active_voice_count() const {
        auto count = std::size_t{0};
        for (const auto &[sound_id, voice] : _level_voices) {
            (void)sound_id;
            if (voice.token && _mixer->is_voice_active(voice.token)) {
                ++count;
            }
        }
        for (const auto &[token, voice] : _sound_effect_voices) {
            (void)token;
            if (voice->token && _mixer->is_voice_active(voice->token)) {
                ++count;
            }
        }
        if (_stage_voice.has_value() &&
            _mixer->is_voice_active(_stage_voice->token)) {
            ++count;
        }
        return count;
    }

    aurora::audio::PlaybackStats
    JAudioPlaybackService::playback_stats() const {
        return _mixer->stats();
    }

    void JAudioPlaybackService::ensure_archive() {
        if (_archive == nullptr) {
            _archive = _archive_factory();
            if (_archive == nullptr) {
                throw std::runtime_error(
                    "JAudio archive factory returned no archive");
            }
        }
    }

    void JAudioPlaybackService::require_working_output() const {
        const auto has_backend_voice = std::ranges::any_of(
            _level_voices, [this](const auto &entry) {
                return entry.second.token &&
                       _mixer->is_voice_active(entry.second.token);
            });
        const auto has_stage_voice = _stage_voice.has_value() &&
                                     _stage_voice->token &&
                                     _mixer->is_voice_active(_stage_voice->token);
        const auto has_sound_effect_voice = std::ranges::any_of(
            _sound_effect_voices, [this](const auto &entry) {
                return entry.second->token &&
                       _mixer->is_voice_active(entry.second->token);
            });
        if ((has_backend_voice || has_stage_voice || has_sound_effect_voice) &&
            !_mixer->is_device_open()) {
            throw std::runtime_error(
                "SDL JAudio playback device stopped accepting mixed audio");
        }
    }

    void JAudioPlaybackService::retire_finished_voices() {
        for (auto &[sound_id, voice] : _level_voices) {
            (void)sound_id;
            if (voice.token && !_mixer->is_voice_active(voice.token)) {
                voice.token = {};
                voice.handle.releaseSound();
                voice.refreshed = false;
                voice.releasing = false;
            }
        }
        for (auto voice = _sound_effect_voices.begin();
             voice != _sound_effect_voices.end();) {
            if (voice->second->token &&
                _mixer->is_voice_active(voice->second->token)) {
                ++voice;
                continue;
            }
            voice->second->token = {};
            voice->second->handle.releaseSound();
            _retired_sound_effect_voices.push_back(
                std::move(voice->second));
            voice = _sound_effect_voices.erase(voice);
        }
        if (_stage_voice.has_value() && _stage_voice->token &&
            !_mixer->is_voice_active(_stage_voice->token)) {
            _stage_voice.reset();
            _stage_handle.releaseSound();
        }
    }

}  // namespace smgpc::runtime
