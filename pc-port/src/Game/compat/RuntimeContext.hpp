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

namespace smgpc::game {

    class RuntimeContext final {
    public:
        struct J3dRuntimePacketTrace {
            std::string model_name;
            std::uint64_t frame_index = 0U;
            std::string draw_pass;
            J3dRendererPacketState state{};
        };

        RuntimeContext(logging::ILogger &logger, render::IWindowService &window_service);
        ~RuntimeContext();

        RuntimeContext(const RuntimeContext &) = delete;
        RuntimeContext &operator=(const RuntimeContext &) = delete;

        static RuntimeContext &instance();
        static RuntimeContext *try_instance();

        void begin_frame(const render::FrameContext &frame_context);
        void draw_3d_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose);
        void draw_2d_normal(render::IRendererEngine &renderer);

        [[nodiscard]] bool is_core_pad_button_a(s32 channel) const;
        [[nodiscard]] bool is_core_pad_button_b(s32 channel) const;
        [[nodiscard]] std::uint64_t frame_index() const;
        [[nodiscard]] const std::optional<CameraPoseCompat> &last_camera_pose() const;
        [[nodiscard]] std::span<const J3dRuntimePacketTrace> j3d_packet_trace() const;
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
        [[nodiscard]] SaveDataService &save_data();
        [[nodiscard]] const SaveDataService &save_data() const;
        [[nodiscard]] MessageService &messages();
        [[nodiscard]] const MessageService &messages() const;
        [[nodiscard]] RflService &rfl();
        [[nodiscard]] const RflService &rfl() const;
        [[nodiscard]] SceneScheduler &scheduler();
        [[nodiscard]] const SceneScheduler &scheduler() const;

        void start_stage_bgm(std::string_view name);
        void unlock_stage_bgm();
        void stop_stage_bgm(s32 fade_frames);
        void start_system_sound(std::string_view name);
        void start_cs_sound(std::string_view name);
        void emit_effect(std::string_view actor_name, std::string_view effect_name);
        void delete_effect_all(std::string_view actor_name);
        void note_layout_archive(std::string_view layout_name, const std::filesystem::path &path);
        void note_missing_layout_archive(std::string_view layout_name);
        void note_layout_texture_decode_failed(std::string_view layout_name, std::string_view texture_name, std::string_view reason);
        void note_object_archive(std::string_view object_name, const std::filesystem::path &path);
        void note_missing_object_archive(std::string_view object_name);
        void note_object_texture_decode_failed(std::string_view object_name, std::string_view reason);
        void note_debug_event(std::string_view message);
        void record_j3d_packet_trace(std::string_view model_name, std::uint64_t frame_index, std::string_view draw_pass,
                                     const J3dRendererPacketState &packet);
        void register_layout(SimpleLayout &layout);
        void unregister_layout(SimpleLayout &layout);
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
        SaveDataService _save_data;
        MessageService _messages;
        RflService _rfl;
        SceneScheduler _scheduler;
        std::uint64_t _frame_index = 0;
        std::optional<CameraPoseCompat> _last_camera_pose{};
        std::vector<J3dRuntimePacketTrace> _j3d_packet_trace{};
        std::optional<std::uint64_t> _hold_title_combo_frame{};
    };

}  // namespace smgpc::game
