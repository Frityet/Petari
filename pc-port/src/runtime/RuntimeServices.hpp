#pragma once

#include <array>
#include <cstddef>
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

#include <aurora/wpad.hpp>
#include <revolution.h>

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "camera/OriginalGameCamera.hpp"
#include "camera/EventCamera.hpp"
#include "camera/StageStartCamera.hpp"
#include "render/GXState.hpp"
#include "render/effects/EffectResource.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/NandFileSystemService.hpp"

class ActorLightCtrl;
class CameraTargetObj;
class LiveActor;
struct RumblePattern;

namespace smgpc::runtime {

    class PlayerSystemService;

    struct DvdFileReadTrace {
        std::string requested_path;
        std::string disc_path;
        std::string resolved_path;
        s32 entry_num = -1;
        std::size_t byte_count = 0U;
        std::size_t offset = 0U;
        s32 priority = 0;
    };

    struct DvdArchiveLoadTrace {
        std::string requested_path;
        std::string resolved_path;
        bool cache_hit = false;
        std::size_t load_count = 0U;
        std::size_t cached_archive_count = 0U;
        std::size_t resource_count = 0U;
    };

    struct DvdEntryMetadata {
        s32 entry_num = -1;
        std::string disc_path;
        std::string resolved_path;
        bool is_directory = false;
        std::size_t length = 0U;
    };

    struct DvdDirectoryEntry {
        s32 entry_num = -1;
        std::string disc_path;
        std::string name;
        bool is_directory = false;
    };

    struct DvdAsyncReadRequest {
        std::uint64_t id = 0U;
        std::string disc_path;
        s32 entry_num = -1;
        DVDFileInfo *file_info = nullptr;
        void *destination = nullptr;
        std::size_t length = 0U;
        std::size_t offset = 0U;
        s32 priority = 0;
        std::uint64_t submitted_frame = 0U;
        std::uint64_t completion_frame = 0U;
        bool completed = false;
        s32 result = DVD_STATE_BUSY;
        DVDCallback callback = nullptr;
    };

    class DvdFileSystemService final {
    public:
        explicit DvdFileSystemService(std::filesystem::path root);

        void begin_frame(std::uint64_t frame_index);
        [[nodiscard]] const std::filesystem::path &root() const;
        [[nodiscard]] std::string normalize_disc_path_string(std::string_view disc_path) const;
        [[nodiscard]] std::filesystem::path resolve(std::string_view disc_path) const;
        [[nodiscard]] bool exists(std::string_view disc_path) const;
        [[nodiscard]] s32 entry_num(std::string_view disc_path) const;
        [[nodiscard]] std::optional<DvdEntryMetadata> entry_metadata(std::string_view disc_path) const;
        [[nodiscard]] std::optional<DvdEntryMetadata> entry_metadata(s32 entry_num) const;
        [[nodiscard]] std::vector<DvdDirectoryEntry> directory_entries(std::string_view disc_path) const;
        [[nodiscard]] std::optional<std::filesystem::path> find_first(std::initializer_list<std::filesystem::path> candidates) const;
        [[nodiscard]] std::optional<std::filesystem::path> find_layout_archive(std::string_view layout_name) const;
        [[nodiscard]] std::optional<std::filesystem::path> find_object_archive(std::string_view object_name) const;
        [[nodiscard]] std::vector<std::uint8_t> read_file(std::string_view disc_path) const;
        [[nodiscard]] std::vector<std::uint8_t> read_file_range(std::string_view disc_path, std::size_t offset, std::size_t length,
                                                                s32 priority) const;
        [[nodiscard]] std::uint64_t submit_async_read(std::string_view disc_path, DVDFileInfo *file_info, void *destination,
                                                      std::size_t length, std::size_t offset, s32 priority, DVDCallback callback,
                                                      std::uint64_t delay_frames = 1U);
        [[nodiscard]] smgpc::resource::RarcArchive &archive(std::string_view disc_path);
        [[nodiscard]] smgpc::resource::RarcArchive &archive_for_path(const std::filesystem::path &path);
        [[nodiscard]] std::size_t archive_load_count(std::string_view disc_path) const;
        [[nodiscard]] std::size_t archive_load_count_for_path(const std::filesystem::path &path) const;
        [[nodiscard]] std::size_t cached_archive_count() const;
        [[nodiscard]] std::span<const DvdFileReadTrace> file_read_trace() const;
        [[nodiscard]] std::span<const DvdAsyncReadRequest> async_read_trace() const;
        [[nodiscard]] std::span<const DvdArchiveLoadTrace> archive_load_trace() const;
        void clear_trace();

    private:
        [[nodiscard]] std::filesystem::path normalize_disc_path(std::string_view disc_path) const;
        void complete_ready_async_reads();
        void ensure_entry_table() const;
        [[nodiscard]] static std::string entry_key(const std::filesystem::path &disc_path);
        [[nodiscard]] std::string archive_cache_key_for_path(const std::filesystem::path &path) const;
        [[nodiscard]] std::string archive_cache_key(std::string_view disc_path) const;
        [[nodiscard]] smgpc::resource::RarcArchive &archive_for_path_with_request(const std::filesystem::path &path, std::string_view requested_path);

        std::filesystem::path _root;
        std::map<std::string, std::unique_ptr<smgpc::resource::RarcArchive>> _archives;
        std::map<std::string, std::size_t> _archive_load_counts;
        std::uint64_t _frame_index = 0U;
        std::uint64_t _next_async_read_id = 1U;
        std::vector<DvdAsyncReadRequest> _async_read_trace;
        mutable bool _entry_table_initialized = false;
        mutable std::vector<DvdEntryMetadata> _entry_table;
        mutable std::map<std::string, s32, std::less<>> _entry_num_by_disc_path;
        mutable std::vector<DvdFileReadTrace> _file_read_trace;
        std::vector<DvdArchiveLoadTrace> _archive_load_trace;
    };

    using WpadPointerState = aurora::WpadPointerState;
    using WpadVec3State = aurora::WpadVec3State;
    using WpadStickState = aurora::WpadStickState;
    using WpadChannelState = aurora::WpadChannelState;
    using WpadService = aurora::WpadService;

    enum class AudioEventKind {
        StageBgmStart,
        StageBgmUnlock,
        StageBgmStop,
        SubBgmStart,
        SubBgmStop,
        SystemSoundStart,
        SystemSoundStop,
        SystemLevelSoundStart,
        ActorSoundStart,
        ActorLevelSoundStart,
        LimitedSoundRegister,
        LevelSoundSubmit,
        LevelSoundPermit,
        AtmosphereSoundStart,
    };

    struct AudioEvent {
        // Actor events capture logical request identity only. They do not
        // claim a positional mixer voice or retain ownership of the source.
        AudioEventKind kind = AudioEventKind::StageBgmStart;
        std::string name;
        std::optional<u32> sound_id;
        const void *source_identity = nullptr;
        std::string source_name;
        s32 parameter_1 = -1;
        s32 parameter_2 = -1;
        s32 parameter_3 = -1;
        bool prepared = false;
        s32 fade_frames = 0;
        u32 delay_frames = 0U;
        std::uint64_t frame_index = 0U;
    };

    class AudioEventService final {
    public:
        static constexpr std::size_t cEventRetentionLimit = 8192U;

        void begin_frame(std::uint64_t frame_index);
        void reset_stage_state();
        void resolve_stage_bgm_absent();
        void start_stage_bgm(u32 sound_id);
        void start_stage_bgm(std::string_view name, u32 sound_id);
        void unlock_stage_bgm();
        void stop_stage_bgm(s32 fade_frames);
        void clear_last_stage_bgm_id();
        void set_cube_bgm_change_invalid(bool invalid);
        void start_system_sound(std::string_view name);
        void stop_system_sound(std::string_view name, u32 delay_frames);
        void start_system_level_sound(std::string_view name);
        void start_actor_sound(const void *actor_identity, std::string_view actor_name,
                               std::string_view name, s32 parameter_1, s32 parameter_2);
        void start_actor_level_sound(const void *actor_identity, std::string_view actor_name,
                                     std::string_view name, s32 parameter_1, s32 parameter_2,
                                     s32 parameter_3);
        void register_limited_sound(std::string_view name, s32 limit);
        void start_sub_bgm(std::string_view name, bool prepared);
        void stop_sub_bgm(u32 fade_frames);
        void submit_level_sound();
        void permit_level_sound();
        void start_atmosphere_sound(std::string_view name);

        [[nodiscard]] bool has_active_stage_bgm() const;
        [[nodiscard]] bool is_stage_bgm_identity_resolved() const;
        [[nodiscard]] std::string_view current_stage_bgm_name() const;
        [[nodiscard]] std::optional<u32> current_stage_bgm_id() const;
        [[nodiscard]] std::optional<u32> last_stage_bgm_id() const;
        [[nodiscard]] bool has_active_sub_bgm() const;
        [[nodiscard]] bool is_sub_bgm_stopping() const;
        [[nodiscard]] bool is_sub_bgm_prepared() const;
        [[nodiscard]] std::string_view current_sub_bgm_name() const;
        [[nodiscard]] u32 sub_bgm_fade_frames_remaining() const;
        [[nodiscard]] bool is_cube_bgm_change_invalid() const;
        [[nodiscard]] std::span<const AudioEvent> events() const;
        [[nodiscard]] std::uint64_t dropped_event_count() const;

    private:
        static constexpr std::size_t cEventRetentionTrimCount =
            cEventRetentionLimit / 2U;

        void push_event(AudioEvent event);

        std::uint64_t _frame_index = 0U;
        std::string _stage_bgm_name;
        std::optional<u32> _stage_bgm_id;
        std::optional<u32> _last_stage_bgm_id;
        bool _stage_bgm_requested = false;
        bool _stage_bgm_identity_resolved = false;
        bool _cube_bgm_change_invalid = false;
        std::string _sub_bgm_name;
        std::uint64_t _sub_bgm_stop_frame = 0U;
        bool _sub_bgm_active = false;
        bool _sub_bgm_stopping = false;
        bool _sub_bgm_prepared = false;
        std::vector<AudioEvent> _events;
        std::uint64_t _dropped_event_count = 0U;
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
        const void *host_identity = nullptr;
        std::string host_name;
        std::string resource_group_name;
        s32 requested_capacity = 0;
        bool sort_enabled = false;
        std::uint64_t frame_index = 0U;
    };

    struct EffectEvent {
        EffectEventKind kind = EffectEventKind::Emit;
        const void *host_identity = nullptr;
        std::string actor_name;
        std::string effect_name;
        std::uint64_t frame_index = 0U;
        std::optional<EffectKeeperRegistration> keeper;
        std::vector<smgpc::render::effects::ResolvedEffectResource> resolved_resources;
    };

    enum class EffectHostBindingSource {
        LiveActorBaseMatrix,
        LayoutActorTransform,
        SimpleLayoutOrigin,
    };

    struct EffectHostBinding {
        EffectKeeperHostKind host_kind = EffectKeeperHostKind::LiveActor;
        EffectHostBindingSource source = EffectHostBindingSource::LiveActorBaseMatrix;
        const void *host_identity = nullptr;
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
        float velocity_x = 0.0F;
        float velocity_y = 0.0F;
        float velocity_z = 0.0F;
        float momentum = 1.0F;
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
        const void *host_identity = nullptr;
        std::string actor_name;
        std::string effect_name;
        std::uint64_t start_frame_index = 0U;
        std::optional<EffectKeeperRegistration> keeper;
        std::optional<EffectHostBinding> host_binding;
        std::vector<smgpc::render::effects::ResolvedEffectResource> resolved_resources;
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
        std::string packet_mode = "JpcBillboard2D";
        std::uint8_t shape_type = 2U;
        bool world_space = false;
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
        void load_resources(const smgpc::resource::RarcArchive &archive);
        void begin_frame(std::uint64_t frame_index);
        void register_keeper(EffectKeeperHostKind host_kind, std::string_view host_name, s32 requested_capacity,
                             std::string_view resource_group_name, bool sort_enabled, const void *host_identity = nullptr);
        void unregister_keeper(std::string_view host_name, const void *host_identity = nullptr);
        void bind_host_transform(EffectKeeperHostKind host_kind, std::string_view host_name, EffectHostBindingSource source,
                                 const std::array<float, 12U> &matrix, bool host_dead, const void *host_identity = nullptr);
        void unbind_host_transform(std::string_view host_name, const void *host_identity = nullptr);
        void emit(std::string_view actor_name, std::string_view effect_name, const void *host_identity = nullptr);
        void delete_effect(std::string_view actor_name, std::string_view effect_name, const void *host_identity = nullptr);
        void delete_all(std::string_view actor_name, const void *host_identity = nullptr);
        void release_host_state(std::string_view host_name, const void *host_identity = nullptr) noexcept;
        void draw(s32 draw_type, const smgpc::camera::CameraPose *camera_pose = nullptr);

        [[nodiscard]] std::span<const EffectEvent> events() const;
        [[nodiscard]] std::span<const ActiveEffectInstance> active_effect_instances() const;
        [[nodiscard]] std::vector<EffectKeeperRegistration> registered_keepers() const;
        [[nodiscard]] std::optional<EffectKeeperRegistration> registered_keeper(std::string_view host_name,
                                                                                const void *host_identity = nullptr) const;
        [[nodiscard]] std::optional<EffectHostBinding> host_binding(std::string_view host_name,
                                                                    const void *host_identity = nullptr) const;
        [[nodiscard]] std::vector<std::string> active_effects(std::string_view actor_name,
                                                              const void *host_identity = nullptr) const;
        [[nodiscard]] const smgpc::render::effects::EffectResourceLibrary *resource_library() const;
#ifndef NDEBUG
        [[nodiscard]] std::span<const EffectDrawPacketTrace> draw_packets() const;
#endif

    private:
        [[nodiscard]] std::vector<smgpc::render::effects::ResolvedEffectResource> resolve(std::string_view actor_name,
                                                                                          std::string_view effect_name,
                                                                                          const void *host_identity = nullptr) const;
        [[nodiscard]] render::TextureHandle texture_handle_for(const smgpc::render::effects::JpcTextureMetadata &texture);
        [[nodiscard]] std::vector<JpcEffectEmitterInstance> create_emitters(std::span<const smgpc::render::effects::ResolvedEffectResource> resources);
        void advance_effects_to_frame(std::uint64_t frame_index);
        void advance_emitter_to_frame(JpcEffectEmitterInstance &emitter, const smgpc::render::effects::ResolvedEffectResource &resource, std::uint64_t frame_index);

        std::uint64_t _frame_index = 0U;
        std::vector<EffectEvent> _events;
        std::vector<ActiveEffectInstance> _active_effects;
        std::map<std::string, EffectKeeperRegistration, std::less<>> _registered_keepers;
        std::map<const void *, EffectKeeperRegistration, std::less<>> _registered_keeper_instances;
        std::map<std::string, EffectHostBinding, std::less<>> _host_bindings;
        std::map<const void *, EffectHostBinding, std::less<>> _host_binding_instances;
        std::optional<smgpc::render::effects::EffectResourceLibrary> _resource_library;
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
        SphereSelectorReaction,
        SphereSelectorFinger,
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
        smgpc::camera::CameraParamVec3 offset{};
#ifndef NDEBUG
        bool was_pointing = false;
        std::optional<std::uint64_t> last_select_frame_index{};
#endif
    };

    class StarPointerService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void register_target(const LiveActor &actor, float radius, const smgpc::camera::CameraParamVec3 &offset);
        void unregister_target(const LiveActor &actor);
        void set_target_radius(const LiveActor &actor, float radius);
        void start_mode(StarPointerMode mode);
        void push_mode(const void *requester, StarPointerMode mode);
        void pop_mode(const void *requester);
        void clear_mode_requests(const void *requester);
        void set_guidance_active(bool active);
        void request_guidance(StarPointerGuidanceRequest request);

        [[nodiscard]] StarPointerMode mode() const;
        [[nodiscard]] bool has_target(const LiveActor &actor) const;
        [[nodiscard]] bool is_pointing(const LiveActor &actor, const WpadService &wpad, const std::optional<smgpc::camera::CameraPose> &camera_pose, bool check_z);
        [[nodiscard]] bool is_guidance_active() const;
        [[nodiscard]] bool is_guidance_requested(StarPointerGuidanceRequest request) const;
        [[nodiscard]] std::span<const StarPointerGuidanceRequest> guidance_requests() const;
        [[nodiscard]] std::span<const StarPointerModeEvent> mode_events() const;
        [[nodiscard]] std::size_t mode_request_count(const void *requester) const;
#ifndef NDEBUG
        [[nodiscard]] std::span<const StarPointerTargetEvent> target_events() const;
#endif

    private:
        struct ModeRequest {
            const void *requester = nullptr;
            StarPointerMode mode = StarPointerMode::None;
        };

        void update_mode_from_requests();
#ifndef NDEBUG
        void record_target_pointing_sample(StarPointerTargetState &target, bool pointing, const WpadPointerState &pointer,
                                           bool has_projection, float target_x, float target_y, float projected_radius, bool check_z,
                                           bool select_triggered);
#endif

        std::uint64_t _frame_index = 0U;
        StarPointerMode _base_mode = StarPointerMode::None;
        StarPointerMode _mode = StarPointerMode::None;
        bool _guidance_active = false;
        std::vector<StarPointerGuidanceRequest> _guidance_requests;
        std::vector<ModeRequest> _mode_requests;
        std::map<const LiveActor *, StarPointerTargetState> _targets;
        std::vector<StarPointerModeEvent> _mode_events;
#ifndef NDEBUG
        std::vector<StarPointerTargetEvent> _target_events;
#endif
    };

    class CameraSystemService final {
    public:
        enum class ShakeRequestKind {
            VeryWeak,
            Weak,
            NormalWeak,
            Normal,
            NormalStrong,
            Strong,
            VeryStrong,
        };

        struct ShakeRequestEvent {
            ShakeRequestKind kind = ShakeRequestKind::Normal;
            std::uint64_t frame_index = 0U;
        };

        void begin_frame(std::uint64_t frame_index);
        void set_shake_projection_dimensions(float screen_width, float efb_height);
        void clear_shake_projection_dimensions() noexcept;
        void reset_camera_man();
        void request_very_weak_shake();
        void request_weak_shake();
        void request_normal_weak_shake();
        void request_normal_shake();
        void request_normal_strong_shake();
        void request_strong_shake();
        void request_very_strong_shake();
        void pause_on_camera_director();
        void pause_off_camera_director();
        void attach_event_camera_catalog(
            const smgpc::camera::EventCameraCatalog &catalog);
        void detach_event_camera_catalog(
            const smgpc::camera::EventCameraCatalog &catalog) noexcept;
        void declare_event_camera(std::int32_t zone_id, std::string_view name);
        void declare_event_camera_animation(
            std::int32_t zone_id, std::string_view name,
            smgpc::camera::CameraAnimation animation);
        void start_event_camera(
            std::int32_t zone_id, std::string_view name,
            smgpc::camera::EventCameraTarget target,
            std::int32_t interpolation_frames, float speed = 1.0F);
        void end_event_camera(std::int32_t zone_id, std::string_view name,
                              bool force, std::int32_t interpolation_frames);
        [[nodiscard]] ActorCameraInfo *create_actor_camera_info(
            std::int32_t camera_set_id, std::int32_t zone_id);
        void declare_event_camera_programmable(std::string_view name);
        void start_global_event_camera_no_target(std::string_view name);
        void end_global_event_camera(std::string_view name);
        [[nodiscard]] std::uint64_t set_stage_start_camera(
            smgpc::camera::ResolvedStageStartCamera camera);
        // Retain an original authored camera controller without
        // entering the retail start-position camera mode.
        [[nodiscard]] std::uint64_t set_authored_game_camera(
            smgpc::camera::ResolvedStageStartCamera camera);
        void set_game_camera_target(
            std::uint64_t owner_generation,
            std::optional<smgpc::camera::StageCameraTargetState> target);
        void set_game_camera_target_player(std::uint64_t owner_generation,
                                           PlayerSystemService &player);
        void clear_stage_start_camera(
            std::uint64_t owner_generation) noexcept;
        void start_start_position_camera(bool immediate);
        void end_start_position_camera();
        void set_game_camera_pose(const smgpc::camera::CameraPose &pose);
        void clear_game_camera_pose();
        [[nodiscard]] std::optional<smgpc::camera::CameraPose> set_programmable_camera_param(std::string_view name, const smgpc::camera::CameraParamVec3 &watch,
                                                                                             const smgpc::camera::CameraParamVec3 &eye, const smgpc::camera::CameraParamVec3 &up,
                                                                                             bool do_zero_w_offset);
        [[nodiscard]] std::optional<smgpc::camera::CameraPose> set_programmable_camera_fovy(std::string_view name, float fovy_degrees);

        [[nodiscard]] std::uint32_t reset_camera_man_count() const;
        [[nodiscard]] std::uint32_t camera_director_pause_count() const;
        [[nodiscard]] bool is_camera_director_paused() const;
        [[nodiscard]] const smgpc::camera::ResolvedStageStartCamera *
        stage_start_camera() const noexcept;
        [[nodiscard]] bool is_start_position_camera_end() const;
        [[nodiscard]] std::uint32_t
        start_position_camera_zero_interpolation_frames() const;
        [[nodiscard]] std::optional<smgpc::camera::CameraPose> game_camera_pose() const;
        [[nodiscard]] std::optional<smgpc::camera::CameraPose> active_event_camera_pose() const;
        [[nodiscard]] std::optional<smgpc::camera::EventCameraKey> active_event_camera_key() const;
        [[nodiscard]] bool is_event_camera_active(std::int32_t zone_id,
                                                  std::string_view name) const;
        [[nodiscard]] bool is_event_camera_declared(
            std::int32_t zone_id, std::string_view name) const;
        [[nodiscard]] bool is_event_camera_animation_end(
            std::int32_t zone_id, std::string_view name) const;
        [[nodiscard]] std::int32_t event_camera_animation_frame(
            std::int32_t zone_id, std::string_view name) const;
        [[nodiscard]] std::int32_t event_camera_frames(
            std::int32_t zone_id, std::string_view name) const;
        [[nodiscard]] std::size_t actor_camera_info_count() const noexcept;
        [[nodiscard]] std::optional<smgpc::camera::CameraPose> active_programmable_camera_pose() const;
        [[nodiscard]] std::optional<smgpc::camera::CameraPose> effective_camera_pose() const;
        [[nodiscard]] smgpc::camera::CameraPose apply_shake(const smgpc::camera::CameraPose &pose) const;
        [[nodiscard]] std::optional<std::string_view> active_programmable_camera_name() const;
        [[nodiscard]] std::uint32_t programmable_camera_declare_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_start_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_end_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_param_count() const;
        [[nodiscard]] std::uint32_t programmable_camera_fovy_count() const;
        [[nodiscard]] std::span<const ShakeRequestEvent> shake_request_events() const;

    private:
        struct ProgrammableCameraEventState {
            smgpc::camera::CameraPose pose{};
            bool declared = false;
            bool active = false;
            bool has_pose = false;
        };

        struct AuthoredGameCameraState {
            std::unique_ptr<smgpc::camera::OriginalGameCamera> controller;
            std::optional<smgpc::camera::StageCameraTargetState> target;
            PlayerSystemService *player_target = nullptr;
            bool overridden = false;
            bool reset_requested = false;
            bool manager_reset_requested = false;
        };

        [[nodiscard]] std::uint64_t set_owned_stage_camera(
            smgpc::camera::ResolvedStageStartCamera camera,
            bool start_position_active);
        void update_authored_game_camera();
        void update_camera_view();
        void clear_camera_view();
        void update_game_camera_activation();
        [[nodiscard]] ProgrammableCameraEventState *find_programmable_event(std::string_view name);
        [[nodiscard]] const ProgrammableCameraEventState *find_programmable_event(std::string_view name) const;
        [[nodiscard]] std::optional<smgpc::camera::CameraPose> active_programmable_camera_pose_for(std::string_view name) const;
        void request_shake(ShakeRequestKind kind);
        void push_shake_event(ShakeRequestKind kind);

        std::uint64_t _frame_index = 0U;
        std::optional<std::uint64_t> _last_camera_movement_frame;
        std::uint32_t _reset_camera_man_count = 0U;
        std::array<std::optional<std::uint32_t>, 7U> _vertical_shake_steps;
        float _shake_offset_x = 0.0F;
        float _shake_offset_y = 0.0F;
        std::optional<float> _shake_screen_width;
        std::optional<float> _shake_efb_height;
        std::uint32_t _camera_director_pause_count = 0U;
        std::optional<smgpc::camera::ResolvedStageStartCamera>
            _stage_start_camera;
        std::optional<AuthoredGameCameraState> _authored_game_camera;
        std::uint64_t _stage_start_camera_owner_generation = 0U;
        std::uint64_t _next_stage_start_camera_owner_generation = 1U;
        bool _start_position_camera_active = false;
        std::uint32_t _start_position_camera_zero_interpolation_frames = 0U;
        std::optional<smgpc::camera::CameraPose> _game_camera_pose;
        std::unique_ptr<smgpc::camera::OriginalCameraView> _camera_view;
        std::optional<smgpc::camera::CameraPose> _view_camera_pose;
        std::optional<std::uint64_t> _last_view_frame;
        smgpc::camera::EventCameraRuntime _event_cameras;
        std::map<std::string, ProgrammableCameraEventState> _programmable_camera_events;
        std::string _active_programmable_camera_name;
        std::uint32_t _programmable_camera_declare_count = 0U;
        std::uint32_t _programmable_camera_start_count = 0U;
        std::uint32_t _programmable_camera_end_count = 0U;
        std::uint32_t _programmable_camera_param_count = 0U;
        std::uint32_t _programmable_camera_fovy_count = 0U;
        std::vector<ShakeRequestEvent> _shake_request_events;
    };

    struct PlayerActorBridge {
        using SwingPermissionWriter = void (*)(LiveActor &, bool);
        using ElementModeReader = s32 (*)(const LiveActor &);
        using BaseMatrixReader = MtxPtr (*)(const LiveActor &);
        using VectorReader = void (*)(const LiveActor &, TVec3f *);

        SwingPermissionWriter set_swing_permission = nullptr;
        // Concrete player owners install this capability only when their
        // attached object really exposes the retail MarioActor mode field.
        ElementModeReader read_element_mode = nullptr;
        BaseMatrixReader read_base_matrix = nullptr;
        VectorReader read_up_vector = nullptr;
        VectorReader read_front_vector = nullptr;
        VectorReader read_side_vector = nullptr;
    };

    class PlayerSystemService final {
    public:
        PlayerSystemService();
        ~PlayerSystemService();

        void reset_stage_state();
        void clear_stage_state();
        void attach_actor(LiveActor &actor,
                          PlayerActorBridge actor_bridge = {});
        void detach_actor(const LiveActor *actor = nullptr);
        void synchronize_attached_actor();
        void set_camera_target(std::unique_ptr<CameraTargetObj> target);
        void advance_camera_target(std::uint64_t frame_index);

        void show_player();
        void hide_player();
        void set_base_matrix(MtxPtr matrix);
        void set_swing_permission(bool permitted);
        void set_player_dead_state(bool dead);
        void clear_player_dead_state();
        void disable_control();
        void enable_control(bool reset_condition);
        void finish_opening_demo();

        [[nodiscard]] bool is_player_hidden() const;
        [[nodiscard]] bool has_base_matrix() const;
        [[nodiscard]] bool has_forced_base_matrix() const;
        [[nodiscard]] std::span<const f32, 12U> base_matrix() const;
        [[nodiscard]] std::span<const f32, 3U> position() const;
        [[nodiscard]] std::span<const f32, 3U> velocity() const;
        [[nodiscard]] std::span<const f32, 3U> gravity() const;
        [[nodiscard]] bool is_on_ground() const;
        [[nodiscard]] std::optional<bool> player_dead_state() const;
        [[nodiscard]] std::optional<s32> player_element_mode() const;
        [[nodiscard]] std::optional<smgpc::camera::StageCameraTargetState> camera_target_state() const;
        [[nodiscard]] CameraTargetObj *camera_target() const;
        [[nodiscard]] MtxPtr actor_base_matrix() const;
        [[nodiscard]] bool copy_actor_up_vector(TVec3f *out) const;
        [[nodiscard]] bool copy_actor_front_vector(TVec3f *out) const;
        [[nodiscard]] bool copy_actor_side_vector(TVec3f *out) const;
        [[nodiscard]] bool is_swing_permitted() const;
        [[nodiscard]] bool is_control_enabled() const;
        [[nodiscard]] std::uint64_t base_matrix_revision() const;
        [[nodiscard]] LiveActor *attached_actor() const;
        bool consume_reset_condition_request();

    private:
        void copy_actor_state();

        LiveActor *_attached_actor = nullptr;
        bool _player_hidden = false;
        bool _has_base_matrix = false;
        bool _has_forced_base_matrix = false;
        bool _on_ground = false;
        std::optional<bool> _player_dead_state;
        bool _swing_permitted = false;
        bool _control_enabled = true;
        bool _reset_condition_requested = false;
        std::uint64_t _base_matrix_revision = 0U;
        std::array<f32, 12U> _base_matrix{};
        std::array<f32, 3U> _position{};
        std::array<f32, 3U> _velocity{};
        std::array<f32, 3U> _gravity{0.0F, -1.0F, 0.0F};
        PlayerActorBridge _actor_bridge{};
        std::unique_ptr<CameraTargetObj> _camera_target;
        std::optional<std::uint64_t> _camera_target_frame;
    };

    class GameLayoutService final {
    public:
        void activate_default_game_layout();
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
        Named,
    };

    struct RumbleRequestEvent {
        RumbleRequestKind kind = RumbleRequestKind::Named;
        std::string pattern_name;
        s32 channel = 0;
        std::uint64_t frame_index = 0U;
    };

    class RumbleActuator {
    public:
        virtual ~RumbleActuator() = default;

        [[nodiscard]] virtual bool is_available(s32 channel) const noexcept = 0;
        virtual void set_motor(s32 channel, bool enabled) noexcept = 0;
    };

    class RumbleService final {
    public:
        explicit RumbleService(RumbleActuator *actuator = nullptr);
        ~RumbleService();

        RumbleService(const RumbleService &) = delete;
        RumbleService &operator=(const RumbleService &) = delete;

        void attach_actuator(RumbleActuator &actuator);
        void begin_frame(std::uint64_t frame_index);
        [[nodiscard]] bool try_request_pattern(const void *source, std::string_view pattern_name, s32 channel);
        void stop_all() noexcept;

        [[nodiscard]] std::span<const RumbleRequestEvent> events() const;

    private:
        struct ActivePattern {
            const void *source = nullptr;
            const RumblePattern *pattern = nullptr;
            std::size_t next_frame = 0U;
        };

        void set_motor(s32 channel, bool enabled) noexcept;

        RumbleActuator *_actuator = nullptr;
        std::uint64_t _frame_index = 0U;
        std::array<std::vector<ActivePattern>, WPAD_MAX_CONTROLLERS> _active_patterns;
        std::array<bool, WPAD_MAX_CONTROLLERS> _motor_enabled = {};
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
        bool consume_change_stage_in_game_after_loading_game_data_request();

        [[nodiscard]] bool is_change_stage_in_game_after_loading_game_data_requested() const;
        [[nodiscard]] std::span<const SequenceRequestEvent> events() const;

    private:
        std::uint64_t _frame_index = 0U;
        bool _change_stage_in_game_after_loading_game_data_requested = false;
        std::vector<SequenceRequestEvent> _events;
    };

    class SaveDataService final {
    public:
        SaveDataService();
        void write_file(std::string_view name, std::span<const std::uint8_t> bytes);
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_file(std::string_view name) const;
        void write_nand_file(std::string_view name, std::span<const std::uint8_t> bytes);
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_nand_file(std::string_view name) const;
        [[nodiscard]] NandFileSystemService &nand();
        [[nodiscard]] const NandFileSystemService &nand() const;
        [[nodiscard]] bool exists(std::string_view name) const;
        bool erase(std::string_view name);
        [[nodiscard]] std::size_t file_count() const;
        void set_host_directory(std::filesystem::path directory);
        [[nodiscard]] const std::optional<std::filesystem::path> &host_directory() const;
        void load_host_files();
        void flush_host_files();
        [[nodiscard]] bool has_valid_game_data_container() const;

    private:
        [[nodiscard]] std::filesystem::path host_file_path(std::string_view name) const;
        void write_host_file(std::string_view name, std::span<const std::uint8_t> bytes) const;
        void erase_host_file(std::string_view name) const;
        [[nodiscard]] std::optional<std::map<std::string, std::vector<std::uint8_t>>> decode_game_data_container(std::span<const std::uint8_t> bytes) const;

        std::map<std::string, std::vector<std::uint8_t>> _files;
        std::optional<std::filesystem::path> _host_directory = {};
        NandFileSystemService _nand;
        bool _has_valid_game_data_container = false;
    };

    class MessageService final {
    public:
        void set_message(std::string_view tag, std::string_view text);
        void set_message(std::string_view tag, std::u16string_view text);
        std::size_t load_message_archive(const smgpc::resource::RarcArchive &archive);
        [[nodiscard]] std::size_t message_count() const;
        [[nodiscard]] const std::string *message(std::string_view tag) const;
        [[nodiscard]] const std::u16string *message_utf16(std::string_view tag) const;
        [[nodiscard]] const std::u16string *message_raw_utf16(std::string_view tag) const;
        [[nodiscard]] const smgpc::resource::BmgMessageInfo *message_info(std::string_view tag) const;
        [[nodiscard]] const std::vector<smgpc::resource::BmgControlTag> *message_control_tags(std::string_view tag) const;
        [[nodiscard]] std::u16string format_message_utf16(std::string_view tag, std::span<const smgpc::resource::BmgFormatArg> args) const;
        [[nodiscard]] std::optional<std::uint32_t> message_index(std::string_view tag) const;
        [[nodiscard]] const std::string *message_id(std::uint32_t index) const;
        [[nodiscard]] const smgpc::resource::BmgFlowData *flow_data() const;
        [[nodiscard]] const smgpc::resource::BmgFlowNode *flow_node(std::uint32_t index) const;
        [[nodiscard]] std::optional<std::uint32_t> first_flow_node_for_message(std::uint32_t message_index) const;
        [[nodiscard]] std::optional<std::uint16_t> branch_flow_node(std::uint32_t branch_index) const;

    private:
        struct MessageText {
            std::u16string raw_utf16;
            std::u16string utf16;
            std::string utf8;
            smgpc::resource::BmgMessageInfo info{};
            std::vector<smgpc::resource::BmgControlTag> control_tags;
        };

        std::map<std::string, MessageText> _messages;
        std::map<std::string, std::uint32_t, std::less<>> _message_indices;
        std::vector<std::string> _message_ids_by_index;
        std::optional<smgpc::resource::BmgFlowData> _flow_data;
    };

    class SceneLightService final {
    public:
        void clear();
        void clear_light(std::size_t index);
        void set_light(std::size_t index, const smgpc::render::GXLightState &light);
        void clear_actor_ambient();
        void set_actor_ambient(smgpc::render::GXColorValue color);
        void register_player_light_ctrl(const ActorLightCtrl *light_ctrl);
        void unregister_player_light_ctrl(const ActorLightCtrl *light_ctrl);

        [[nodiscard]] const smgpc::render::GXLightState *light(std::size_t index) const;
        [[nodiscard]] std::span<const smgpc::render::GXLightState> lights() const;
        [[nodiscard]] const std::optional<smgpc::render::GXColorValue> &actor_ambient() const;
        [[nodiscard]] const ActorLightCtrl *player_light_ctrl() const;
        [[nodiscard]] std::uint8_t loaded_mask() const;

    private:
        std::array<smgpc::render::GXLightState, 8U> _lights = {};
        std::optional<smgpc::render::GXColorValue> _actor_ambient = {};
        const ActorLightCtrl *_player_light_ctrl = nullptr;
    };

}  // namespace smgpc::runtime
