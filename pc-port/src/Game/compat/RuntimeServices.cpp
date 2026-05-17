#include "RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "Game/Scene/SceneFunction.hpp"
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

        [[nodiscard]] std::string texture_format_name(TplTextureFormat format) {
            switch (format) {
            case TplTextureFormat::I4:
                return "I4";
            case TplTextureFormat::I8:
                return "I8";
            case TplTextureFormat::IA4:
                return "IA4";
            case TplTextureFormat::IA8:
                return "IA8";
            case TplTextureFormat::RGB565:
                return "RGB565";
            case TplTextureFormat::RGB5A3:
                return "RGB5A3";
            case TplTextureFormat::RGBA8:
                return "RGBA8";
            case TplTextureFormat::C4:
                return "C4";
            case TplTextureFormat::C8:
                return "C8";
            case TplTextureFormat::C14X2:
                return "C14X2";
            case TplTextureFormat::CMPR:
                return "CMPR";
            }

            return "Unknown";
        }

        [[nodiscard]] bool effect_draw_type_accepts_order(s32 draw_type, std::string_view draw_order) {
            if (draw_type == MR::DrawType_EffectDraw3D) {
                return draw_order.empty() || draw_order == "3D";
            }
            if (draw_type == MR::DrawType_EffectDrawIndirect) {
                return draw_order == "Indirect";
            }
            if (draw_type == MR::DrawType_EffectDrawAfterIndirect) {
                return draw_order == "AfterIndirect";
            }
            if (draw_type == MR::DrawType_EffectDraw2D) {
                return draw_order.empty() || draw_order == "2D";
            }

            if (draw_type == MR::DrawType_EffectDrawFor2DModel) {
                return draw_order == "2DModel" || draw_order == "For2DModel";
            }
            if (draw_type == MR::DrawType_EffectDrawForBloomEffect) {
                return draw_order == "Bloom" || draw_order == "ForBloomEffect";
            }
            if (draw_type == MR::DrawType_EffectDrawAfterImageEffect) {
                return draw_order == "AfterImageEffect";
            }

            return false;
        }

        [[nodiscard]] float effect_billboard_size(const JpcTextureMetadata &texture, const ResolvedEffectResource &resource) {
            const auto source_size = static_cast<float>(std::max(texture.width, texture.height));
            const auto base_shape_scale = resource.resource != nullptr && resource.resource->base_shape.has_value() ?
                                              std::max(resource.resource->base_shape->base_size_x, resource.resource->base_shape->base_size_y) :
                                              1.0F;
            const auto scale = std::max(resource.auto_effect_scale_value * base_shape_scale, 0.125F);
            return std::clamp(source_size * scale, 8.0F, 192.0F);
        }

        [[nodiscard]] const JpcTextureMetadata *primary_effect_texture(const ResolvedEffectResource &resource) {
            if (resource.primary_texture_index.has_value()) {
                for (const auto &texture : resource.textures) {
                    if (texture.index == *resource.primary_texture_index) {
                        return &texture;
                    }
                }
            }

            return resource.textures.empty() ? nullptr : &resource.textures.front();
        }

        [[nodiscard]] const JpcTextureMetadata *child_effect_texture(const ResolvedEffectResource &resource) {
            if (resource.resource != nullptr && resource.resource->child_texture_index.has_value()) {
                for (const auto &texture : resource.textures) {
                    if (texture.index == *resource.resource->child_texture_index) {
                        return &texture;
                    }
                }
            }

            return primary_effect_texture(resource);
        }

        [[nodiscard]] std::uint32_t next_jpa_random_u(std::uint32_t &seed) {
            seed = seed * 0x0019660dU + 0x3c6ef35fU;
            return seed;
        }

        [[nodiscard]] float next_jpa_random_f(std::uint32_t &seed) {
            const auto bits = (next_jpa_random_u(seed) >> 9U) | 0x3f800000U;
            return std::bit_cast<float>(bits) - 1.0F;
        }

        [[nodiscard]] float next_jpa_random_zp(std::uint32_t &seed) {
            const auto value = next_jpa_random_f(seed);
            return value + value - 1.0F;
        }

        [[nodiscard]] std::uint32_t jpa_emitter_seed(std::uint16_t user_index, std::uint64_t frame_index) {
            const auto folded_frame = static_cast<std::uint32_t>(frame_index ^ (frame_index >> 32U));
            return 0x4a504100U ^ (static_cast<std::uint32_t>(user_index) << 16U) ^ folded_frame;
        }

        [[nodiscard]] std::uint16_t jpa_particle_lifetime(const JpcDynamicsBlockMetadata &dynamics, std::uint32_t &seed) {
            const auto source_lifetime = std::max<std::int16_t>(dynamics.lifetime, 1);
            const auto lifetime = (1.0F - dynamics.lifetime_random * next_jpa_random_f(seed)) * static_cast<float>(source_lifetime);
            return static_cast<std::uint16_t>(std::max(1.0F, std::floor(lifetime)));
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

    void EffectService::load_resources(const RarcArchive &archive) {
        _resource_library = EffectResourceLibrary::from_archive(archive);
    }

    void EffectService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        advance_effects_to_frame(frame_index);
#ifndef NDEBUG
        _draw_packets.clear();
#endif
    }

    void EffectService::register_keeper(EffectKeeperHostKind host_kind, std::string_view host_name, s32 requested_capacity,
                                        std::string_view resource_group_name, bool sort_enabled) {
        if (host_name.empty()) {
            return;
        }

        auto keeper = EffectKeeperRegistration{
            .host_kind = host_kind,
            .host_name = std::string(host_name),
            .resource_group_name = std::string(resource_group_name),
            .requested_capacity = requested_capacity,
            .sort_enabled = sort_enabled,
            .frame_index = _frame_index,
        };
        _registered_keepers[std::string(host_name)] = keeper;

        if (_resource_library.has_value() && !resource_group_name.empty()) {
            auto resolved = _resource_library->resolve_auto_effect(resource_group_name, resource_group_name);
            if (!resolved.empty()) {
                const auto found = std::ranges::find_if(_active_effects, [host_name, resource_group_name](const auto &active) {
                    return active.actor_name == host_name && active.effect_name == resource_group_name;
                });
                if (found == _active_effects.end()) {
                    auto emitters = create_emitters(resolved);
                    auto &active = _active_effects.emplace_back(ActiveEffectInstance{
                        .actor_name = std::string(host_name),
                        .effect_name = std::string(resource_group_name),
                        .start_frame_index = _frame_index,
                        .keeper = keeper,
                        .resolved_resources = resolved,
                        .emitters = std::move(emitters),
                    });
                    for (auto emitter_index = std::size_t{}; emitter_index < active.emitters.size() &&
                                                                emitter_index < active.resolved_resources.size();
                         ++emitter_index) {
                        advance_emitter_to_frame(active.emitters[emitter_index], active.resolved_resources[emitter_index], _frame_index);
                    }
                    _events.push_back(EffectEvent{
                        .kind = EffectEventKind::Emit,
                        .actor_name = std::string(host_name),
                        .effect_name = std::string(resource_group_name),
                        .frame_index = _frame_index,
                        .keeper = keeper,
                        .resolved_resources = std::move(resolved),
                    });
                }
            }
        }
    }

    void EffectService::unregister_keeper(std::string_view host_name) {
        if (const auto it = _registered_keepers.find(host_name); it != _registered_keepers.end()) {
            _registered_keepers.erase(it);
        }
    }

    void EffectService::emit(std::string_view actor_name, std::string_view effect_name) {
        const auto keeper = registered_keeper(actor_name);
        auto resolved = resolve(actor_name, effect_name);
        const auto found = std::ranges::find_if(_active_effects, [actor_name, effect_name](const auto &active) {
            return active.actor_name == actor_name && active.effect_name == effect_name;
        });
        if (found == _active_effects.end()) {
            auto emitters = create_emitters(resolved);
            auto &active = _active_effects.emplace_back(ActiveEffectInstance{
                .actor_name = std::string(actor_name),
                .effect_name = std::string(effect_name),
                .start_frame_index = _frame_index,
                .keeper = keeper,
                .resolved_resources = resolved,
                .emitters = std::move(emitters),
            });
            for (auto emitter_index = std::size_t{}; emitter_index < active.emitters.size() && emitter_index < active.resolved_resources.size();
                 ++emitter_index) {
                advance_emitter_to_frame(active.emitters[emitter_index], active.resolved_resources[emitter_index], _frame_index);
            }
        } else {
            found->keeper = keeper;
            found->resolved_resources = resolved;
            found->emitters = create_emitters(found->resolved_resources);
            for (auto emitter_index = std::size_t{}; emitter_index < found->emitters.size() && emitter_index < found->resolved_resources.size();
                 ++emitter_index) {
                advance_emitter_to_frame(found->emitters[emitter_index], found->resolved_resources[emitter_index], _frame_index);
            }
        }

        _events.push_back(EffectEvent{
            .kind = EffectEventKind::Emit,
            .actor_name = std::string(actor_name),
            .effect_name = std::string(effect_name),
            .frame_index = _frame_index,
            .keeper = keeper,
            .resolved_resources = std::move(resolved),
        });
    }

    void EffectService::delete_effect(std::string_view actor_name, std::string_view effect_name) {
        std::erase_if(_active_effects, [actor_name, effect_name](const auto &active) {
            return active.actor_name == actor_name && active.effect_name == effect_name;
        });

        _events.push_back(EffectEvent{
            .kind = EffectEventKind::Delete,
            .actor_name = std::string(actor_name),
            .effect_name = std::string(effect_name),
            .frame_index = _frame_index,
            .keeper = registered_keeper(actor_name),
            .resolved_resources = resolve(actor_name, effect_name),
        });
    }

    void EffectService::delete_all(std::string_view actor_name) {
        std::erase_if(_active_effects, [actor_name](const auto &active) { return active.actor_name == actor_name; });
        _events.push_back(EffectEvent{
            .kind = EffectEventKind::DeleteAll,
            .actor_name = std::string(actor_name),
            .effect_name = {},
            .frame_index = _frame_index,
            .keeper = registered_keeper(actor_name),
        });
    }

    void EffectService::draw(render::IRendererEngine &renderer, s32 draw_type) {
        for (const auto &active : _active_effects) {
            for (auto resource_index = std::size_t{}; resource_index < active.resolved_resources.size(); ++resource_index) {
                const auto &resource = active.resolved_resources[resource_index];
                if (!effect_draw_type_accepts_order(draw_type, resource.auto_effect_draw_order)) {
                    continue;
                }
                if (resource_index >= active.emitters.size()) {
                    continue;
                }
                const auto &emitter = active.emitters[resource_index];
                if (emitter.particles.empty()) {
                    continue;
                }

                const auto *primary_texture = primary_effect_texture(resource);
                const auto *child_texture = child_effect_texture(resource);
                if (primary_texture == nullptr || child_texture == nullptr) {
                    continue;
                }

                const auto base_size = effect_billboard_size(*primary_texture, resource);
                for (const auto &particle : emitter.particles) {
                    const auto &texture = particle.child ? *child_texture : *primary_texture;
                    auto texture_handle = texture_handle_for(renderer, texture);
                    if (!texture_handle.is_valid()) {
                        continue;
                    }

                    const auto half_size_x = base_size * particle.scale_x * 0.5F;
                    const auto half_size_y = base_size * particle.scale_y * 0.5F;
                    const auto x = particle.x;
                    const auto y = particle.y;
                    const auto z = particle.z;
                    const auto alpha = static_cast<std::uint8_t>(std::clamp(particle.alpha, 0.0F, 1.0F) * 255.0F);
                    const auto color = std::array<std::uint8_t, 4U>{255U, 255U, 255U, alpha};
                    renderer.submit_textured_quad(texture_handle,
                                                  render::TexturedQuad2D{
                                                      .vertices =
                                                          {
                                                              render::TexturedVertex2D{
                                                                  .x = x - half_size_x,
                                                                  .y = y - half_size_y,
                                                                  .z = z,
                                                                  .u = 0.0F,
                                                                  .v = 1.0F,
                                                                  .color = color,
                                                              },
                                                              render::TexturedVertex2D{
                                                                  .x = x + half_size_x,
                                                                  .y = y - half_size_y,
                                                                  .z = z,
                                                                  .u = 1.0F,
                                                                  .v = 1.0F,
                                                                  .color = color,
                                                              },
                                                              render::TexturedVertex2D{
                                                                  .x = x + half_size_x,
                                                                  .y = y + half_size_y,
                                                                  .z = z,
                                                                  .u = 1.0F,
                                                                  .v = 0.0F,
                                                                  .color = color,
                                                              },
                                                              render::TexturedVertex2D{
                                                                  .x = x - half_size_x,
                                                                  .y = y + half_size_y,
                                                                  .z = z,
                                                                  .u = 0.0F,
                                                                  .v = 0.0F,
                                                                  .color = color,
                                                              },
                                                          },
                                                      .wrap_u = texture.wrap_s,
                                                      .wrap_v = texture.wrap_t,
                                                      .min_filter = texture.min_filter,
                                                      .mag_filter = texture.mag_filter,
                                                      .blend = true,
                                                  });
#ifndef NDEBUG
                    _draw_packets.push_back(EffectDrawPacketTrace{
                        .actor_name = active.actor_name,
                        .effect_name = active.effect_name,
                        .particle_name = resource.particle_name,
                        .user_index = resource.user_index,
                        .draw_order = resource.auto_effect_draw_order,
                        .frame_index = _frame_index,
                        .draw_type = draw_type,
                        .vertex_count = 4U,
                        .index_count = 6U,
                        .particle_id = particle.id,
                        .particle_age = particle.age,
                        .particle_lifetime = particle.lifetime,
                        .live_particle_count = static_cast<std::uint32_t>(emitter.particles.size()),
                        .child_particle = particle.child,
                        .texture =
                            EffectTextureBindingTrace{
                                .slot = static_cast<std::uint8_t>(particle.child ? 1U : 0U),
                                .texture_index = texture.index,
                                .name = texture.name,
                                .width = texture.width,
                                .height = texture.height,
                                .format_raw = static_cast<std::uint32_t>(texture.format),
                                .format_name = texture_format_name(texture.format),
                            },
                    });
#endif
                }
            }
        }
    }

    std::span<const EffectEvent> EffectService::events() const {
        return _events;
    }

    std::span<const ActiveEffectInstance> EffectService::active_effect_instances() const {
        return _active_effects;
    }

    std::vector<EffectKeeperRegistration> EffectService::registered_keepers() const {
        auto out = std::vector<EffectKeeperRegistration>{};
        out.reserve(_registered_keepers.size());
        for (const auto &[_, keeper] : _registered_keepers) {
            out.push_back(keeper);
        }
        return out;
    }

    std::optional<EffectKeeperRegistration> EffectService::registered_keeper(std::string_view host_name) const {
        if (auto it = _registered_keepers.find(host_name); it != _registered_keepers.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    std::vector<std::string> EffectService::active_effects(std::string_view actor_name) const {
        auto out = std::vector<std::string>{};
        for (const auto &active : _active_effects) {
            if (active.actor_name == actor_name) {
                out.push_back(active.effect_name);
            }
        }

        return out;
    }

    const EffectResourceLibrary *EffectService::resource_library() const {
        return _resource_library.has_value() ? &*_resource_library : nullptr;
    }

#ifndef NDEBUG
    std::span<const EffectDrawPacketTrace> EffectService::draw_packets() const {
        return _draw_packets;
    }
#endif

    std::vector<ResolvedEffectResource> EffectService::resolve(std::string_view actor_name, std::string_view effect_name) const {
        if (!_resource_library.has_value() || effect_name.empty()) {
            return {};
        }

        if (const auto keeper = registered_keeper(actor_name); keeper.has_value() && !keeper->resource_group_name.empty()) {
            auto resources = _resource_library->resolve_auto_effect(keeper->resource_group_name, effect_name);
            if (!resources.empty()) {
                return resources;
            }
        }

        return _resource_library->resolve_effect_request(effect_name);
    }

    std::vector<JpcEffectEmitterInstance> EffectService::create_emitters(std::span<const ResolvedEffectResource> resources) const {
        auto emitters = std::vector<JpcEffectEmitterInstance>{};
        emitters.reserve(resources.size());
        for (const auto &resource : resources) {
            emitters.push_back(JpcEffectEmitterInstance{
                .user_index = resource.user_index,
                .particle_name = resource.particle_name,
                .start_frame_index = _frame_index,
                .next_update_frame_index = _frame_index,
                .random_seed = jpa_emitter_seed(resource.user_index, _frame_index),
            });
        }
        return emitters;
    }

    void EffectService::advance_effects_to_frame(std::uint64_t frame_index) {
        for (auto &active : _active_effects) {
            for (auto emitter_index = std::size_t{}; emitter_index < active.emitters.size() && emitter_index < active.resolved_resources.size();
                 ++emitter_index) {
                advance_emitter_to_frame(active.emitters[emitter_index], active.resolved_resources[emitter_index], frame_index);
            }
        }
    }

    void EffectService::advance_emitter_to_frame(JpcEffectEmitterInstance &emitter, const ResolvedEffectResource &resource,
                                                 std::uint64_t frame_index) {
        constexpr auto MAX_PARTICLES_PER_EMITTER = std::size_t{4096U};
        const auto *metadata = resource.resource;
        if (metadata == nullptr || !metadata->dynamics.has_value()) {
            if (emitter.particles.empty() && emitter.next_update_frame_index <= frame_index) {
                emitter.particles.push_back(JpcEffectParticleInstance{
                    .id = emitter.next_particle_id++,
                    .lifetime = 1U,
                    .x = resource.auto_effect_offset_x,
                    .y = -resource.auto_effect_offset_y,
                    .z = resource.auto_effect_offset_z * 0.001F,
                    .alpha = std::clamp(resource.auto_effect_rate_value, 0.0F, 1.0F),
                });
                emitter.next_update_frame_index = frame_index + 1U;
            }
            return;
        }

        const auto &dynamics = *metadata->dynamics;
        while (emitter.next_update_frame_index <= frame_index) {
            const auto local_frame = emitter.next_update_frame_index - emitter.start_frame_index;
            for (auto &particle : emitter.particles) {
                if (particle.age < particle.lifetime) {
                    ++particle.age;
                }
            }
            std::erase_if(emitter.particles, [](const auto &particle) {
                return particle.age >= particle.lifetime;
            });

            if (local_frame >= static_cast<std::uint64_t>(std::max<std::int16_t>(dynamics.start_frame, 0))) {
                const auto emitter_age = local_frame - static_cast<std::uint64_t>(std::max<std::int16_t>(dynamics.start_frame, 0));
                const auto stop_emit = dynamics.max_frame > 0 && emitter_age >= static_cast<std::uint64_t>(dynamics.max_frame);
                auto emit_count = 0;
                if (emitter.rate_step_emit && !stop_emit) {
                    if (dynamics.fixed_interval) {
                        emit_count = std::max<std::uint16_t>(dynamics.div_number, 1U);
                    } else {
                        const auto effective_rate = std::max(0.0F, dynamics.rate * resource.auto_effect_rate_value);
                        const auto new_particle_count = effective_rate * (dynamics.rate_random * next_jpa_random_zp(emitter.random_seed) + 1.0F);
                        emitter.fractional_emit_count += new_particle_count;
                        emit_count = static_cast<int>(emitter.fractional_emit_count);
                        emitter.fractional_emit_count -= static_cast<float>(emit_count);
                        if (emitter.first_emit && new_particle_count > 0.0F && new_particle_count < 1.0F) {
                            emit_count = 1;
                        }
                    }
                }

                for (auto emitted = 0; emitted < emit_count && emitter.particles.size() < MAX_PARTICLES_PER_EMITTER; ++emitted) {
                    const auto lifetime = jpa_particle_lifetime(dynamics, emitter.random_seed);
                    emitter.particles.push_back(JpcEffectParticleInstance{
                        .id = emitter.next_particle_id++,
                        .age = 0U,
                        .lifetime = lifetime,
                        .x = resource.auto_effect_offset_x + dynamics.emitter_translation.x,
                        .y = -(resource.auto_effect_offset_y + dynamics.emitter_translation.y),
                        .z = (resource.auto_effect_offset_z + dynamics.emitter_translation.z) * 0.001F,
                        .alpha = std::clamp(resource.auto_effect_rate_value, 0.0F, 1.0F),
                    });
                }

                if (metadata->child_shape.has_value()) {
                    const auto &child_shape = *metadata->child_shape;
                    const auto child_rate = std::max<std::int16_t>(child_shape.rate, 0);
                    const auto child_lifetime = static_cast<std::uint16_t>(std::max<std::int16_t>(child_shape.lifetime, 1));
                    const auto child_step = static_cast<std::uint16_t>(child_shape.step) + 1U;
                    const auto parent_count = emitter.particles.size();
                    for (auto particle_index = std::size_t{}; particle_index < parent_count; ++particle_index) {
                        const auto &parent = emitter.particles[particle_index];
                        if (parent.child) {
                            continue;
                        }
                        const auto child_start_age = static_cast<int>(
                            std::floor(static_cast<float>(std::max<std::uint16_t>(parent.lifetime, 1U) - 1U) * child_shape.timing));
                        const auto child_time = static_cast<int>(parent.age) - child_start_age;
                        if (child_time < 0 || child_time % child_step != 0) {
                            continue;
                        }

                        for (auto child_index = std::int16_t{}; child_index < child_rate && emitter.particles.size() < MAX_PARTICLES_PER_EMITTER;
                             ++child_index) {
                            const auto scale_inherit = child_shape.scale_inherited ? child_shape.inherit_scale : 1.0F;
                            const auto alpha_inherit = child_shape.alpha_inherited ? child_shape.inherit_alpha : 1.0F;
                            emitter.particles.push_back(JpcEffectParticleInstance{
                                .id = emitter.next_particle_id++,
                                .age = 0U,
                                .lifetime = child_lifetime,
                                .child = true,
                                .x = parent.x,
                                .y = parent.y,
                                .z = parent.z,
                                .scale_x = std::max(0.01F, child_shape.scale_x * scale_inherit),
                                .scale_y = std::max(0.01F, child_shape.scale_y * scale_inherit),
                                .alpha = std::clamp(parent.alpha * alpha_inherit, 0.0F, 1.0F),
                            });
                        }
                    }
                }

                const auto rate_step_period = static_cast<std::uint16_t>(dynamics.rate_step) + 1U;
                ++emitter.rate_step_timer;
                if (emitter.rate_step_timer >= rate_step_period) {
                    emitter.rate_step_timer = static_cast<std::uint16_t>(emitter.rate_step_timer - rate_step_period);
                    emitter.rate_step_emit = true;
                } else {
                    emitter.rate_step_emit = false;
                }
                emitter.first_emit = false;
            }

            ++emitter.next_update_frame_index;
        }
    }

    render::TextureHandle EffectService::texture_handle_for(render::IRendererEngine &renderer, const JpcTextureMetadata &texture) {
        if (const auto it = _texture_handles.find(texture.index); it != _texture_handles.end() && it->second.is_valid()) {
            return it->second;
        }

        if (texture.image.rgba.empty() || texture.image.width == 0U || texture.image.height == 0U) {
            return {};
        }

        auto handle = renderer.create_rgba8_texture(texture.image.width, texture.image.height,
                                                    std::span<const std::uint8_t>(texture.image.rgba.data(), texture.image.rgba.size()));
        if (handle.is_valid()) {
            _texture_handles[texture.index] = handle;
        }
        return handle;
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

    void CameraSystemService::declare_event_camera_programmable(std::string_view name) {
        if (name.empty()) {
            return;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        ++_programmable_camera_declare_count;
    }

    void CameraSystemService::start_global_event_camera_no_target(std::string_view name) {
        if (name.empty()) {
            return;
        }

        if (auto *event = find_programmable_event(_active_programmable_camera_name)) {
            event->active = false;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        event.active = true;
        _active_programmable_camera_name = std::string(name);
        ++_programmable_camera_start_count;
    }

    void CameraSystemService::end_global_event_camera(std::string_view name) {
        if (name.empty()) {
            return;
        }

        if (auto *event = find_programmable_event(name)) {
            event->active = false;
        }
        if (_active_programmable_camera_name == name) {
            _active_programmable_camera_name.clear();
        }
        ++_programmable_camera_end_count;
    }

    std::optional<CameraPoseCompat> CameraSystemService::set_programmable_camera_param(std::string_view name, const CameraParamVec3 &watch,
                                                                                       const CameraParamVec3 &eye, const CameraParamVec3 &up,
                                                                                       bool do_zero_w_offset) {
        (void)do_zero_w_offset;
        if (name.empty()) {
            return std::nullopt;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        event.pose.eye = eye;
        event.pose.watch = watch;
        event.pose.up = up;
        event.has_pose = true;
        ++_programmable_camera_param_count;

        return active_programmable_camera_pose_for(name);
    }

    std::optional<CameraPoseCompat> CameraSystemService::set_programmable_camera_fovy(std::string_view name, float fovy_degrees) {
        if (name.empty()) {
            return std::nullopt;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        event.pose.fovy_degrees = fovy_degrees;
        ++_programmable_camera_fovy_count;

        return active_programmable_camera_pose_for(name);
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

    std::optional<CameraPoseCompat> CameraSystemService::active_programmable_camera_pose() const {
        return active_programmable_camera_pose_for(_active_programmable_camera_name);
    }

    std::optional<std::string_view> CameraSystemService::active_programmable_camera_name() const {
        if (_active_programmable_camera_name.empty()) {
            return std::nullopt;
        }

        return std::string_view(_active_programmable_camera_name);
    }

    std::uint32_t CameraSystemService::programmable_camera_declare_count() const {
        return _programmable_camera_declare_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_start_count() const {
        return _programmable_camera_start_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_end_count() const {
        return _programmable_camera_end_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_param_count() const {
        return _programmable_camera_param_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_fovy_count() const {
        return _programmable_camera_fovy_count;
    }

    CameraSystemService::ProgrammableCameraEventState *CameraSystemService::find_programmable_event(std::string_view name) {
        const auto it = _programmable_camera_events.find(std::string(name));
        return it == _programmable_camera_events.end() ? nullptr : &it->second;
    }

    const CameraSystemService::ProgrammableCameraEventState *CameraSystemService::find_programmable_event(std::string_view name) const {
        const auto it = _programmable_camera_events.find(std::string(name));
        return it == _programmable_camera_events.end() ? nullptr : &it->second;
    }

    std::optional<CameraPoseCompat> CameraSystemService::active_programmable_camera_pose_for(std::string_view name) const {
        if (name.empty() || _active_programmable_camera_name != name) {
            return std::nullopt;
        }

        const auto *event = find_programmable_event(name);
        if (event == nullptr || !event->active || !event->has_pose) {
            return std::nullopt;
        }

        return event->pose;
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
            auto state = SlotState{};
            state.slot_index = slot_index;
            _slot_states.push_back(std::move(state));
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

        auto state = SlotState{};
        state.slot_index = slot_index;
        return state;
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
