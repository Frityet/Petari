#include "RuntimeServices.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace smgpc::game {
    namespace {

        [[nodiscard]] bool exists_regular_file(const std::filesystem::path &path) {
            std::error_code error{};
            return std::filesystem::is_regular_file(path, error);
        }

        [[nodiscard]] std::filesystem::path weakly_canonical_or_normal(const std::filesystem::path &path) {
            std::error_code error{};
            auto canonical = std::filesystem::weakly_canonical(path, error);
            if (!error) {
                return canonical;
            }

            return path.lexically_normal();
        }

    }  // namespace

    DvdFileSystemService::DvdFileSystemService(std::filesystem::path root) : _root(weakly_canonical_or_normal(std::move(root))) {
    }

    const std::filesystem::path &DvdFileSystemService::root() const {
        return _root;
    }

    std::filesystem::path DvdFileSystemService::resolve(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path(disc_path);
        if (normalized.empty()) {
            return _root;
        }

        return weakly_canonical_or_normal(_root / normalized);
    }

    bool DvdFileSystemService::exists(std::string_view disc_path) const {
        return exists_regular_file(resolve(disc_path));
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_first(std::initializer_list<std::filesystem::path> candidates) const {
        for (const auto &candidate : candidates) {
            const auto path = candidate.is_absolute() ? weakly_canonical_or_normal(candidate) : resolve(candidate.generic_string());
            if (exists_regular_file(path)) {
                return path;
            }
        }

        return std::nullopt;
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_layout_archive(std::string_view layout_name) const {
        const auto archive_name = std::string(layout_name) + ".arc";
        return find_first({
            std::filesystem::path("KrKorean") / "LayoutData" / archive_name,
            std::filesystem::path("LayoutData") / archive_name,
        });
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_object_archive(std::string_view object_name) const {
        const auto archive_name = std::string(object_name) + ".arc";
        return find_first({
            std::filesystem::path("ObjectData") / archive_name,
        });
    }

    std::vector<std::uint8_t> DvdFileSystemService::read_file(std::string_view disc_path) const {
        const auto path = resolve(disc_path);
        auto file = std::ifstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open DVD file " + path.string());
        }

        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if (size < 0) {
            throw std::runtime_error("Cannot determine DVD file size " + path.string());
        }

        auto bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!file) {
            throw std::runtime_error("Cannot read DVD file " + path.string());
        }

        return bytes;
    }

    RarcArchive &DvdFileSystemService::archive(std::string_view disc_path) {
        const auto key = archive_cache_key(disc_path);
        if (auto it = _archives.find(key); it != _archives.end()) {
            return *it->second;
        }

        auto archive = std::make_unique<RarcArchive>(RarcArchive::from_file(std::filesystem::path(key)));
        auto [it, inserted] = _archives.emplace(key, std::move(archive));
        if (inserted) {
            ++_archive_load_counts[key];
        }

        return *it->second;
    }

    std::size_t DvdFileSystemService::archive_load_count(std::string_view disc_path) const {
        const auto key = archive_cache_key(disc_path);
        if (auto it = _archive_load_counts.find(key); it != _archive_load_counts.end()) {
            return it->second;
        }

        return 0U;
    }

    std::size_t DvdFileSystemService::cached_archive_count() const {
        return _archives.size();
    }

    std::filesystem::path DvdFileSystemService::normalize_disc_path(std::string_view disc_path) const {
        auto text = std::string(disc_path);
        std::ranges::replace(text, '\\', '/');

        while (!text.empty() && text.front() == '/') {
            text.erase(text.begin());
        }
        while (text.starts_with("./")) {
            text.erase(0U, 2U);
        }
        if (text.starts_with("files/")) {
            text.erase(0U, 6U);
        }

        auto normalized = std::filesystem::path();
        for (const auto &component : std::filesystem::path(text)) {
            const auto part = component.generic_string();
            if (part.empty() || part == ".") {
                continue;
            }
            if (part == "..") {
                throw std::runtime_error("DVD path escapes disc root: " + std::string(disc_path));
            }

            normalized /= component;
        }

        return normalized;
    }

    std::string DvdFileSystemService::archive_cache_key(std::string_view disc_path) const {
        return resolve(disc_path).generic_string();
    }

    void WpadService::begin_frame() {
        for (auto &channel : _channels) {
            channel.previous_hold = channel.hold;
            channel.previous_core_swing = channel.core_swing;
            channel.previous_sub_swing = channel.sub_swing;
            channel.trigger = 0U;
            channel.release = 0U;
            channel.repeat = 0U;
            if (channel.hold != 0U) {
                ++channel.hold_frame_count;
                if (channel.hold_frame_count == 1U || (channel.hold_frame_count > 30U && (channel.hold_frame_count % 8U) == 0U)) {
                    channel.repeat = channel.hold;
                }
            } else {
                channel.hold_frame_count = 0U;
            }
            if (!channel.connected) {
                channel.hold = 0U;
                channel.pointer.valid = false;
                channel.pointer_history_count = 0U;
            }
        }
    }

    void WpadService::set_connected(s32 channel, bool connected) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = connected;
        if (!connected) {
            state->hold = 0U;
            state->trigger = 0U;
            state->release = 0U;
            state->repeat = 0U;
            state->hold_frame_count = 0U;
            state->pointer.valid = false;
            state->pointer_history_count = 0U;
        }
    }

    void WpadService::set_button_mask(s32 channel, std::uint32_t hold) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = true;
        state->hold = hold;
        state->trigger = hold & ~state->previous_hold;
        state->release = state->previous_hold & ~hold;
        if (hold != state->previous_hold) {
            state->hold_frame_count = hold == 0U ? 0U : 1U;
            state->repeat = state->trigger;
        }
    }

    void WpadService::set_pointer(s32 channel, float x, float y, bool valid) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = true;
        state->pointer = WpadPointerState{
            .x = x,
            .y = y,
            .valid = valid,
        };
        for (auto i = state->pointer_history.size() - 1U; i > 0U; --i) {
            state->pointer_history[i] = state->pointer_history[i - 1U];
        }
        state->pointer_history[0U] = state->pointer;
        state->pointer_history_count = std::min<std::uint32_t>(static_cast<std::uint32_t>(state->pointer_history.size()),
                                                               state->pointer_history_count + 1U);
    }

    void WpadService::set_sub_stick(s32 channel, float x, float y) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = true;
        state->sub_stick = WpadStickState{
            .x = x,
            .y = y,
        };
    }

    void WpadService::set_core_acceleration(s32 channel, float x, float y, float z) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = true;
        state->core_acceleration = WpadVec3State{
            .x = x,
            .y = y,
            .z = z,
        };
    }

    void WpadService::set_sub_acceleration(s32 channel, float x, float y, float z) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = true;
        state->sub_acceleration = WpadVec3State{
            .x = x,
            .y = y,
            .z = z,
        };
    }

    void WpadService::set_swing(s32 channel, bool core_swing, bool sub_swing) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = true;
        state->core_swing = core_swing;
        state->sub_swing = sub_swing;
    }

    void WpadService::set_distance_to_display(s32 channel, float distance) {
        auto *state = mutable_channel_state(channel);
        if (state == nullptr) {
            return;
        }

        state->connected = true;
        state->distance_to_display = distance;
    }

    bool WpadService::is_connected(s32 channel) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected;
    }

    bool WpadService::is_button_held(s32 channel, std::uint32_t button_mask) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected && (state->hold & button_mask) != 0U;
    }

    bool WpadService::is_button_triggered(s32 channel, std::uint32_t button_mask) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected && (state->trigger & button_mask) != 0U;
    }

    bool WpadService::is_button_released(s32 channel, std::uint32_t button_mask) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected && (state->release & button_mask) != 0U;
    }

    bool WpadService::is_button_repeated(s32 channel, std::uint32_t button_mask) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected && (state->repeat & button_mask) != 0U;
    }

    WpadPointerState WpadService::pointer(s32 channel) const {
        const auto *state = channel_state(channel);
        return state == nullptr ? WpadPointerState{} : state->pointer;
    }

    WpadPointerState WpadService::past_pointer(s32 channel, std::uint32_t index) const {
        const auto *state = channel_state(channel);
        if (state == nullptr || index >= state->pointer_history_count || index >= state->pointer_history.size()) {
            return {};
        }

        return state->pointer_history[index];
    }

    std::uint32_t WpadService::pointer_history_count(s32 channel) const {
        const auto *state = channel_state(channel);
        return state == nullptr ? 0U : state->pointer_history_count;
    }

    WpadStickState WpadService::sub_stick(s32 channel) const {
        const auto *state = channel_state(channel);
        return state == nullptr ? WpadStickState{} : state->sub_stick;
    }

    WpadVec3State WpadService::core_acceleration(s32 channel) const {
        const auto *state = channel_state(channel);
        return state == nullptr ? WpadVec3State{} : state->core_acceleration;
    }

    WpadVec3State WpadService::sub_acceleration(s32 channel) const {
        const auto *state = channel_state(channel);
        return state == nullptr ? WpadVec3State{} : state->sub_acceleration;
    }

    bool WpadService::is_core_swing(s32 channel) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected && state->core_swing;
    }

    bool WpadService::is_core_swing_triggered(s32 channel) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected && state->core_swing && !state->previous_core_swing;
    }

    bool WpadService::is_sub_swing(s32 channel) const {
        const auto *state = channel_state(channel);
        return state != nullptr && state->connected && state->sub_swing;
    }

    float WpadService::distance_to_display(s32 channel) const {
        const auto *state = channel_state(channel);
        return state == nullptr ? 0.0F : state->distance_to_display;
    }

    const WpadChannelState *WpadService::channel_state(s32 channel) const {
        if (channel < 0 || channel >= static_cast<s32>(_channels.size())) {
            return nullptr;
        }

        return &_channels[static_cast<std::size_t>(channel)];
    }

    WpadChannelState *WpadService::mutable_channel_state(s32 channel) {
        if (channel < 0 || channel >= static_cast<s32>(_channels.size())) {
            return nullptr;
        }

        return &_channels[static_cast<std::size_t>(channel)];
    }

    void AudioEventService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void AudioEventService::start_stage_bgm(std::string_view name) {
        _stage_bgm_requested = true;
        _stage_bgm_start_frame = _frame_index;
        _stage_bgm_name = name;
        push_event(AudioEventKind::StageBgmStart, name);
    }

    void AudioEventService::unlock_stage_bgm() {
        _stage_bgm_unlocked = true;
        push_event(AudioEventKind::StageBgmUnlock, {});
    }

    void AudioEventService::stop_stage_bgm(s32 fade_frames) {
        const auto stopped_name = _stage_bgm_name;
        _stage_bgm_requested = false;
        _stage_bgm_name.clear();
        push_event(AudioEventKind::StageBgmStop, stopped_name, fade_frames);
    }

    void AudioEventService::start_system_sound(std::string_view name) {
        push_event(AudioEventKind::SystemSoundStart, name);
    }

    void AudioEventService::start_controller_speaker_sound(std::string_view name) {
        push_event(AudioEventKind::ControllerSpeakerSoundStart, name);
    }

    bool AudioEventService::is_stage_bgm_prepared() const {
        return _stage_bgm_requested && _frame_index > _stage_bgm_start_frame;
    }

    bool AudioEventService::is_stage_bgm_unlocked() const {
        return _stage_bgm_unlocked;
    }

    std::string_view AudioEventService::current_stage_bgm_name() const {
        return _stage_bgm_name;
    }

    std::span<const AudioEvent> AudioEventService::events() const {
        return _events;
    }

    void AudioEventService::push_event(AudioEventKind kind, std::string_view name, s32 fade_frames) {
        _events.push_back(AudioEvent{
            .kind = kind,
            .name = std::string(name),
            .fade_frames = fade_frames,
            .frame_index = _frame_index,
        });
    }

    void EffectService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void EffectService::emit(std::string_view actor_name, std::string_view effect_name) {
        auto &actor_effects = _active_effects[std::string(actor_name)];
        const auto found = std::ranges::find_if(actor_effects, [effect_name](const auto &active_name) { return active_name == effect_name; });
        if (found == actor_effects.end()) {
            actor_effects.emplace_back(effect_name);
        }

        _events.push_back(EffectEvent{
            .kind = EffectEventKind::Emit,
            .actor_name = std::string(actor_name),
            .effect_name = std::string(effect_name),
            .frame_index = _frame_index,
        });
    }

    void EffectService::delete_all(std::string_view actor_name) {
        _active_effects.erase(std::string(actor_name));
        _events.push_back(EffectEvent{
            .kind = EffectEventKind::DeleteAll,
            .actor_name = std::string(actor_name),
            .effect_name = {},
            .frame_index = _frame_index,
        });
    }

    std::span<const EffectEvent> EffectService::events() const {
        return _events;
    }

    std::vector<std::string> EffectService::active_effects(std::string_view actor_name) const {
        if (auto it = _active_effects.find(std::string(actor_name)); it != _active_effects.end()) {
            return it->second;
        }

        return {};
    }

    void SaveDataService::write_file(std::string_view name, std::span<const std::uint8_t> bytes) {
        _files[std::string(name)] = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
    }

    std::optional<std::vector<std::uint8_t>> SaveDataService::read_file(std::string_view name) const {
        if (auto it = _files.find(std::string(name)); it != _files.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    bool SaveDataService::exists(std::string_view name) const {
        return _files.contains(std::string(name));
    }

    bool SaveDataService::erase(std::string_view name) {
        return _files.erase(std::string(name)) != 0U;
    }

    std::size_t SaveDataService::file_count() const {
        return _files.size();
    }

    void MessageService::set_message(std::string_view tag, std::string_view text) {
        _messages[std::string(tag)] = text;
    }

    const std::string *MessageService::message(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second;
        }

        return nullptr;
    }

    std::string MessageService::message_or(std::string_view tag, std::string_view fallback) const {
        const auto *text = message(tag);
        return text == nullptr ? std::string(fallback) : *text;
    }

    RflService::RflService()
        : _miis{
              RflMiiEntry{
                  .index = 0,
                  .name = "Mario",
              },
          } {
    }

    void RflService::set_initialized(bool initialized) {
        _initialized = initialized;
    }

    void RflService::set_error(bool error) {
        _error = error;
    }

    void RflService::set_miis(std::vector<RflMiiEntry> miis) {
        _miis = std::move(miis);
    }

    bool RflService::is_initialized() const {
        return _initialized;
    }

    bool RflService::has_error() const {
        return _error;
    }

    std::span<const RflMiiEntry> RflService::valid_miis() const {
        return _miis;
    }

}  // namespace smgpc::game
