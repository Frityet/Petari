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

#include "Game/compat/BmgMessageArchive.hpp"
#include "Game/compat/CameraPose.hpp"
#include "Game/compat/EffectResourceCompat.hpp"
#include "Game/compat/GXState.hpp"
#include "Game/compat/RarcArchive.hpp"
#include "RendererService.hpp"

class UserFile;
class LiveActor;

namespace smgpc::game {

    struct DvdFileReadTrace {
        std::string requested_path;
        std::string resolved_path;
        std::size_t byte_count = 0U;
    };

    struct DvdArchiveLoadTrace {
        std::string requested_path;
        std::string resolved_path;
        bool cache_hit = false;
        std::size_t load_count = 0U;
        std::size_t cached_archive_count = 0U;
        std::size_t resource_count = 0U;
    };

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
        [[nodiscard]] std::span<const DvdFileReadTrace> file_read_trace() const;
        [[nodiscard]] std::span<const DvdArchiveLoadTrace> archive_load_trace() const;
        void clear_trace();

    private:
        [[nodiscard]] std::filesystem::path normalize_disc_path(std::string_view disc_path) const;
        [[nodiscard]] std::string archive_cache_key_for_path(const std::filesystem::path &path) const;
        [[nodiscard]] std::string archive_cache_key(std::string_view disc_path) const;
        [[nodiscard]] RarcArchive &archive_for_path_with_request(const std::filesystem::path &path, std::string_view requested_path);

        std::filesystem::path _root;
        std::map<std::string, std::unique_ptr<RarcArchive>> _archives;
        std::map<std::string, std::size_t> _archive_load_counts;
        mutable std::vector<DvdFileReadTrace> _file_read_trace;
        std::vector<DvdArchiveLoadTrace> _archive_load_trace;
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
        SystemSoundStop,
        SystemLevelSoundStart,
        LevelSoundSubmit,
        LevelSoundPermit,
        AtmosphereSoundStart,
        SystemMEStart,
        ControllerSpeakerSoundStart,
    };

    struct AudioEvent {
        AudioEventKind kind = AudioEventKind::StageBgmStart;
        std::string name;
        s32 fade_frames = 0;
        s32 state = 0;
        u32 change_frames = 0U;
        u32 delay_frames = 0U;
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
        void stop_system_sound(std::string_view name, u32 delay_frames);
        void start_system_level_sound(std::string_view name);
        void submit_level_sound();
        void permit_level_sound();
        void start_atmosphere_sound(std::string_view name);
        void start_system_me(std::string_view name);
        void start_controller_speaker_sound(std::string_view name);

        [[nodiscard]] bool is_stage_bgm_prepared() const;
        [[nodiscard]] bool is_stage_bgm_unlocked() const;
        [[nodiscard]] std::string_view current_stage_bgm_name() const;
        [[nodiscard]] s32 stage_bgm_state() const;
        [[nodiscard]] u32 stage_bgm_state_change_frames() const;
        [[nodiscard]] std::span<const AudioEvent> events() const;

    private:
        void push_event(AudioEventKind kind, std::string_view name, s32 fade_frames = 0, s32 state = 0, u32 change_frames = 0U,
                        u32 delay_frames = 0U);

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

    enum class EffectKeeperHostKind {
        LiveActor,
        LayoutActor,
        SimpleLayout,
    };

    struct EffectKeeperRegistration {
        EffectKeeperHostKind host_kind = EffectKeeperHostKind::LiveActor;
        std::string host_name;
        std::string resource_group_name;
        s32 requested_capacity = 0;
        bool sort_enabled = false;
        std::uint64_t frame_index = 0U;
    };

    struct EffectEvent {
        EffectEventKind kind = EffectEventKind::Emit;
        std::string actor_name;
        std::string effect_name;
        std::uint64_t frame_index = 0U;
        std::optional<EffectKeeperRegistration> keeper;
        std::vector<ResolvedEffectResource> resolved_resources;
    };

    enum class EffectHostBindingSource {
        LiveActorBaseMatrix,
        LayoutActorTransform,
        SimpleLayoutOrigin,
    };

    struct EffectHostBinding {
        EffectKeeperHostKind host_kind = EffectKeeperHostKind::LiveActor;
        EffectHostBindingSource source = EffectHostBindingSource::LiveActorBaseMatrix;
        std::string host_name;
        std::array<float, 12U> matrix{};
        std::array<float, 3U> translation{};
        bool host_dead = false;
        std::uint64_t frame_index = 0U;
    };

    struct JpcEffectParticleInstance {
        std::uint32_t id = 0U;
        std::uint16_t age = 0U;
        std::uint16_t lifetime = 1U;
        bool child = false;
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        float scale_x = 1.0F;
        float scale_y = 1.0F;
        float alpha = 1.0F;
    };

    struct JpcEffectEmitterInstance {
        std::uint16_t user_index = 0U;
        std::string particle_name;
        std::uint64_t start_frame_index = 0U;
        std::uint64_t next_update_frame_index = 0U;
        std::uint32_t random_seed = 0U;
        float fractional_emit_count = 0.0F;
        std::uint16_t rate_step_timer = 0U;
        bool first_emit = true;
        bool rate_step_emit = true;
        std::uint32_t next_particle_id = 0U;
        std::vector<JpcEffectParticleInstance> particles;
    };

    struct ActiveEffectInstance {
        std::string actor_name;
        std::string effect_name;
        std::uint64_t start_frame_index = 0U;
        std::optional<EffectKeeperRegistration> keeper;
        std::optional<EffectHostBinding> host_binding;
        std::vector<ResolvedEffectResource> resolved_resources;
        std::vector<JpcEffectEmitterInstance> emitters;
    };

#ifndef NDEBUG
    struct EffectTextureBindingTrace {
        std::uint8_t slot = 0U;
        std::uint16_t texture_index = 0U;
        std::string name;
        std::uint16_t width = 0U;
        std::uint16_t height = 0U;
        std::uint32_t format_raw = 0U;
        std::string format_name;
    };

    struct EffectDrawPacketTrace {
        std::string actor_name;
        std::string effect_name;
        std::string particle_name;
        std::uint16_t user_index = 0U;
        std::string draw_order;
        std::uint64_t frame_index = 0U;
        s32 draw_type = 0;
        std::string primitive_type = "triangles";
        std::uint32_t vertex_count = 0U;
        std::uint32_t index_count = 0U;
        std::uint32_t color_channel_count = 1U;
        std::uint32_t particle_id = 0U;
        std::uint16_t particle_age = 0U;
        std::uint16_t particle_lifetime = 0U;
        bool host_binding_found = false;
        std::string host_binding_source;
        std::array<float, 3U> host_translation = {};
        float particle_x = 0.0F;
        float particle_y = 0.0F;
        float particle_z = 0.0F;
        float particle_scale_x = 1.0F;
        float particle_scale_y = 1.0F;
        float particle_alpha = 1.0F;
        std::uint32_t live_particle_count = 0U;
        bool child_particle = false;
        bool alpha_compare_enabled = false;
        bool blend_enabled = true;
        EffectTextureBindingTrace texture = {};
    };
#endif

    class EffectService final {
    public:
        void load_resources(const RarcArchive &archive);
        void begin_frame(std::uint64_t frame_index);
        void register_keeper(EffectKeeperHostKind host_kind, std::string_view host_name, s32 requested_capacity,
                             std::string_view resource_group_name, bool sort_enabled);
        void unregister_keeper(std::string_view host_name);
        void bind_host_transform(EffectKeeperHostKind host_kind, std::string_view host_name, EffectHostBindingSource source,
                                 const std::array<float, 12U> &matrix, bool host_dead);
        void unbind_host_transform(std::string_view host_name);
        void emit(std::string_view actor_name, std::string_view effect_name);
        void delete_effect(std::string_view actor_name, std::string_view effect_name);
        void delete_all(std::string_view actor_name);
        void draw(render::IRendererEngine &renderer, s32 draw_type);

        [[nodiscard]] std::span<const EffectEvent> events() const;
        [[nodiscard]] std::span<const ActiveEffectInstance> active_effect_instances() const;
        [[nodiscard]] std::vector<EffectKeeperRegistration> registered_keepers() const;
        [[nodiscard]] std::optional<EffectKeeperRegistration> registered_keeper(std::string_view host_name) const;
        [[nodiscard]] std::optional<EffectHostBinding> host_binding(std::string_view host_name) const;
        [[nodiscard]] std::vector<std::string> active_effects(std::string_view actor_name) const;
        [[nodiscard]] const EffectResourceLibrary *resource_library() const;
#ifndef NDEBUG
        [[nodiscard]] std::span<const EffectDrawPacketTrace> draw_packets() const;
#endif

    private:
        [[nodiscard]] std::vector<ResolvedEffectResource> resolve(std::string_view actor_name, std::string_view effect_name) const;
        [[nodiscard]] render::TextureHandle texture_handle_for(render::IRendererEngine &renderer, const JpcTextureMetadata &texture);
        [[nodiscard]] std::vector<JpcEffectEmitterInstance> create_emitters(std::span<const ResolvedEffectResource> resources);
        void advance_effects_to_frame(std::uint64_t frame_index);
        void advance_emitter_to_frame(JpcEffectEmitterInstance &emitter, const ResolvedEffectResource &resource, std::uint64_t frame_index);

        std::uint64_t _frame_index = 0U;
        std::vector<EffectEvent> _events;
        std::vector<ActiveEffectInstance> _active_effects;
        std::map<std::string, EffectKeeperRegistration, std::less<>> _registered_keepers;
        std::map<std::string, EffectHostBinding, std::less<>> _host_bindings;
        std::optional<EffectResourceLibrary> _resource_library;
        std::map<std::uint16_t, render::TextureHandle> _texture_handles;
        std::uint32_t _emitter_random_seed = 0U;
#ifndef NDEBUG
        std::vector<EffectDrawPacketTrace> _draw_packets;
#endif
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

    enum class ImageEffectControlKind {
        ForceOff,
        ControlAuto,
    };

    struct ImageEffectControlEvent {
        ImageEffectControlKind kind = ImageEffectControlKind::ForceOff;
        std::uint64_t frame_index = 0U;
    };

    class ImageEffectService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void force_off();
        void set_control_auto();

        [[nodiscard]] bool is_forced_off() const;
        [[nodiscard]] bool is_control_auto() const;
        [[nodiscard]] std::span<const ImageEffectControlEvent> events() const;

    private:
        void push_event(ImageEffectControlKind kind);

        std::uint64_t _frame_index = 0U;
        bool _forced_off = false;
        bool _control_auto = true;
        std::vector<ImageEffectControlEvent> _events;
    };

    enum class StarPointerMode {
        None,
        ScreenMenu,
        TargetSelection,
        SystemModal,
        DocumentViewer,
    };

    enum class StarPointerGuidanceRequest {
        None,
        Primary,
        Secondary,
    };

    struct StarPointerModeEvent {
        StarPointerMode mode = StarPointerMode::None;
        std::uint64_t frame_index = 0U;
    };

#ifndef NDEBUG
    enum class StarPointerTargetEventKind {
        Enter,
        Leave,
        Select,
    };

    struct StarPointerTargetEvent {
        StarPointerTargetEventKind kind = StarPointerTargetEventKind::Enter;
        std::string actor_name;
        std::uint64_t frame_index = 0U;
        s32 channel = WPAD_CHAN0;
        float pointer_x = 0.0F;
        float pointer_y = 0.0F;
        float target_x = 0.0F;
        float target_y = 0.0F;
        float projected_radius = 0.0F;
        bool check_z = false;
    };
#endif

    struct StarPointerTargetState {
        const LiveActor *actor = nullptr;
        float radius = 0.0F;
        CameraParamVec3 offset{};
#ifndef NDEBUG
        bool was_pointing = false;
        std::optional<std::uint64_t> last_select_frame_index{};
#endif
    };

    class StarPointerService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void register_target(const LiveActor &actor, float radius, const CameraParamVec3 &offset);
        void unregister_target(const LiveActor &actor);
        void set_target_radius(const LiveActor &actor, float radius);
        void start_mode(StarPointerMode mode);
        void set_guidance_active(bool active);
        void request_guidance(StarPointerGuidanceRequest request);

        [[nodiscard]] StarPointerMode mode() const;
        [[nodiscard]] bool has_target(const LiveActor &actor) const;
        [[nodiscard]] bool is_pointing(const LiveActor &actor, const WpadService &wpad, const std::optional<CameraPoseCompat> &camera_pose, bool check_z);
        [[nodiscard]] bool is_guidance_active() const;
        [[nodiscard]] bool is_guidance_requested(StarPointerGuidanceRequest request) const;
        [[nodiscard]] std::span<const StarPointerGuidanceRequest> guidance_requests() const;
        [[nodiscard]] std::span<const StarPointerModeEvent> mode_events() const;
#ifndef NDEBUG
        [[nodiscard]] std::span<const StarPointerTargetEvent> target_events() const;
#endif

    private:
#ifndef NDEBUG
        void record_target_pointing_sample(StarPointerTargetState &target, bool pointing, const WpadPointerState &pointer,
                                           bool has_projection, float target_x, float target_y, float projected_radius, bool check_z,
                                           bool select_triggered);
#endif

        std::uint64_t _frame_index = 0U;
        StarPointerMode _mode = StarPointerMode::None;
        bool _guidance_active = false;
        std::vector<StarPointerGuidanceRequest> _guidance_requests;
        std::map<const LiveActor *, StarPointerTargetState> _targets;
        std::vector<StarPointerModeEvent> _mode_events;
#ifndef NDEBUG
        std::vector<StarPointerTargetEvent> _target_events;
#endif
    };

    class CameraSystemService final {
    public:
        enum class ShakeRequestKind {
            Normal,
        };

        struct ShakeRequestEvent {
            ShakeRequestKind kind = ShakeRequestKind::Normal;
            std::uint64_t frame_index = 0U;
        };

        void begin_frame(std::uint64_t frame_index);
        void reset_camera_man();
        void request_normal_shake();
        void pause_on_camera_director();
        void pause_off_camera_director();
        void declare_event_camera_programmable(std::string_view name);
        void start_global_event_camera_no_target(std::string_view name);
        void end_global_event_camera(std::string_view name);
        [[nodiscard]] std::optional<CameraPoseCompat> set_programmable_camera_param(std::string_view name, const CameraParamVec3 &watch,
                                                                                    const CameraParamVec3 &eye, const CameraParamVec3 &up,
                                                                                    bool do_zero_w_offset);
        [[nodiscard]] std::optional<CameraPoseCompat> set_programmable_camera_fovy(std::string_view name, float fovy_degrees);

        [[nodiscard]] std::uint32_t reset_camera_man_count() const;
        [[nodiscard]] std::uint32_t normal_shake_request_count() const;
        [[nodiscard]] std::uint32_t camera_director_pause_count() const;
        [[nodiscard]] bool is_camera_director_paused() const;
        [[nodiscard]] std::optional<CameraPoseCompat> active_programmable_camera_pose() const;
        [[nodiscard]] std::optional<std::string_view> active_programmable_camera_name() const;
        [[nodiscard]] std::uint32_t programmable_camera_declare_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_start_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_end_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_param_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_fovy_count() const;
        [[nodiscard]] std::span<const ShakeRequestEvent> shake_request_events() const;

    private:
        struct ProgrammableCameraEventState {
            CameraPoseCompat pose{};
            bool declared = false;
            bool active = false;
            bool has_pose = false;
        };

        [[nodiscard]] ProgrammableCameraEventState *find_programmable_event(std::string_view name);
        [[nodiscard]] const ProgrammableCameraEventState *find_programmable_event(std::string_view name) const;
        [[nodiscard]] std::optional<CameraPoseCompat> active_programmable_camera_pose_for(std::string_view name) const;
        void push_shake_event(ShakeRequestKind kind);

        std::uint64_t _frame_index = 0U;
        std::uint32_t _reset_camera_man_count = 0U;
        std::uint32_t _normal_shake_request_count = 0U;
        std::uint32_t _camera_director_pause_count = 0U;
        std::map<std::string, ProgrammableCameraEventState> _programmable_camera_events;
        std::string _active_programmable_camera_name;
        std::uint32_t _programmable_camera_declare_count = 0U;
        std::uint32_t _programmable_camera_start_count = 0U;
        std::uint32_t _programmable_camera_end_count = 0U;
        std::uint32_t _programmable_camera_param_count = 0U;
        std::uint32_t _programmable_camera_fovy_count = 0U;
        std::vector<ShakeRequestEvent> _shake_request_events;
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
        Middle,
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
        void request_middle(s32 channel);
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
        void write_nand_file(std::string_view name, std::span<const std::uint8_t> bytes);
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_nand_file(std::string_view name) const;
        [[nodiscard]] bool exists(std::string_view name) const;
        bool erase(std::string_view name);
        [[nodiscard]] std::size_t file_count() const;
        void set_host_directory(std::filesystem::path directory);
        [[nodiscard]] const std::optional<std::filesystem::path> &host_directory() const;
        void load_host_files();
        void flush_host_files();
        [[nodiscard]] bool has_valid_game_data_container() const;
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
        [[nodiscard]] std::optional<std::map<std::string, std::vector<std::uint8_t>>> decode_game_data_container(std::span<const std::uint8_t> bytes) const;
        [[nodiscard]] std::vector<std::uint8_t> encode_game_data_container() const;
        void set_slot_state_internal(s32 slot_index, const SlotState &state, bool materialize_files);
        void materialize_slot_files(const SlotState &state);
        void load_slot_states_from_files();
        void load_sys_config_from_files();
        void write_sys_config_file();

        std::map<std::string, std::vector<std::uint8_t>> _files;
        std::vector<SlotState> _slot_states;
        std::optional<std::filesystem::path> _host_directory = {};
        OSTime _sys_config_time_announced = 0;
        OSTime _sys_config_time_sent = 0;
        u32 _sys_config_sent_bytes = 0U;
        bool _has_valid_game_data_container = false;
    };

    class MessageService final {
    public:
        void set_message(std::string_view tag, std::string_view text);
        void set_message(std::string_view tag, std::u16string_view text);
        std::size_t load_message_archive(const RarcArchive &archive);
        [[nodiscard]] std::size_t message_count() const;
        [[nodiscard]] const std::string *message(std::string_view tag) const;
        [[nodiscard]] const std::u16string *message_utf16(std::string_view tag) const;
        [[nodiscard]] const std::u16string *message_raw_utf16(std::string_view tag) const;
        [[nodiscard]] const std::vector<BmgControlTag> *message_control_tags(std::string_view tag) const;
        [[nodiscard]] std::string message_or(std::string_view tag, std::string_view fallback) const;
        [[nodiscard]] std::u16string message_utf16_or(std::string_view tag, std::u16string_view fallback) const;
        [[nodiscard]] std::u16string message_raw_utf16_or(std::string_view tag, std::u16string_view fallback) const;
        [[nodiscard]] std::u16string format_message_utf16(std::string_view tag, std::span<const BmgFormatArg> args) const;
        [[nodiscard]] std::u16string format_message_utf16_or(std::string_view tag, std::span<const BmgFormatArg> args,
                                                             std::u16string_view fallback) const;

    private:
        struct MessageText {
            std::u16string raw_utf16;
            std::u16string utf16;
            std::string utf8;
            std::vector<BmgControlTag> control_tags;
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
        std::array<GXLightState, 8U> _lights = {};
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
