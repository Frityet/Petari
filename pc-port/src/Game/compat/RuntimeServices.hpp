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

#include "Game/compat/GXState.hpp"
#include "Game/compat/RarcArchive.hpp"

class UserFile;

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
        [[nodiscard]] RarcArchive &archive_for_path(const std::filesystem::path &path);
        [[nodiscard]] std::size_t archive_load_count(std::string_view disc_path) const;
        [[nodiscard]] std::size_t archive_load_count_for_path(const std::filesystem::path &path) const;
        [[nodiscard]] std::size_t cached_archive_count() const;

    private:
        [[nodiscard]] std::filesystem::path normalize_disc_path(std::string_view disc_path) const;
        [[nodiscard]] std::string archive_cache_key_for_path(const std::filesystem::path &path) const;
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
        StageBgmStateChange,
        SystemSoundStart,
        ControllerSpeakerSoundStart,
    };

    struct AudioEvent {
        AudioEventKind kind = AudioEventKind::StageBgmStart;
        std::string name;
        s32 fade_frames = 0;
        s32 state = 0;
        u32 change_frames = 0U;
        std::uint64_t frame_index = 0U;
    };

    class AudioEventService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void start_stage_bgm(std::string_view name);
        void unlock_stage_bgm();
        void stop_stage_bgm(s32 fade_frames);
        void set_stage_bgm_state(s32 state, u32 change_frames);
        void start_system_sound(std::string_view name);
        void start_controller_speaker_sound(std::string_view name);

        [[nodiscard]] bool is_stage_bgm_prepared() const;
        [[nodiscard]] bool is_stage_bgm_unlocked() const;
        [[nodiscard]] std::string_view current_stage_bgm_name() const;
        [[nodiscard]] s32 stage_bgm_state() const;
        [[nodiscard]] u32 stage_bgm_state_change_frames() const;
        [[nodiscard]] std::span<const AudioEvent> events() const;

    private:
        void push_event(AudioEventKind kind, std::string_view name, s32 fade_frames = 0, s32 state = 0, u32 change_frames = 0U);

        std::uint64_t _frame_index = 0U;
        std::uint64_t _stage_bgm_start_frame = 0U;
        std::string _stage_bgm_name;
        s32 _stage_bgm_state = 0;
        u32 _stage_bgm_state_change_frames = 0U;
        bool _stage_bgm_requested = false;
        bool _stage_bgm_unlocked = false;
        std::vector<AudioEvent> _events;
    };

    enum class EffectEventKind {
        Emit,
        Delete,
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
        void delete_effect(std::string_view actor_name, std::string_view effect_name);
        void delete_all(std::string_view actor_name);

        [[nodiscard]] std::span<const EffectEvent> events() const;
        [[nodiscard]] std::vector<std::string> active_effects(std::string_view actor_name) const;

    private:
        std::uint64_t _frame_index = 0U;
        std::vector<EffectEvent> _events;
        std::map<std::string, std::vector<std::string>> _active_effects;
    };

    enum class WipeEventKind {
        Open,
        Close,
        ForceOpen,
        ForceClose,
    };

    enum class WipeState {
        Open,
        Closed,
        Opening,
        Closing,
    };

    struct WipeEvent {
        WipeEventKind kind = WipeEventKind::Open;
        std::string name;
        s32 frame_count = 0;
        std::uint64_t frame_index = 0U;
    };

    class WipeService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void open(std::string_view name, s32 frame_count);
        void close(std::string_view name, s32 frame_count);
        void force_open(std::string_view name);
        void force_close(std::string_view name);

        [[nodiscard]] bool is_active() const;
        [[nodiscard]] bool is_blank() const;
        [[nodiscard]] bool is_open() const;
        [[nodiscard]] WipeState state() const;
        [[nodiscard]] std::string_view current_name() const;
        [[nodiscard]] s32 remaining_frames() const;
        [[nodiscard]] s32 duration_frames() const;
        [[nodiscard]] std::span<const WipeEvent> events() const;

    private:
        void start_transition(WipeEventKind kind, WipeState state, std::string_view name, s32 frame_count);
        void push_event(WipeEventKind kind, std::string_view name, s32 frame_count);
        [[nodiscard]] static s32 normalized_frame_count(s32 frame_count);

        std::uint64_t _frame_index = 0U;
        WipeState _state = WipeState::Open;
        std::string _current_name;
        s32 _remaining_frames = 0;
        s32 _duration_frames = 0;
        std::vector<WipeEvent> _events;
    };

    enum class StarPointerMode {
        None,
        Title,
        FileSelect,
        SaveLoad,
        PictureBook,
    };

    struct StarPointerModeEvent {
        StarPointerMode mode = StarPointerMode::None;
        std::uint64_t frame_index = 0U;
    };

    class StarPointerService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void start_mode(StarPointerMode mode);
        void set_guidance_active(bool active);
        void request_file_select_guidance();
        void request_file_select_copy_guidance();

        [[nodiscard]] StarPointerMode mode() const;
        [[nodiscard]] bool is_guidance_active() const;
        [[nodiscard]] bool is_file_select_guidance_requested() const;
        [[nodiscard]] bool is_file_select_copy_guidance_requested() const;
        [[nodiscard]] std::span<const StarPointerModeEvent> mode_events() const;

    private:
        std::uint64_t _frame_index = 0U;
        StarPointerMode _mode = StarPointerMode::None;
        bool _guidance_active = false;
        bool _file_select_guidance_requested = false;
        bool _file_select_copy_guidance_requested = false;
        std::vector<StarPointerModeEvent> _mode_events;
    };

    class CameraSystemService final {
    public:
        void reset_camera_man();
        void request_normal_shake();
        void pause_on_camera_director();
        void pause_off_camera_director();

        [[nodiscard]] std::uint32_t reset_camera_man_count() const;
        [[nodiscard]] std::uint32_t normal_shake_request_count() const;
        [[nodiscard]] std::uint32_t camera_director_pause_count() const;
        [[nodiscard]] bool is_camera_director_paused() const;

    private:
        std::uint32_t _reset_camera_man_count = 0U;
        std::uint32_t _normal_shake_request_count = 0U;
        std::uint32_t _camera_director_pause_count = 0U;
    };

    class PlayerSystemService final {
    public:
        void hide_player();
        void set_base_matrix(MtxPtr matrix);

        [[nodiscard]] bool is_player_hidden() const;
        [[nodiscard]] bool has_base_matrix() const;
        [[nodiscard]] std::span<const f32, 12U> base_matrix() const;

    private:
        bool _player_hidden = false;
        bool _has_base_matrix = false;
        std::array<f32, 12U> _base_matrix{};
    };

    class GameLayoutService final {
    public:
        void deactivate_default_game_layout();
        void activate_game_scene_draw_3d();
        void deactivate_game_scene_draw_3d();

        [[nodiscard]] bool is_default_game_layout_active() const;
        [[nodiscard]] bool is_game_scene_draw_3d_active() const;

    private:
        bool _default_game_layout_active = true;
        bool _game_scene_draw_3d_active = true;
    };

    enum class RumbleRequestKind {
        Strong,
        Weak,
    };

    struct RumbleRequestEvent {
        RumbleRequestKind kind = RumbleRequestKind::Strong;
        s32 channel = 0;
        std::uint64_t frame_index = 0U;
    };

    class RumbleService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void request_strong(s32 channel);
        void request_weak(s32 channel);

        [[nodiscard]] std::span<const RumbleRequestEvent> events() const;

    private:
        void push_event(RumbleRequestKind kind, s32 channel);

        std::uint64_t _frame_index = 0U;
        std::vector<RumbleRequestEvent> _events;
    };

    enum class SequenceRequestKind {
        ChangeStageInGameAfterLoadingGameData,
    };

    struct SequenceRequestEvent {
        SequenceRequestKind kind = SequenceRequestKind::ChangeStageInGameAfterLoadingGameData;
        std::uint64_t frame_index = 0U;
    };

    class SequenceRequestService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void request_change_stage_in_game_after_loading_game_data();

        [[nodiscard]] bool is_change_stage_in_game_after_loading_game_data_requested() const;
        [[nodiscard]] std::span<const SequenceRequestEvent> events() const;

    private:
        std::uint64_t _frame_index = 0U;
        bool _change_stage_in_game_after_loading_game_data_requested = false;
        std::vector<SequenceRequestEvent> _events;
    };

    class SaveDataService final {
    public:
        struct SlotState {
            s32 slot_index = 0;
            bool created = false;
            bool game_data_corrupted = false;
            bool config_data_corrupted = false;
            bool last_loaded_mario = true;
            s32 power_star_num = 0;
            s32 star_piece_num = 0;
            s32 player_miss_num = 0;
            bool has_mii_id = false;
            std::optional<s32> rfl_mii_index{};
            std::optional<u32> icon_id{};
            bool view_normal_ending = false;
            bool view_complete_ending = false;
            bool complete_ending_mario_and_luigi = false;
            std::map<std::string, bool> game_event_flags;
            std::map<std::string, u16> game_event_values;
            OSTime last_modified = 0;
        };

        SaveDataService();

        void write_file(std::string_view name, std::span<const std::uint8_t> bytes);
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_file(std::string_view name) const;
        [[nodiscard]] bool exists(std::string_view name) const;
        bool erase(std::string_view name);
        [[nodiscard]] std::size_t file_count() const;
        void set_host_directory(std::filesystem::path directory);
        [[nodiscard]] const std::optional<std::filesystem::path> &host_directory() const;
        void load_host_files();
        void flush_host_files();
        [[nodiscard]] const SlotState *slot_state(s32 slot_index) const;
        [[nodiscard]] SlotState slot_state_or_default(s32 slot_index) const;
        void set_slot_state(s32 slot_index, const SlotState &state);
        void copy_slot_state(s32 dst_slot_index, s32 src_slot_index);
        void clear_slot_states();
        [[nodiscard]] std::span<const SlotState> slot_states() const;
        void restore_user_file(UserFile &file, s32 slot_index, bool is_player_mario) const;
        void store_user_file(s32 slot_index, const UserFile &file);
        void set_sys_config_time_announced(OSTime time);
        void update_sys_config_time_announced();
        [[nodiscard]] OSTime sys_config_time_announced() const;
        void set_sys_config_time_sent(OSTime time);
        [[nodiscard]] OSTime sys_config_time_sent() const;
        void set_sys_config_sent_bytes(u32 bytes);
        [[nodiscard]] u32 sys_config_sent_bytes() const;

    private:
        [[nodiscard]] std::filesystem::path host_file_path(std::string_view name) const;
        void write_host_file(std::string_view name, std::span<const std::uint8_t> bytes) const;
        void erase_host_file(std::string_view name) const;
        void load_slot_states_from_files();
        void load_sys_config_from_files();
        void write_sys_config_file();

        std::map<std::string, std::vector<std::uint8_t>> _files;
        std::vector<SlotState> _slot_states;
        std::optional<std::filesystem::path> _host_directory{};
        OSTime _sys_config_time_announced = 0;
        OSTime _sys_config_time_sent = 0;
        u32 _sys_config_sent_bytes = 0U;
    };

    class MessageService final {
    public:
        void set_message(std::string_view tag, std::string_view text);
        void set_message(std::string_view tag, std::u16string_view text);
        std::size_t load_message_archive(const RarcArchive &archive);
        [[nodiscard]] std::size_t message_count() const;
        [[nodiscard]] const std::string *message(std::string_view tag) const;
        [[nodiscard]] const std::u16string *message_utf16(std::string_view tag) const;
        [[nodiscard]] std::string message_or(std::string_view tag, std::string_view fallback) const;
        [[nodiscard]] std::u16string message_utf16_or(std::string_view tag, std::u16string_view fallback) const;

    private:
        struct MessageText {
            std::u16string utf16;
            std::string utf8;
        };

        std::map<std::string, MessageText> _messages;
    };

    class SceneLightService final {
    public:
        void clear();
        void clear_light(std::size_t index);
        void set_light(std::size_t index, const GXLightState &light);

        [[nodiscard]] const GXLightState *light(std::size_t index) const;
        [[nodiscard]] std::span<const GXLightState> lights() const;
        [[nodiscard]] std::uint8_t loaded_mask() const;

    private:
        std::array<GXLightState, 8U> _lights{};
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
