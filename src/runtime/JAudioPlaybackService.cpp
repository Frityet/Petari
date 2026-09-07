#include <aurora/exception.hpp>
#include <aurora/allocation.hpp>
#include "runtime/JAudioPlaybackService.hpp"

#include "compat/JAudioSoundParameterSemantics.hpp"
#include "compat/JAudioCategoryVolumeOwnership.hpp"
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
                aurora::throw_host_exception<std::runtime_error>(
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
                        aurora::throw_host_exception<std::runtime_error>(
                            "WSYS wave archive name is not a plain filename");
                    }
                    const auto retail_path = dvd.find_first({
                        localized_waves_path / name_path,
                        std::filesystem::path("AudioRes") / "Waves" / name_path,
                    });
                    if (!retail_path.has_value()) {
                        aurora::throw_host_exception<std::runtime_error>(
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
                aurora::throw_host_exception<std::invalid_argument>(
                    "JAudio stream playback requires a retail stream path");
            }
            if (stream_path.is_absolute()) {
                stream_path = stream_path.relative_path();
            }
            if (std::ranges::any_of(stream_path, [](const auto &component) {
                    return component == "..";
                })) {
                aurora::throw_host_exception<std::runtime_error>(
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
          _mixer(std::move(mixer)),
          _category_volume(std::make_unique<compat::JAudioCategoryVolumeOwnership>()) {
        if (!_archive_factory) {
            aurora::throw_host_exception<std::invalid_argument>(
                "JAudio playback requires an archive factory");
        }
        if (_mixer == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>(
                "JAudio playback requires a concrete audio mixer");
        }
        if (!_stream_loader) {
            aurora::throw_host_exception<std::invalid_argument>(
                "JAudio playback requires a retail stream loader");
        }
    }

    JAudioPlaybackService::~JAudioPlaybackService() {
        reset_scene();
    }

    void JAudioPlaybackService::begin_frame(std::uint64_t frame_index) {
        if (_frame_open) {
            aurora::throw_host_exception<std::logic_error>(
                "JAudio playback frame was begun before the previous frame ended");
        }

        _category_volume->update();
        apply_category_gains();
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
            aurora::throw_host_exception<std::logic_error>(
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
        const auto host_allocations = aurora::allocation::HostAllocationScope{};
        if (name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "JAudio level playback requires a nonempty sound name");
        }
        ensure_archive();
        const auto sound_id = _archive->find_sound_id(name);
        if (!sound_id.has_value()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Level sound is absent from the retail JAudio name table: " +
                std::string(name));
        }
        if (!is_level_sound_permitted(*sound_id)) {
            return nullptr;
        }
        const auto adjustment =
            smgpc::compat::resolve_jaudio_sound_parameter_adjustment(
                *sound_id, parameter_1, parameter_2);
        if (!adjustment.has_value()) {
            aurora::throw_host_exception<std::logic_error>(
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
                aurora::throw_host_exception<std::logic_error>(
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
                aurora::throw_host_exception<std::logic_error>(
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
            aurora::throw_host_exception<std::logic_error>(
                "Two JAudio names unexpectedly resolved to one level-sound ID");
        }
        auto spec = voice.recipe.voice;
        spec.gain_multiplier = adjustment->gain_multiplier;
        spec.pitch_multiplier = adjustment->pitch_multiplier;
        spec.bus_gain_multiplier = sound_category_gain(*sound_id);
        voice.token = _mixer->start_voice(spec);
        voice.handle.attachBackend(this, voice.token.value);
        voice.refreshed = true;
        voice.releasing = false;
        return &voice.handle;
    }

    JAISoundHandle *JAudioPlaybackService::start_sound_effect(
        std::string_view name, std::int32_t parameter_1,
        std::int32_t parameter_2) {
        const auto host_allocations = aurora::allocation::HostAllocationScope{};
        if (name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "JAudio sound-effect playback requires a nonempty sound name");
        }

        ensure_archive();
        const auto sound_id = _archive->find_sound_id(name);
        if (!sound_id.has_value()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Sound effect is absent from the retail JAudio name table: " +
                std::string(name));
        }
        if (!is_trigger_sound_permitted(*sound_id)) {
            return nullptr;
        }
        if (parameter_1 != -1) {
            aurora::throw_host_exception<std::logic_error>(
                "Parameterized JAudio one-shot semantics are unavailable for " +
                std::string(name) + " (parameter 1=" +
                std::to_string(parameter_1) + ", parameter 2=" +
                std::to_string(parameter_2) + ")");
        }

        auto recipe = _sound_effect_recipes.find(*sound_id);
        if (recipe == _sound_effect_recipes.end()) {
            const auto resolved = _archive->resolve_sound_effect(name);
            if (!resolved.has_value()) {
                aurora::throw_host_exception<std::logic_error>(
                    "JAudio sound disappeared between name lookup and recipe resolution");
            }
            recipe = _sound_effect_recipes.emplace(*sound_id, *resolved).first;
        }

        _mixer->open_default_playback();
        require_working_output();
        retire_finished_voices();
        auto spec = recipe->second.voice;
        spec.bus_gain_multiplier = sound_category_gain(*sound_id);
        const auto token = _mixer->start_voice(spec);
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
            aurora::throw_host_exception<std::logic_error>(
                "JAudio mixer reused an active backend token");
        }
        return handle;
    }

    void JAudioPlaybackService::stop_sound_effect(
        std::string_view name, std::uint32_t delay_frames) {
        if (name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Stopping a JAudio sound effect requires a nonempty sound name");
        }
        ensure_archive();
        const auto sound_id = _archive->find_sound_id(name);
        if (!sound_id.has_value()) {
            aurora::throw_host_exception<std::invalid_argument>(
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

    void JAudioPlaybackService::set_trigger_sound_permitted(bool permitted) {
        _trigger_sound_permitted = permitted;
    }

    bool JAudioPlaybackService::is_trigger_sound_permitted() const {
        return _trigger_sound_permitted;
    }

    bool JAudioPlaybackService::is_trigger_sound_permitted(std::uint32_t sound_id) const {
        const auto group = JAISoundID(sound_id).getGroupID();
        // AudSystem::_82B exempts the system and HOME-menu groups.
        return _trigger_sound_permitted || group == 0U || group == 0xDU;
    }

    void JAudioPlaybackService::set_level_sound_permitted(bool permitted) {
        _level_sound_permitted = permitted;
    }

    bool JAudioPlaybackService::is_level_sound_permitted() const {
        return _level_sound_permitted;
    }

    bool JAudioPlaybackService::is_level_sound_permitted(std::uint32_t sound_id) const {
        const auto group = JAISoundID(sound_id).getGroupID();
        // AudSystem::_82C has the same exemptions, independently of _82B.
        return _level_sound_permitted || group == 0U || group == 0xDU;
    }

    bool JAudioPlaybackService::is_sound_permitted() const {
        return _trigger_sound_permitted && _level_sound_permitted;
    }

    void JAudioPlaybackService::set_sound_volume_setting(std::int32_t volume_set, std::uint32_t steps) {
        _category_volume->controller().setSeVolumeSetTrig(volume_set, steps);
        apply_category_gains();
    }

    void JAudioPlaybackService::recover_sound_volume_setting(std::uint32_t steps) {
        _category_volume->controller().recoverSeVolumeSet(steps);
        apply_category_gains();
    }

    void JAudioPlaybackService::set_sound_volume_setting_level(std::int32_t volume_set) {
        _category_volume->controller().setSeVolumeSetLevel(volume_set);
    }

    float JAudioPlaybackService::sound_category_gain(std::uint32_t sound_id) const {
        return _category_volume->sound_gain(sound_id);
    }

    void JAudioPlaybackService::apply_category_gains() {
        for (const auto &[sound_id, voice] : _level_voices) {
            (void)_mixer->try_set_voice_bus_gain(voice.token, sound_category_gain(sound_id));
        }
        for (const auto &[token, voice] : _sound_effect_voices) {
            (void)token;
            (void)_mixer->try_set_voice_bus_gain(voice->token, sound_category_gain(voice->sound_id));
        }
    }

    JAISoundHandle *JAudioPlaybackService::start_bgm(BgmLane lane,
        std::string_view name, bool prepared) {
        if (name.empty()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "JAudio BGM playback requires a nonempty sound name");
        }
        ensure_archive();
        const auto metadata = _archive->resolve_sound(name);
        if (!metadata.has_value()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "BGM is absent from the retail JAudio name table: " +
                std::string(name));
        }
        return start_bgm(lane, *metadata, name, prepared);
    }

    JAISoundHandle *JAudioPlaybackService::start_bgm(BgmLane lane,
        std::uint32_t sound_id, bool prepared) {
        ensure_archive();
        return start_bgm(lane, _archive->resolve_sound(sound_id), {}, prepared);
    }

    JAISoundHandle *JAudioPlaybackService::start_bgm(BgmLane lane,
        aurora::audio::JAudioSoundMetadata metadata,
        std::string_view name, bool prepared) {
        const auto host_allocations = aurora::allocation::HostAllocationScope{};
        if (metadata.kind == aurora::audio::JAudioSoundKind::Sequence || (metadata.sound_id & 0x10000U) != 0) {
            // This backend has no section-one JAS sequencer yet. A disabled
            // decoder produces no voice, matching an unattached JAudio start.
            stop_bgm(lane, 0);
            return nullptr;
        }
        if (metadata.kind != aurora::audio::JAudioSoundKind::Stream) {
            aurora::throw_host_exception<std::invalid_argument>(
                "BGM playback requires a retail stream or sequence ID");
        }
        if (metadata.stream_path.empty()) {
            aurora::throw_host_exception<std::logic_error>(
                "The retail JAudio stream has no concrete disc path");
        }

        auto recipe = aurora::audio::decode_jaudio_stream(
            _stream_loader(metadata.stream_path), metadata.channel_control);
        recipe.voice.gain_multiplier =
            static_cast<float>(metadata.volume) / 255.0F;

        _mixer->open_default_playback();
        require_working_output();

        if (_bgm_voices[static_cast<std::size_t>(lane)].has_value()) {
            _mixer->stop_voice(_bgm_voices[static_cast<std::size_t>(lane)]->token);
            _bgm_voices[static_cast<std::size_t>(lane)].reset();
            _bgm_handles[static_cast<std::size_t>(lane)].releaseSound();
        }

        const auto token = _mixer->start_voice(recipe.voice);
        try {
            if (prepared) {
                _mixer->set_voice_paused(token, true);
            }
            _bgm_handles[static_cast<std::size_t>(lane)].attachBackend(this, token.value);
            _bgm_voices[static_cast<std::size_t>(lane)] = BgmVoiceEntry{
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
            _bgm_handles[static_cast<std::size_t>(lane)].releaseSound();
            throw;
        }
        return &_bgm_handles[static_cast<std::size_t>(lane)];
    }

    void JAudioPlaybackService::unlock_bgm(BgmLane lane) {
        retire_finished_voices();
        if (!_bgm_voices[static_cast<std::size_t>(lane)].has_value()) {
            aurora::throw_host_exception<std::logic_error>(
                "Cannot unlock BGM without a concrete backend voice");
        }
        if (_bgm_voices[static_cast<std::size_t>(lane)]->prepared && !_bgm_voices[static_cast<std::size_t>(lane)]->unlocked) {
            _bgm_voices[static_cast<std::size_t>(lane)]->unlocked = true;
            _mixer->set_voice_paused(_bgm_voices[static_cast<std::size_t>(lane)]->token,
                                     _bgm_voices[static_cast<std::size_t>(lane)]->host_paused);
        }
    }

    void JAudioPlaybackService::stop_bgm(BgmLane lane, std::uint32_t fade_frames) {
        retire_finished_voices();
        if (!_bgm_voices[static_cast<std::size_t>(lane)].has_value()) {
            return;
        }
        if (fade_frames == 0U) {
            _mixer->stop_voice(_bgm_voices[static_cast<std::size_t>(lane)]->token);
            _bgm_voices[static_cast<std::size_t>(lane)].reset();
            _bgm_handles[static_cast<std::size_t>(lane)].releaseSound();
            return;
        }
        // A prepared or explicitly paused host voice cannot advance its gain
        // ramp. Release the mixer pause as the host mechanism for preserving
        // JAudio's stop/fade retirement lifecycle, including a stop issued
        // before the first unlock.
        _mixer->set_voice_paused(_bgm_voices[static_cast<std::size_t>(lane)]->token, false);
        _bgm_voices[static_cast<std::size_t>(lane)]->unlocked = true;
        _bgm_voices[static_cast<std::size_t>(lane)]->host_paused = false;
        _mixer->fade_out_voice(
            _bgm_voices[static_cast<std::size_t>(lane)]->token, static_cast<double>(fade_frames) / 60.0);
        _bgm_voices[static_cast<std::size_t>(lane)]->stopping = true;
    }

    void JAudioPlaybackService::pause_bgm(BgmLane lane, bool paused) {
        retire_finished_voices();
        if (!_bgm_voices[static_cast<std::size_t>(lane)].has_value()) {
            aurora::throw_host_exception<std::logic_error>(
                "Cannot change BGM pause state without a concrete backend voice");
        }
        if (_bgm_voices[static_cast<std::size_t>(lane)]->stopping) {
            aurora::throw_host_exception<std::logic_error>(
                "Cannot change BGM pause state while its concrete voice is stopping");
        }
        _bgm_voices[static_cast<std::size_t>(lane)]->host_paused = paused;
        const auto preparation_locked =
            _bgm_voices[static_cast<std::size_t>(lane)]->prepared && !_bgm_voices[static_cast<std::size_t>(lane)]->unlocked;
        _mixer->set_voice_paused(_bgm_voices[static_cast<std::size_t>(lane)]->token,
                                 paused || preparation_locked);
    }

    bool JAudioPlaybackService::is_bgm_prepared(BgmLane lane) const {
        return _bgm_voices[static_cast<std::size_t>(lane)].has_value() && _bgm_voices[static_cast<std::size_t>(lane)]->prepared &&
               !_bgm_voices[static_cast<std::size_t>(lane)]->unlocked &&
               _mixer->is_voice_active(_bgm_voices[static_cast<std::size_t>(lane)]->token);
    }

    bool JAudioPlaybackService::is_bgm_paused(BgmLane lane) const {
        if (!has_active_bgm(lane)) {
            return false;
        }
        const auto paused = _mixer->voice_paused(_bgm_voices[static_cast<std::size_t>(lane)]->token);
        if (!paused.has_value()) {
            aurora::throw_host_exception<std::logic_error>(
                "Active BGM token disappeared during its pause query");
        }
        return *paused;
    }

    bool JAudioPlaybackService::is_bgm_stopping(BgmLane lane) const {
        return has_active_bgm(lane) && _bgm_voices[static_cast<std::size_t>(lane)]->stopping;
    }

    bool JAudioPlaybackService::has_active_bgm(BgmLane lane) const {
        return _bgm_voices[static_cast<std::size_t>(lane)].has_value() &&
               _mixer->is_voice_active(_bgm_voices[static_cast<std::size_t>(lane)]->token);
    }

    std::optional<std::uint32_t> JAudioPlaybackService::bgm_id(BgmLane lane) const {
        if (!has_active_bgm(lane)) {
            return std::nullopt;
        }
        return _bgm_voices[static_cast<std::size_t>(lane)]->metadata.sound_id;
    }

    std::string_view JAudioPlaybackService::bgm_name(BgmLane lane) const {
        if (!has_active_bgm(lane)) {
            return {};
        }
        return _bgm_voices[static_cast<std::size_t>(lane)]->name;
    }

    JAISoundHandle *JAudioPlaybackService::bgm_handle(BgmLane lane) {
        if (!has_active_bgm(lane)) {
            return nullptr;
        }
        if (!_bgm_handles[static_cast<std::size_t>(lane)].isBackendAttached(this, _bgm_voices[static_cast<std::size_t>(lane)]->token.value)) {
            aurora::throw_host_exception<std::logic_error>(
                "BGM handle is detached from its concrete backend voice");
        }
        return &_bgm_handles[static_cast<std::size_t>(lane)];
    }

    std::uint64_t JAudioPlaybackService::bgm_backend_token(BgmLane lane) const {
        if (!has_active_bgm(lane)) {
            return 0U;
        }
        return _bgm_voices[static_cast<std::size_t>(lane)]->token.value;
    }

    void JAudioPlaybackService::set_bgm_bus_gain(BgmLane lane, float volume) {
        auto &voice = _bgm_voices[static_cast<std::size_t>(lane)];
        if (voice.has_value()) {
            (void)_mixer->try_set_voice_bus_gain(voice->token, volume);
        }
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
        _category_volume->reset();
        _trigger_sound_permitted = true;
        _level_sound_permitted = true;
        for (std::size_t index = 0; index < _bgm_voices.size(); ++index) {
            _bgm_voices[index].reset();
            _bgm_handles[index].releaseSound();
        }
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
        for (const auto &voice : _bgm_voices) {
            if (voice.has_value() && _mixer->is_voice_active(voice->token)) {
                ++count;
            }
        }
        return count;
    }

    aurora::audio::PlaybackStats
    JAudioPlaybackService::playback_stats() const {
        return _mixer->stats();
    }

    void JAudioPlaybackService::ensure_archive() {
        const auto host_allocations = aurora::allocation::HostAllocationScope{};
        if (_archive == nullptr) {
            _archive = _archive_factory();
            if (_archive == nullptr) {
                aurora::throw_host_exception<std::runtime_error>(
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
        const auto has_bgm_voice = std::ranges::any_of(_bgm_voices, [this](const auto &voice) {
            return voice.has_value() && voice->token && _mixer->is_voice_active(voice->token);
        });
        const auto has_sound_effect_voice = std::ranges::any_of(
            _sound_effect_voices, [this](const auto &entry) {
                return entry.second->token &&
                       _mixer->is_voice_active(entry.second->token);
            });
        if ((has_backend_voice || has_bgm_voice || has_sound_effect_voice) &&
            !_mixer->is_device_open()) {
            aurora::throw_host_exception<std::runtime_error>(
                "SDL JAudio playback device stopped accepting mixed audio");
        }
    }

    void JAudioPlaybackService::retire_finished_voices() {
        const auto host_allocations = aurora::allocation::HostAllocationScope{};
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
        for (std::size_t index = 0; index < _bgm_voices.size(); ++index) {
            auto &voice = _bgm_voices[index];
            if (voice.has_value() && voice->token && !_mixer->is_voice_active(voice->token)) {
                voice.reset();
                _bgm_handles[index].releaseSound();
            }
        }
    }

}  // namespace smgpc::runtime
