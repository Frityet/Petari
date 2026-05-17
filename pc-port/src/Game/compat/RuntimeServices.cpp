#include "RuntimeServices.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/compat/BmgMessageArchive.hpp"
#include "Game/compat/TextEncoding.hpp"

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

        [[nodiscard]] std::vector<std::uint8_t> read_binary_file(const std::filesystem::path &path) {
            auto file = std::ifstream(path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot open save file " + path.string());
            }

            file.seekg(0, std::ios::end);
            const auto size = file.tellg();
            if (size < 0) {
                throw std::runtime_error("Cannot determine save file size " + path.string());
            }

            auto bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(size));
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file) {
                throw std::runtime_error("Cannot read save file " + path.string());
            }

            return bytes;
        }

        void write_binary_file(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
            std::error_code error{};
            std::filesystem::create_directories(path.parent_path(), error);
            if (error) {
                throw std::runtime_error("Cannot create save directory " + path.parent_path().string());
            }

            auto file = std::ofstream(path, std::ios::binary | std::ios::trunc);
            if (!file) {
                throw std::runtime_error("Cannot open save file for writing " + path.string());
            }

            file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file) {
                throw std::runtime_error("Cannot write save file " + path.string());
            }
        }

        [[nodiscard]] std::string original_config_name(s32 slot_index) {
            return "config" + std::to_string(slot_index);
        }

        [[nodiscard]] std::string original_game_name(s32 slot_index, bool is_player_mario) {
            return std::string(is_player_mario ? "mario" : "luigi") + std::to_string(slot_index);
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
        return archive_for_path(resolve(disc_path));
    }

    RarcArchive &DvdFileSystemService::archive_for_path(const std::filesystem::path &path) {
        const auto key = archive_cache_key_for_path(path);
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
        return archive_load_count_for_path(resolve(disc_path));
    }

    std::size_t DvdFileSystemService::archive_load_count_for_path(const std::filesystem::path &path) const {
        const auto key = archive_cache_key_for_path(path);
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

    std::string DvdFileSystemService::archive_cache_key_for_path(const std::filesystem::path &path) const {
        return weakly_canonical_or_normal(path).generic_string();
    }

    std::string DvdFileSystemService::archive_cache_key(std::string_view disc_path) const {
        return archive_cache_key_for_path(resolve(disc_path));
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

    void AudioEventService::set_stage_bgm_state(s32 state, u32 change_frames) {
        _stage_bgm_state = state;
        _stage_bgm_state_change_frames = change_frames;
        push_event(AudioEventKind::StageBgmStateChange, _stage_bgm_name, 0, state, change_frames);
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

    s32 AudioEventService::stage_bgm_state() const {
        return _stage_bgm_state;
    }

    u32 AudioEventService::stage_bgm_state_change_frames() const {
        return _stage_bgm_state_change_frames;
    }

    std::span<const AudioEvent> AudioEventService::events() const {
        return _events;
    }

    void AudioEventService::push_event(AudioEventKind kind, std::string_view name, s32 fade_frames, s32 state, u32 change_frames) {
        _events.push_back(AudioEvent{
            .kind = kind,
            .name = std::string(name),
            .fade_frames = fade_frames,
            .state = state,
            .change_frames = change_frames,
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

    void EffectService::delete_effect(std::string_view actor_name, std::string_view effect_name) {
        if (auto it = _active_effects.find(std::string(actor_name)); it != _active_effects.end()) {
            std::erase(it->second, std::string(effect_name));
            if (it->second.empty()) {
                _active_effects.erase(it);
            }
        }

        _events.push_back(EffectEvent{
            .kind = EffectEventKind::Delete,
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

    void WipeService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        if ((_state != WipeState::Opening && _state != WipeState::Closing) || _remaining_frames <= 0) {
            return;
        }

        --_remaining_frames;
        if (_remaining_frames <= 0) {
            _state = _state == WipeState::Opening ? WipeState::Open : WipeState::Closed;
        }
    }

    void WipeService::open(std::string_view name, s32 frame_count) {
        start_transition(WipeEventKind::Open, WipeState::Opening, name, frame_count);
    }

    void WipeService::close(std::string_view name, s32 frame_count) {
        start_transition(WipeEventKind::Close, WipeState::Closing, name, frame_count);
    }

    void WipeService::force_open(std::string_view name) {
        _current_name = name;
        _state = WipeState::Open;
        _remaining_frames = 0;
        _duration_frames = 0;
        push_event(WipeEventKind::ForceOpen, name, 0);
    }

    void WipeService::force_close(std::string_view name) {
        _current_name = name;
        _state = WipeState::Closed;
        _remaining_frames = 0;
        _duration_frames = 0;
        push_event(WipeEventKind::ForceClose, name, 0);
    }

    bool WipeService::is_active() const {
        return _state == WipeState::Opening || _state == WipeState::Closing;
    }

    bool WipeService::is_blank() const {
        return _state == WipeState::Closed;
    }

    bool WipeService::is_open() const {
        return _state == WipeState::Open;
    }

    WipeState WipeService::state() const {
        return _state;
    }

    std::string_view WipeService::current_name() const {
        return _current_name;
    }

    s32 WipeService::remaining_frames() const {
        return _remaining_frames;
    }

    s32 WipeService::duration_frames() const {
        return _duration_frames;
    }

    std::span<const WipeEvent> WipeService::events() const {
        return _events;
    }

    void WipeService::start_transition(WipeEventKind kind, WipeState state, std::string_view name, s32 frame_count) {
        _current_name = name;
        _duration_frames = normalized_frame_count(frame_count);
        _remaining_frames = _duration_frames;
        _state = _remaining_frames <= 0 ? (state == WipeState::Opening ? WipeState::Open : WipeState::Closed) : state;
        push_event(kind, name, frame_count);
    }

    void WipeService::push_event(WipeEventKind kind, std::string_view name, s32 frame_count) {
        _events.push_back(WipeEvent{
            .kind = kind,
            .name = std::string(name),
            .frame_count = frame_count,
            .frame_index = _frame_index,
        });
    }

    s32 WipeService::normalized_frame_count(s32 frame_count) {
        return frame_count < 0 ? 30 : frame_count;
    }

    void StarPointerService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void StarPointerService::start_mode(StarPointerMode mode) {
        if (_mode == mode) {
            return;
        }

        _mode = mode;
        _mode_events.push_back(StarPointerModeEvent{
            .mode = mode,
            .frame_index = _frame_index,
        });
    }

    void StarPointerService::set_guidance_active(bool active) {
        _guidance_active = active;
    }

    void StarPointerService::request_file_select_guidance() {
        _file_select_guidance_requested = true;
    }

    void StarPointerService::request_file_select_copy_guidance() {
        _file_select_copy_guidance_requested = true;
    }

    StarPointerMode StarPointerService::mode() const {
        return _mode;
    }

    bool StarPointerService::is_guidance_active() const {
        return _guidance_active;
    }

    bool StarPointerService::is_file_select_guidance_requested() const {
        return _file_select_guidance_requested;
    }

    bool StarPointerService::is_file_select_copy_guidance_requested() const {
        return _file_select_copy_guidance_requested;
    }

    std::span<const StarPointerModeEvent> StarPointerService::mode_events() const {
        return _mode_events;
    }

    void CameraSystemService::reset_camera_man() {
        ++_reset_camera_man_count;
    }

    void CameraSystemService::request_normal_shake() {
        ++_normal_shake_request_count;
    }

    void CameraSystemService::pause_on_camera_director() {
        ++_camera_director_pause_count;
    }

    void CameraSystemService::pause_off_camera_director() {
        if (_camera_director_pause_count > 0U) {
            --_camera_director_pause_count;
        }
    }

    std::uint32_t CameraSystemService::reset_camera_man_count() const {
        return _reset_camera_man_count;
    }

    std::uint32_t CameraSystemService::normal_shake_request_count() const {
        return _normal_shake_request_count;
    }

    std::uint32_t CameraSystemService::camera_director_pause_count() const {
        return _camera_director_pause_count;
    }

    bool CameraSystemService::is_camera_director_paused() const {
        return _camera_director_pause_count > 0U;
    }

    void PlayerSystemService::hide_player() {
        _player_hidden = true;
    }

    void PlayerSystemService::set_base_matrix(MtxPtr matrix) {
        _has_base_matrix = matrix != nullptr;
        if (matrix == nullptr) {
            _base_matrix = {};
            return;
        }

        auto index = std::size_t{};
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                _base_matrix[index++] = matrix[row][column];
            }
        }
    }

    bool PlayerSystemService::is_player_hidden() const {
        return _player_hidden;
    }

    bool PlayerSystemService::has_base_matrix() const {
        return _has_base_matrix;
    }

    std::span<const f32, 12U> PlayerSystemService::base_matrix() const {
        return _base_matrix;
    }

    void GameLayoutService::deactivate_default_game_layout() {
        _default_game_layout_active = false;
    }

    void GameLayoutService::activate_game_scene_draw_3d() {
        _game_scene_draw_3d_active = true;
    }

    void GameLayoutService::deactivate_game_scene_draw_3d() {
        _game_scene_draw_3d_active = false;
    }

    bool GameLayoutService::is_default_game_layout_active() const {
        return _default_game_layout_active;
    }

    bool GameLayoutService::is_game_scene_draw_3d_active() const {
        return _game_scene_draw_3d_active;
    }

    void RumbleService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void RumbleService::request_strong(s32 channel) {
        push_event(RumbleRequestKind::Strong, channel);
    }

    void RumbleService::request_weak(s32 channel) {
        push_event(RumbleRequestKind::Weak, channel);
    }

    std::span<const RumbleRequestEvent> RumbleService::events() const {
        return _events;
    }

    void RumbleService::push_event(RumbleRequestKind kind, s32 channel) {
        _events.push_back(RumbleRequestEvent{
            .kind = kind,
            .channel = channel,
            .frame_index = _frame_index,
        });
    }

    void SequenceRequestService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void SequenceRequestService::request_change_stage_in_game_after_loading_game_data() {
        if (_change_stage_in_game_after_loading_game_data_requested) {
            return;
        }

        _change_stage_in_game_after_loading_game_data_requested = true;
        _events.push_back(SequenceRequestEvent{
            .kind = SequenceRequestKind::ChangeStageInGameAfterLoadingGameData,
            .frame_index = _frame_index,
        });
    }

    bool SequenceRequestService::is_change_stage_in_game_after_loading_game_data_requested() const {
        return _change_stage_in_game_after_loading_game_data_requested;
    }

    std::span<const SequenceRequestEvent> SequenceRequestService::events() const {
        return _events;
    }

    SaveDataService::SaveDataService() {
        for (auto slot_index = s32{1}; slot_index <= 6; ++slot_index) {
            _slot_states.push_back(SlotState{.slot_index = slot_index});
        }
    }

    void SaveDataService::write_file(std::string_view name, std::span<const std::uint8_t> bytes) {
        _files[std::string(name)] = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
        write_host_file(name, bytes);
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
        const auto erased = _files.erase(std::string(name)) != 0U;
        erase_host_file(name);
        return erased;
    }

    std::size_t SaveDataService::file_count() const {
        return _files.size();
    }

    void SaveDataService::set_host_directory(std::filesystem::path directory) {
        _host_directory = weakly_canonical_or_normal(std::move(directory));
        load_host_files();
    }

    const std::optional<std::filesystem::path> &SaveDataService::host_directory() const {
        return _host_directory;
    }

    void SaveDataService::load_host_files() {
        if (!_host_directory.has_value()) {
            return;
        }

        std::error_code error{};
        std::filesystem::create_directories(*_host_directory, error);
        if (error) {
            throw std::runtime_error("Cannot create save directory " + _host_directory->string());
        }

        _files.clear();
        for (const auto &entry : std::filesystem::recursive_directory_iterator(*_host_directory, error)) {
            if (error) {
                throw std::runtime_error("Cannot scan save directory " + _host_directory->string());
            }
            if (!entry.is_regular_file(error)) {
                continue;
            }

            const auto relative = std::filesystem::relative(entry.path(), *_host_directory, error);
            if (error || relative.empty()) {
                continue;
            }
            _files[relative.generic_string()] = read_binary_file(entry.path());
        }

        load_slot_states_from_files();
        load_sys_config_from_files();
    }

    void SaveDataService::flush_host_files() {
        if (!_host_directory.has_value()) {
            return;
        }

        write_sys_config_file();
        for (const auto &[name, bytes] : _files) {
            write_host_file(name, bytes);
        }
    }

    const SaveDataService::SlotState *SaveDataService::slot_state(s32 slot_index) const {
        const auto found = std::ranges::find_if(_slot_states, [slot_index](const auto &entry) { return entry.slot_index == slot_index; });
        return found == _slot_states.end() ? nullptr : &*found;
    }

    SaveDataService::SlotState SaveDataService::slot_state_or_default(s32 slot_index) const {
        if (const auto *state = slot_state(slot_index)) {
            return *state;
        }

        return SlotState{.slot_index = slot_index};
    }

    void SaveDataService::set_slot_state(s32 slot_index, const SlotState &state) {
        auto slot_state = state;
        slot_state.slot_index = slot_index;

        auto found = std::ranges::find_if(_slot_states, [slot_index](const auto &entry) { return entry.slot_index == slot_index; });
        if (found != _slot_states.end()) {
            *found = slot_state;
        } else {
            _slot_states.push_back(slot_state);
        }

        std::ranges::sort(_slot_states, {}, &SlotState::slot_index);
    }

    void SaveDataService::copy_slot_state(s32 dst_slot_index, s32 src_slot_index) {
        set_slot_state(dst_slot_index, slot_state_or_default(src_slot_index));
        const auto copy_file = [this, dst_slot_index, src_slot_index](std::string_view src_prefix, std::string_view dst_prefix) {
            const auto src_name = std::string(src_prefix) + std::to_string(src_slot_index);
            const auto dst_name = std::string(dst_prefix) + std::to_string(dst_slot_index);
            if (const auto bytes = read_file(src_name)) {
                write_file(dst_name, *bytes);
            } else {
                erase(dst_name);
            }
        };
        copy_file("config", "config");
        copy_file("mario", "mario");
        copy_file("luigi", "luigi");
    }

    void SaveDataService::clear_slot_states() {
        _slot_states.clear();
    }

    std::span<const SaveDataService::SlotState> SaveDataService::slot_states() const {
        return _slot_states;
    }

    void SaveDataService::restore_user_file(UserFile &file, s32 slot_index, bool is_player_mario) const {
        file.restoreFromSaveDataServiceSlot(slot_state_or_default(slot_index), slot_index, is_player_mario);
    }

    void SaveDataService::store_user_file(s32 slot_index, const UserFile &file) {
        set_slot_state(slot_index, file.makeSaveDataServiceSlot(slot_index));

        auto config_bytes = std::vector<std::uint8_t>(256U);
        auto game_bytes = std::vector<std::uint8_t>(256U);
        file.makeConfigDataBinary(config_bytes.data(), config_bytes.size());
        file.makeGameDataBinary(game_bytes.data(), game_bytes.size());
        write_file(file.getConfigDataName(), config_bytes);
        write_file(file.getGameDataName(), game_bytes);
    }

    void SaveDataService::set_sys_config_time_announced(OSTime time) {
        _sys_config_time_announced = time;
        write_sys_config_file();
    }

    void SaveDataService::update_sys_config_time_announced() {
        _sys_config_time_announced = OSGetTime();
        write_sys_config_file();
    }

    OSTime SaveDataService::sys_config_time_announced() const {
        return _sys_config_time_announced;
    }

    void SaveDataService::set_sys_config_time_sent(OSTime time) {
        _sys_config_time_sent = time;
        write_sys_config_file();
    }

    OSTime SaveDataService::sys_config_time_sent() const {
        return _sys_config_time_sent;
    }

    void SaveDataService::set_sys_config_sent_bytes(u32 bytes) {
        _sys_config_sent_bytes = bytes;
        write_sys_config_file();
    }

    u32 SaveDataService::sys_config_sent_bytes() const {
        return _sys_config_sent_bytes;
    }

    std::filesystem::path SaveDataService::host_file_path(std::string_view name) const {
        if (!_host_directory.has_value()) {
            return {};
        }

        auto relative = std::filesystem::path(std::string(name)).lexically_normal();
        if (relative.empty() || relative.is_absolute()) {
            throw std::runtime_error("Invalid save file name " + std::string(name));
        }

        for (const auto &part : relative) {
            if (part == "..") {
                throw std::runtime_error("Invalid save file name " + std::string(name));
            }
        }

        return *_host_directory / relative;
    }

    void SaveDataService::write_host_file(std::string_view name, std::span<const std::uint8_t> bytes) const {
        if (!_host_directory.has_value()) {
            return;
        }

        write_binary_file(host_file_path(name), bytes);
    }

    void SaveDataService::erase_host_file(std::string_view name) const {
        if (!_host_directory.has_value()) {
            return;
        }

        std::error_code error{};
        std::filesystem::remove(host_file_path(name), error);
    }

    void SaveDataService::load_slot_states_from_files() {
        for (auto slot_index = s32{1}; slot_index <= 6; ++slot_index) {
            auto file = UserFile();
            const auto config_name = original_config_name(slot_index);
            const auto config_bytes = read_file(config_name);
            file.loadFromConfigDataBinary(config_name.c_str(), config_bytes.has_value() ? config_bytes->data() : nullptr,
                                          config_bytes.has_value() ? static_cast<u32>(config_bytes->size()) : 0U);

            const auto is_player_mario = file.isLastLoadedMario();
            const auto game_name = original_game_name(slot_index, is_player_mario);
            const auto game_bytes = read_file(game_name);
            file.loadFromGameDataBinary(game_name.c_str(), game_bytes.has_value() ? game_bytes->data() : nullptr,
                                        game_bytes.has_value() ? static_cast<u32>(game_bytes->size()) : 0U);
            file.mIsPlayerMario = is_player_mario;
            set_slot_state(slot_index, file.makeSaveDataServiceSlot(slot_index));
        }
    }

    void SaveDataService::load_sys_config_from_files() {
        const auto sys_config_bytes = read_file("sysconf");
        if (!sys_config_bytes.has_value()) {
            return;
        }

        auto sys_config = SysConfigFile();
        sys_config.loadFromDataBinary(sys_config_bytes->data(), static_cast<u32>(sys_config_bytes->size()));
        _sys_config_time_announced = sys_config.getTimeAnnounced();
        _sys_config_time_sent = sys_config.getTimeSent();
        _sys_config_sent_bytes = sys_config.getSentBytes();
    }

    void SaveDataService::write_sys_config_file() {
        if (!_host_directory.has_value()) {
            return;
        }

        auto sys_config = SysConfigFile();
        sys_config.setTimeAnnounced(_sys_config_time_announced);
        sys_config.setTimeSent(_sys_config_time_sent);
        sys_config.setSentBytes(_sys_config_sent_bytes);
        auto bytes = std::vector<std::uint8_t>(64U);
        sys_config.makeDataBinary(bytes.data(), bytes.size());
        write_file("sysconf", bytes);
    }

    void MessageService::set_message(std::string_view tag, std::string_view text) {
        set_message(tag, utf16_from_utf8_lossy(text));
    }

    void MessageService::set_message(std::string_view tag, std::u16string_view text) {
        _messages[std::string(tag)] = MessageText{
            .utf16 = std::u16string(text),
            .utf8 = utf8_from_utf16_lossy(text),
        };
    }

    std::size_t MessageService::load_message_archive(const RarcArchive &archive) {
        const auto messages = BmgMessageArchive::from_message_archive(archive);
        for (const auto &message : messages.messages()) {
            set_message(message.id, message.display_text);
        }

        return messages.message_count();
    }

    std::size_t MessageService::message_count() const {
        return _messages.size();
    }

    const std::string *MessageService::message(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.utf8;
        }

        return nullptr;
    }

    const std::u16string *MessageService::message_utf16(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.utf16;
        }

        return nullptr;
    }

    std::string MessageService::message_or(std::string_view tag, std::string_view fallback) const {
        const auto *text = message(tag);
        return text == nullptr ? std::string(fallback) : *text;
    }

    std::u16string MessageService::message_utf16_or(std::string_view tag, std::u16string_view fallback) const {
        const auto *text = message_utf16(tag);
        return text == nullptr ? std::u16string(fallback) : *text;
    }

    void SceneLightService::clear() {
        _lights = {};
    }

    void SceneLightService::clear_light(std::size_t index) {
        if (index >= _lights.size()) {
            return;
        }

        _lights[index] = GXLightState{};
    }

    void SceneLightService::set_light(std::size_t index, const GXLightState &light) {
        if (index >= _lights.size()) {
            return;
        }

        _lights[index] = light;
        _lights[index].loaded = true;
    }

    const GXLightState *SceneLightService::light(std::size_t index) const {
        if (index >= _lights.size() || !_lights[index].loaded) {
            return nullptr;
        }

        return &_lights[index];
    }

    std::span<const GXLightState> SceneLightService::lights() const {
        return _lights;
    }

    std::uint8_t SceneLightService::loaded_mask() const {
        auto mask = std::uint8_t{};
        for (auto index = std::size_t{}; index < _lights.size(); ++index) {
            if (_lights[index].loaded) {
                mask |= static_cast<std::uint8_t>(1U << index);
            }
        }
        return mask;
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
