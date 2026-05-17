#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <revolution.h>

#include "Game/compat/CameraPose.hpp"
#include "Game/compat/J3dModelRenderer.hpp"
#include "Game/compat/RuntimeServices.hpp"
#include "Game/compat/SceneScheduler.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"

class SimpleLayout;
class LiveActor;
class LayoutActor;

namespace smgpc::game {

    class RuntimeContext final {
    public:
#ifndef NDEBUG
        struct J3dRuntimePacketTrace {
            std::string model_name;
            std::uint64_t frame_index = 0U;
            std::string draw_pass;
            J3dRendererPacketState state{};
        };
#endif

        struct GxPixelUpdateState {
            bool color_update = true;
            bool alpha_update = true;
        };

#ifndef NDEBUG
        struct SemanticTraceEvent {
            std::uint64_t index = 0U;
            std::uint64_t frame_index = 0U;
            std::string category;
            std::string name;
            std::string detail;
            std::string stage_name;
        };
#endif

        RuntimeContext(logging::ILogger &logger, render::IWindowService &window_service);
        ~RuntimeContext();

        RuntimeContext(const RuntimeContext &) = delete;
        RuntimeContext &operator=(const RuntimeContext &) = delete;

        static RuntimeContext &instance();
        static RuntimeContext *try_instance();

        void begin_frame(const render::FrameContext &frame_context);
        void set_scene_camera_pose(const CameraPoseCompat &camera_pose);
        void draw_3d_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose);
        void draw_3d_normal(render::IRendererEngine &renderer);
        void draw_2d_normal(render::IRendererEngine &renderer);
#ifndef NDEBUG
        void set_j3d_packet_trace_frame(std::optional<std::uint64_t> frame_index);
#endif
        void set_j3d_pixel_update_state(std::optional<GxPixelUpdateState> state);
        void set_current_stage_name(std::string_view stage_name);
        void set_current_sequence_scene_name(std::string_view scene_name);
        void set_next_sequence_scene_name(std::string_view scene_name);

        [[nodiscard]] bool is_core_pad_button_a(s32 channel) const;
        [[nodiscard]] bool is_core_pad_button_b(s32 channel) const;
        [[nodiscard]] std::uint64_t frame_index() const;
        [[nodiscard]] const std::optional<CameraPoseCompat> &last_camera_pose() const;
#ifndef NDEBUG
        [[nodiscard]] std::span<const J3dRuntimePacketTrace> j3d_packet_trace() const;
        [[nodiscard]] std::span<const SemanticTraceEvent> semantic_trace_events() const;
        [[nodiscard]] bool should_record_j3d_packet_trace() const;
#endif
        [[nodiscard]] const std::optional<GxPixelUpdateState> &j3d_pixel_update_state() const;
        [[nodiscard]] std::string_view current_stage_name() const;
        [[nodiscard]] std::string_view current_sequence_scene_name() const;
        [[nodiscard]] std::string_view next_sequence_scene_name() const;
        [[nodiscard]] bool is_stage_bgm_prepared() const;
        [[nodiscard]] std::string_view current_stage_bgm_name() const;
        [[nodiscard]] std::optional<std::filesystem::path> find_layout_archive(std::string_view layout_name) const;
        [[nodiscard]] std::optional<std::filesystem::path> find_object_archive(std::string_view object_name) const;
        [[nodiscard]] DvdFileSystemService &dvd();
        [[nodiscard]] const DvdFileSystemService &dvd() const;
        [[nodiscard]] WpadService &wpad();
        [[nodiscard]] const WpadService &wpad() const;
        [[nodiscard]] AudioEventService &audio();
        [[nodiscard]] const AudioEventService &audio() const;
        [[nodiscard]] EffectService &effects();
        [[nodiscard]] const EffectService &effects() const;
        [[nodiscard]] WipeService &scene_wipe();
        [[nodiscard]] const WipeService &scene_wipe() const;
        [[nodiscard]] WipeService &system_wipe();
        [[nodiscard]] const WipeService &system_wipe() const;
        [[nodiscard]] StarPointerService &star_pointer();
        [[nodiscard]] const StarPointerService &star_pointer() const;
        [[nodiscard]] CameraSystemService &camera_system();
        [[nodiscard]] const CameraSystemService &camera_system() const;
        [[nodiscard]] PlayerSystemService &player_system();
        [[nodiscard]] const PlayerSystemService &player_system() const;
        [[nodiscard]] GameLayoutService &game_layout();
        [[nodiscard]] const GameLayoutService &game_layout() const;
        [[nodiscard]] RumbleService &rumble();
        [[nodiscard]] const RumbleService &rumble() const;
        [[nodiscard]] SequenceRequestService &sequence_requests();
        [[nodiscard]] const SequenceRequestService &sequence_requests() const;
        [[nodiscard]] SaveDataService &save_data();
        [[nodiscard]] const SaveDataService &save_data() const;
        [[nodiscard]] MessageService &messages();
        [[nodiscard]] const MessageService &messages() const;
        [[nodiscard]] SceneLightService &scene_lights();
        [[nodiscard]] const SceneLightService &scene_lights() const;
        [[nodiscard]] RflService &rfl();
        [[nodiscard]] const RflService &rfl() const;
        [[nodiscard]] SceneScheduler &scheduler();
        [[nodiscard]] const SceneScheduler &scheduler() const;

        void start_stage_bgm(std::string_view name);
        void unlock_stage_bgm();
        void stop_stage_bgm(s32 fade_frames);
        void set_stage_bgm_state(s32 state, u32 change_frames);
        void start_system_sound(std::string_view name);
        void start_cs_sound(std::string_view name);
        void emit_effect(std::string_view actor_name, std::string_view effect_name);
        void delete_effect(std::string_view actor_name, std::string_view effect_name);
        void delete_effect_all(std::string_view actor_name);
        void note_layout_archive(std::string_view layout_name, const std::filesystem::path &path);
        void note_missing_layout_archive(std::string_view layout_name);
        void note_layout_texture_decode_failed(std::string_view layout_name, std::string_view texture_name, std::string_view reason);
        void note_object_archive(std::string_view object_name, const std::filesystem::path &path);
        void note_missing_object_archive(std::string_view object_name);
        void note_object_texture_decode_failed(std::string_view object_name, std::string_view reason);
#ifndef NDEBUG
        void note_debug_event(std::string_view message);
        void emit_semantic_trace_event(std::string_view category, std::string_view name, std::string_view detail = {});
        void emit_sequence_state_trace_event(std::string_view name, std::string_view detail = {}, std::string_view draw_phase = {});
        void record_j3d_packet_trace(std::string_view model_name, std::uint64_t frame_index, std::string_view draw_pass,
                                     const J3dRendererPacketState &packet);
#endif
        void register_layout(SimpleLayout &layout);
        void unregister_layout(SimpleLayout &layout);
        void register_layout_actor(LayoutActor &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type);
        void unregister_layout_actor(LayoutActor &layout);
        void register_live_actor_model(LiveActor &actor, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type);
        void unregister_live_actor_model(LiveActor &actor);
        void register_sky_actor(LiveActor &actor);
        void unregister_sky_actor(LiveActor &actor);

    private:
        [[nodiscard]] std::filesystem::path resolve_disc_files_root() const;

        logging::ILogger &_logger;
        render::IWindowService &_window_service;
        std::filesystem::path _disc_files_root;
        DvdFileSystemService _dvd;
        WpadService _wpad;
        AudioEventService _audio;
        EffectService _effects;
        WipeService _scene_wipe;
        WipeService _system_wipe;
        StarPointerService _star_pointer;
        CameraSystemService _camera_system;
        PlayerSystemService _player_system;
        GameLayoutService _game_layout;
        RumbleService _rumble;
        SequenceRequestService _sequence_requests;
        SaveDataService _save_data;
        MessageService _messages;
        SceneLightService _scene_lights;
        RflService _rfl;
        SceneScheduler _scheduler;
        std::uint64_t _frame_index = 0;
        std::optional<CameraPoseCompat> _scene_camera_pose{};
        std::optional<CameraPoseCompat> _last_camera_pose{};
        std::optional<GxPixelUpdateState> _j3d_pixel_update_state{};
        std::string _current_stage_name;
        std::string _current_sequence_scene_name = "Game";
        std::string _next_sequence_scene_name;
#ifndef NDEBUG
        std::vector<J3dRuntimePacketTrace> _j3d_packet_trace{};
        std::vector<SemanticTraceEvent> _semantic_trace_events{};
        std::optional<std::uint64_t> _j3d_packet_trace_frame{};
        std::optional<std::uint64_t> _hold_title_combo_frame{};
        std::uint64_t _next_semantic_trace_event_index = 0U;
        bool _emitted_title_combo_held_event = false;
#endif
    };

}  // namespace smgpc::game
