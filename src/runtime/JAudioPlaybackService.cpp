#include <aurora/exception.hpp>
#include <aurora/allocation.hpp>
#include "runtime/JAudioPlaybackService.hpp"

#include "compat/JAudioSoundParameterSemantics.hpp"
#include "compat/JAudioCategoryVolumeOwnership.hpp"
#include "compat/NativePcmSound.hpp"
#include "compat/JaiStreamPlayback.hpp"
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
        _stream_playback = std::make_unique<compat::JaiStreamPlayback>(*_mixer, _stream_loader);
    }

    JAudioPlaybackService::~JAudioPlaybackService() {
        reset_scene();
    }

    void JAudioPlaybackService::begin_frame(std::uint64_t frame_index) {
        if (_frame_open) aurora::throw_host_exception<std::logic_error>("JAudio frame is already open");
        _category_volume->update();
        apply_category_gains();
        _frame_index = frame_index;
        _frame_open = true;
        require_working_output();
        retire_finished_voices();
    }

    void JAudioPlaybackService::end_frame() {
        if (!_frame_open) aurora::throw_host_exception<std::logic_error>("JAudio frame was not opened");
        require_working_output();
        for (auto& [id, voice] : _level_voices) if (voice.sound) voice.sound->advance();
        for (auto& [token, voice] : _sound_effect_voices) voice->sound->advance();
        _stream_playback->advance();
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
        if (existing == _level_voices.end()) {
            const auto recipe = _archive->resolve_persistent_sound(name);
            if (!recipe) aurora::throw_host_exception<std::logic_error>("Resolved level sound has no PCM recipe");
            LevelVoiceEntry entry;
            entry.name = name;
            entry.recipe = *recipe;
            existing = _level_voices.emplace(*sound_id, std::move(entry)).first;
        }
        auto& voice = existing->second;
        if (!voice.sound || voice.sound->isDead()) {
            auto spec = voice.recipe.voice;
            spec.bus_gain_multiplier = sound_category_gain(*sound_id);
            voice.sound = std::make_unique<compat::NativePcmSound>(*_mixer, JAISoundID(*sound_id), spec);
            voice.sound->attachHandle(&voice.handle);
            voice.sound->setLifeTime(1, false);
        } else {
            voice.sound->updateLifeTime(1);
        }
        voice.sound->getProperty().mVolume = adjustment->gain_multiplier;
        voice.sound->getProperty().mPitch = adjustment->pitch_multiplier;
        voice.sound->mix();
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
        auto voice = std::make_unique<SoundEffectVoiceEntry>();
        voice->name = name;
        voice->sound_id = *sound_id;
        voice->sound = std::make_unique<compat::NativePcmSound>(*_mixer, JAISoundID(*sound_id), spec);
        voice->sound->attachHandle(&voice->handle);
        const auto token = voice->sound->token();
        auto* handle = &voice->handle;
        const auto [position, inserted] = _sound_effect_voices.emplace(token.value, std::move(voice));
        if (!inserted) aurora::throw_host_exception<std::logic_error>("PCM mixer reused a live token");
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
            voice->sound->stop(delay_frames);
            voice->sound->mix();
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

    std::vector<std::uint8_t> JAudioPlaybackService::native_sound_name_table() {
        ensure_archive();
        return _archive->native_sound_name_table();
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
        for (const auto& [id, voice] : _level_voices) if (voice.sound) {
            (void)_mixer->try_set_voice_bus_gain(voice.sound->token(), sound_category_gain(id));
        }
        for (const auto& [token, voice] : _sound_effect_voices) {
            (void)_mixer->try_set_voice_bus_gain(voice->sound->token(), sound_category_gain(voice->sound_id));
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

        _mixer->open_default_playback();
        require_working_output();
        stop_bgm(lane, 0);
        const auto index = static_cast<std::size_t>(lane);
        auto* sound = _stream_playback->start(metadata, _bgm_handles[index], prepared);
        if (!sound) return nullptr;
        _bgm_voices[index] = BgmVoiceEntry{std::string(name), std::move(metadata), sound};
        return &_bgm_handles[index];
    }

    void JAudioPlaybackService::unlock_bgm(BgmLane lane) {
        retire_finished_voices();
        auto* sound = bgm_sound(lane);
        if (!sound) aurora::throw_host_exception<std::logic_error>("Cannot unlock an absent original stream");
        sound->unlockIfLocked();
        _stream_playback->mix();
    }

    void JAudioPlaybackService::stop_bgm(BgmLane lane, std::uint32_t fade_frames) {
        retire_finished_voices();
        if (auto* sound = bgm_sound(lane)) sound->stop(fade_frames);
    }

    void JAudioPlaybackService::pause_bgm(BgmLane lane, bool paused) {
        retire_finished_voices();
        auto* sound = bgm_sound(lane);
        if (!sound) aurora::throw_host_exception<std::logic_error>("Cannot pause an absent original stream");
        sound->pause(paused);
        _stream_playback->mix();
    }

    bool JAudioPlaybackService::is_bgm_prepared(BgmLane lane) const {
        auto* sound = bgm_sound(lane);
        return sound && sound->isPrepared();
    }

    bool JAudioPlaybackService::is_bgm_paused(BgmLane lane) const {
        auto* sound = bgm_sound(lane);
        return sound && sound->isPaused();
    }

    bool JAudioPlaybackService::is_bgm_stopping(BgmLane lane) const {
        auto* sound = bgm_sound(lane);
        return sound && sound->isStopping();
    }

    bool JAudioPlaybackService::has_active_bgm(BgmLane lane) const {
        auto* sound = bgm_sound(lane);
        return sound && sound->mHandle && !sound->isDead();
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
        auto* sound = bgm_sound(lane);
        return sound && !sound->isDead() ? sound->mHandle : nullptr;
    }

    std::uint64_t JAudioPlaybackService::bgm_backend_token(BgmLane lane) const {
        return _stream_playback->token(bgm_sound(lane)).value;
    }

    void JAudioPlaybackService::set_bgm_bus_gain(BgmLane lane, float volume) {
        if (auto* sound = bgm_sound(lane)) {
            sound->getAuxiliary().moveVolume(volume, 0);
            _stream_playback->mix();
        }
    }

    bool JAudioPlaybackService::has_me() const {
        return false;
    }

    void JAudioPlaybackService::reset_scene() {
        if (_stream_playback) _stream_playback->reset();
        for (auto& [id, voice] : _level_voices) voice.sound.reset();
        _sound_effect_voices.clear();
        _retired_sound_effect_voices.clear();
        _mixer->stop_all_voices();
        _category_volume->reset();
        _trigger_sound_permitted = true;
        _level_sound_permitted = true;
        for (auto& voice : _bgm_voices) voice.reset();
        // Scene teardown may occur between begin_frame and end_frame.
    }

    bool JAudioPlaybackService::is_device_open() const {
        return _mixer->is_device_open();
    }

    std::size_t JAudioPlaybackService::active_voice_count() const {
        return _mixer->active_voice_count();
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
        if (active_voice_count() && !_mixer->is_device_open()) {
            aurora::throw_host_exception<std::runtime_error>("SDL JAudio device stopped accepting mixed audio");
        }
    }

    void JAudioPlaybackService::retire_finished_voices() {
        const aurora::allocation::HostAllocationScope host;
        for (auto& [id, voice] : _level_voices) if (voice.sound) voice.sound->reconcile_completion();
        for (auto it = _sound_effect_voices.begin(); it != _sound_effect_voices.end();) {
            it->second->sound->reconcile_completion();
            if (!it->second->sound->isDead()) { ++it; continue; }
            _retired_sound_effect_voices.push_back(std::move(it->second));
            it = _sound_effect_voices.erase(it);
        }
        if (_stream_playback) _stream_playback->reconcile();
        for (auto& voice : _bgm_voices) if (voice && !_stream_playback->find(voice->sound)) voice.reset();
    }

    JAISound* JAudioPlaybackService::bgm_sound(BgmLane lane) const {
        const auto& voice = _bgm_voices[static_cast<std::size_t>(lane)];
        return voice ? _stream_playback->find(voice->sound) : nullptr;
    }
    void JAudioPlaybackService::bind_bgm_handle(BgmLane lane, JAISoundHandle& handle) {
        auto* sound = bgm_sound(lane);
        if (sound && sound->mHandle) {
            if (sound->mHandle != &handle) sound->attachHandle(&handle);
        } else {
            handle.releaseSound();
        }
    }
    std::uint64_t JAudioPlaybackService::sound_backend_token(const JAISound* sound) const {
        if (!sound) return 0;
        for (const auto& [id, voice] : _level_voices) if (voice.sound.get() == sound) return voice.sound->token().value;
        for (const auto& [token, voice] : _sound_effect_voices) if (voice->sound.get() == sound) return voice->sound->token().value;
        return _stream_playback->token(sound).value;
    }
    bool JAudioPlaybackService::owns_sound(const JAISound* sound) const {
        if (!sound) return false;
        for (const auto& [id, voice] : _level_voices) if (voice.sound.get() == sound) return true;
        for (const auto& [token, voice] : _sound_effect_voices) if (voice->sound.get() == sound) return true;
        return _stream_playback->find(sound) != nullptr;
    }

}  // namespace smgpc::runtime
