#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <revolution.h>

#include "Game/compat/RarcArchive.hpp"

namespace smgpc::game {

    class DvdFileSystemService final {
    public:
        explicit DvdFileSystemService(std::filesystem::path root);

        [[nodiscard]] const std::filesystem::path &root() const;
        [[nodiscard]] std::filesystem::path resolve(std::string_view disc_path) const;
        [[nodiscard]] bool exists(std::string_view disc_path) const;
        [[nodiscard]] std::optional<std::filesystem::path> find_first(std::initializer_list<std::filesystem::path> candidates) const;
        [[nodiscard]] std::optional<std::filesystem::path> find_layout_archive(std::string_view layout_name) const;
        [[nodiscard]] std::optional<std::filesystem::path> find_object_archive(std::string_view object_name) const;
        [[nodiscard]] std::vector<std::uint8_t> read_file(std::string_view disc_path) const;
        [[nodiscard]] RarcArchive &archive(std::string_view disc_path);
        [[nodiscard]] std::size_t archive_load_count(std::string_view disc_path) const;
        [[nodiscard]] std::size_t cached_archive_count() const;

    private:
        [[nodiscard]] std::filesystem::path normalize_disc_path(std::string_view disc_path) const;
        [[nodiscard]] std::string archive_cache_key(std::string_view disc_path) const;

        std::filesystem::path _root;
        std::map<std::string, std::unique_ptr<RarcArchive>> _archives;
        std::map<std::string, std::size_t> _archive_load_counts;
    };

    struct WpadPointerState {
        float x = 0.0F;
        float y = 0.0F;
        bool valid = false;
    };

    struct WpadVec3State {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
    };

    struct WpadStickState {
        float x = 0.0F;
        float y = 0.0F;
    };

    struct WpadChannelState {
        bool connected = false;
        std::uint32_t previous_hold = 0U;
        std::uint32_t hold = 0U;
        std::uint32_t trigger = 0U;
        std::uint32_t release = 0U;
        std::uint32_t repeat = 0U;
        std::uint32_t hold_frame_count = 0U;
        WpadPointerState pointer{};
        std::array<WpadPointerState, 16U> pointer_history{};
        std::uint32_t pointer_history_count = 0U;
        WpadVec3State core_acceleration{};
        WpadVec3State sub_acceleration{};
        WpadStickState sub_stick{};
        bool core_swing = false;
        bool previous_core_swing = false;
        bool sub_swing = false;
        bool previous_sub_swing = false;
        float distance_to_display = 0.0F;
    };

    class WpadService final {
    public:
        void begin_frame();
        void set_connected(s32 channel, bool connected);
        void set_button_mask(s32 channel, std::uint32_t hold);
        void set_pointer(s32 channel, float x, float y, bool valid);
        void set_sub_stick(s32 channel, float x, float y);
        void set_core_acceleration(s32 channel, float x, float y, float z);
        void set_sub_acceleration(s32 channel, float x, float y, float z);
        void set_swing(s32 channel, bool core_swing, bool sub_swing);
        void set_distance_to_display(s32 channel, float distance);

        [[nodiscard]] bool is_connected(s32 channel) const;
        [[nodiscard]] bool is_button_held(s32 channel, std::uint32_t button_mask) const;
        [[nodiscard]] bool is_button_triggered(s32 channel, std::uint32_t button_mask) const;
        [[nodiscard]] bool is_button_released(s32 channel, std::uint32_t button_mask) const;
        [[nodiscard]] bool is_button_repeated(s32 channel, std::uint32_t button_mask) const;
        [[nodiscard]] WpadPointerState pointer(s32 channel) const;
        [[nodiscard]] WpadPointerState past_pointer(s32 channel, std::uint32_t index) const;
        [[nodiscard]] std::uint32_t pointer_history_count(s32 channel) const;
        [[nodiscard]] WpadStickState sub_stick(s32 channel) const;
        [[nodiscard]] WpadVec3State core_acceleration(s32 channel) const;
        [[nodiscard]] WpadVec3State sub_acceleration(s32 channel) const;
        [[nodiscard]] bool is_core_swing(s32 channel) const;
        [[nodiscard]] bool is_core_swing_triggered(s32 channel) const;
        [[nodiscard]] bool is_sub_swing(s32 channel) const;
        [[nodiscard]] float distance_to_display(s32 channel) const;
        [[nodiscard]] const WpadChannelState *channel_state(s32 channel) const;

    private:
        [[nodiscard]] WpadChannelState *mutable_channel_state(s32 channel);

        std::array<WpadChannelState, WPAD_MAX_CONTROLLERS> _channels{};
    };

    enum class AudioEventKind {
        StageBgmStart,
        StageBgmUnlock,
        StageBgmStop,
        SystemSoundStart,
        ControllerSpeakerSoundStart,
    };

    struct AudioEvent {
        AudioEventKind kind = AudioEventKind::StageBgmStart;
        std::string name;
        s32 fade_frames = 0;
        std::uint64_t frame_index = 0U;
    };

    class AudioEventService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void start_stage_bgm(std::string_view name);
        void unlock_stage_bgm();
        void stop_stage_bgm(s32 fade_frames);
        void start_system_sound(std::string_view name);
        void start_controller_speaker_sound(std::string_view name);

        [[nodiscard]] bool is_stage_bgm_prepared() const;
        [[nodiscard]] bool is_stage_bgm_unlocked() const;
        [[nodiscard]] std::string_view current_stage_bgm_name() const;
        [[nodiscard]] std::span<const AudioEvent> events() const;

    private:
        void push_event(AudioEventKind kind, std::string_view name, s32 fade_frames = 0);

        std::uint64_t _frame_index = 0U;
        std::uint64_t _stage_bgm_start_frame = 0U;
        std::string _stage_bgm_name;
        bool _stage_bgm_requested = false;
        bool _stage_bgm_unlocked = false;
        std::vector<AudioEvent> _events;
    };

    enum class EffectEventKind {
        Emit,
        DeleteAll,
    };

    struct EffectEvent {
        EffectEventKind kind = EffectEventKind::Emit;
        std::string actor_name;
        std::string effect_name;
        std::uint64_t frame_index = 0U;
    };

    class EffectService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void emit(std::string_view actor_name, std::string_view effect_name);
        void delete_all(std::string_view actor_name);

        [[nodiscard]] std::span<const EffectEvent> events() const;
        [[nodiscard]] std::vector<std::string> active_effects(std::string_view actor_name) const;

    private:
        std::uint64_t _frame_index = 0U;
        std::vector<EffectEvent> _events;
        std::map<std::string, std::vector<std::string>> _active_effects;
    };

    class SaveDataService final {
    public:
        void write_file(std::string_view name, std::span<const std::uint8_t> bytes);
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_file(std::string_view name) const;
        [[nodiscard]] bool exists(std::string_view name) const;
        bool erase(std::string_view name);
        [[nodiscard]] std::size_t file_count() const;

    private:
        std::map<std::string, std::vector<std::uint8_t>> _files;
    };

    class MessageService final {
    public:
        void set_message(std::string_view tag, std::string_view text);
        [[nodiscard]] const std::string *message(std::string_view tag) const;
        [[nodiscard]] std::string message_or(std::string_view tag, std::string_view fallback) const;

    private:
        std::map<std::string, std::string> _messages;
    };

    struct RflMiiEntry {
        s32 index = 0;
        std::string name;
    };

    class RflService final {
    public:
        RflService();

        void set_initialized(bool initialized);
        void set_error(bool error);
        void set_miis(std::vector<RflMiiEntry> miis);

        [[nodiscard]] bool is_initialized() const;
        [[nodiscard]] bool has_error() const;
        [[nodiscard]] std::span<const RflMiiEntry> valid_miis() const;

    private:
        bool _initialized = true;
        bool _error = false;
        std::vector<RflMiiEntry> _miis;
    };

}  // namespace smgpc::game
