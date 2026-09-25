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
#include <JSystem/JGeometry/TVec.hpp>

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"
#include "render/GXState.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/NandFileSystemService.hpp"

class ActorLightCtrl;
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
        [[nodiscard]] std::shared_ptr<const smgpc::resource::RarcArchive> retain_archive_for_path(const std::filesystem::path &path);
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
        std::map<std::string, std::shared_ptr<smgpc::resource::RarcArchive>> _archives;
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
#ifndef NDEBUG
        bool was_pointing = false;
        std::optional<std::uint64_t> last_select_frame_index{};
#endif
    };

    class StarPointerService final {
    public:
        void begin_frame(std::uint64_t frame_index);
        void unregister_target(const LiveActor &actor);
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

    struct PlayerActorBridge {
        using ElementModeReader = s32 (*)(const LiveActor &);
        using BaseMatrixReader = MtxPtr (*)(const LiveActor &);
        using VectorReader = void (*)(const LiveActor &, TVec3f *);
        using NerveChangeReader = bool (*)(const LiveActor &);
        using CenterPositionReader = TVec3f *(*)(LiveActor &);

        // Concrete player owners install this capability only when their
        // attached object really exposes the retail MarioActor mode field.
        ElementModeReader read_element_mode = nullptr;
        BaseMatrixReader read_base_matrix = nullptr;
        VectorReader read_up_vector = nullptr;
        VectorReader read_front_vector = nullptr;
        VectorReader read_side_vector = nullptr;
        // Original MR::isPlayerDead is the inverse of this live actor query.
        NerveChangeReader read_nerve_change_enabled = nullptr;
        CenterPositionReader read_center_position = nullptr;
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

        void set_base_matrix(MtxPtr matrix);

        [[nodiscard]] bool has_base_matrix() const;
        [[nodiscard]] bool has_forced_base_matrix() const;
        [[nodiscard]] std::span<const f32, 12U> base_matrix() const;
        [[nodiscard]] std::span<const f32, 3U> position() const;
        [[nodiscard]] std::span<const f32, 3U> velocity() const;
        [[nodiscard]] std::span<const f32, 3U> gravity() const;
        [[nodiscard]] bool is_on_ground() const;
        [[nodiscard]] std::optional<bool> player_dead_state() const;
        [[nodiscard]] std::optional<s32> player_element_mode() const;
        [[nodiscard]] MtxPtr actor_base_matrix() const;
        [[nodiscard]] TVec3f *actor_center_position() const;
        [[nodiscard]] bool copy_actor_up_vector(TVec3f *out) const;
        [[nodiscard]] bool copy_actor_front_vector(TVec3f *out) const;
        [[nodiscard]] bool copy_actor_side_vector(TVec3f *out) const;
        [[nodiscard]] std::uint64_t base_matrix_revision() const;
        [[nodiscard]] LiveActor *attached_actor() const;

    private:
        void copy_actor_state();

        LiveActor *_attached_actor = nullptr;
        bool _has_base_matrix = false;
        bool _has_forced_base_matrix = false;
        bool _on_ground = false;
        std::uint64_t _base_matrix_revision = 0U;
        std::array<f32, 12U> _base_matrix{};
        std::array<f32, 3U> _position{};
        std::array<f32, 3U> _velocity{};
        std::array<f32, 3U> _gravity{0.0F, -1.0F, 0.0F};
        PlayerActorBridge _actor_bridge{};
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
        ~SaveDataService();
        void activate_nand();
        void write_file(std::string_view name, std::span<const std::uint8_t> bytes);
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_file(std::string_view name) const;
        void write_nand_file(std::string_view name, std::span<const std::uint8_t> bytes, u8 permission = 0x3c, u8 attribute = 0);
        [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_nand_file(std::string_view name) const;
        s32 create_nand_file(std::string_view name, u8 permission, u8 attribute);
        s32 move_nand_file(std::string_view source, std::string_view destination);
        bool erase_nand_file(std::string_view name);
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
        [[nodiscard]] std::string nand_file_key(std::string_view name) const;
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
        [[nodiscard]] const std::wstring *message_raw_wide(std::string_view tag) const;
        [[nodiscard]] const char *message_id_for_wide_pointer(const wchar_t *text) const noexcept;
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
            std::wstring raw_wide;
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

        [[nodiscard]] const smgpc::render::GXLightState *light(std::size_t index) const;
        [[nodiscard]] std::span<const smgpc::render::GXLightState> lights() const;
        [[nodiscard]] const std::optional<smgpc::render::GXColorValue> &actor_ambient() const;
        [[nodiscard]] std::uint8_t loaded_mask() const;

    private:
        std::array<smgpc::render::GXLightState, 8U> _lights = {};
        std::optional<smgpc::render::GXColorValue> _actor_ambient = {};
    };

}  // namespace smgpc::runtime
