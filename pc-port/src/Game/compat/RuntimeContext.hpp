#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <revolution.h>

#include "Logger.hpp"
#include "RendererService.hpp"

class SimpleLayout;

namespace smgpc::game {

    class RuntimeContext final {
    public:
        RuntimeContext(logging::ILogger& logger, render::IWindowService& window_service);
        ~RuntimeContext();

        RuntimeContext(const RuntimeContext&) = delete;
        RuntimeContext& operator=(const RuntimeContext&) = delete;

        static RuntimeContext& instance();
        static RuntimeContext* try_instance();

        void begin_frame(const render::FrameContext& frame_context);
        void draw_layouts(render::IRendererEngine& renderer);

        [[nodiscard]] bool is_core_pad_button_a(s32 channel) const;
        [[nodiscard]] bool is_core_pad_button_b(s32 channel) const;
        [[nodiscard]] bool is_stage_bgm_prepared() const;
        [[nodiscard]] std::optional< std::filesystem::path > find_layout_archive(std::string_view layout_name) const;
        [[nodiscard]] std::optional< std::filesystem::path > find_object_archive(std::string_view object_name) const;

        void start_stage_bgm(std::string_view name);
        void unlock_stage_bgm();
        void stop_stage_bgm(s32 fade_frames);
        void start_system_sound(std::string_view name);
        void start_cs_sound(std::string_view name);
        void emit_effect(std::string_view actor_name, std::string_view effect_name);
        void delete_effect_all(std::string_view actor_name);
        void note_layout_archive(std::string_view layout_name, const std::filesystem::path& path);
        void note_missing_layout_archive(std::string_view layout_name);
        void note_layout_texture_decode_failed(std::string_view layout_name, std::string_view texture_name, std::string_view reason);
        void note_object_archive(std::string_view object_name, const std::filesystem::path& path);
        void note_missing_object_archive(std::string_view object_name);
        void note_object_texture_decode_failed(std::string_view object_name, std::string_view reason);
        void note_debug_event(std::string_view message);
        void register_layout(SimpleLayout& layout);
        void unregister_layout(SimpleLayout& layout);

    private:
        [[nodiscard]] std::filesystem::path resolve_disc_files_root() const;

        logging::ILogger& _logger;
        render::IWindowService& _window_service;
        std::filesystem::path _disc_files_root;
        std::uint64_t _frame_index = 0;
        std::uint64_t _bgm_start_frame = 0;
        bool _stage_bgm_requested = false;
        std::vector< SimpleLayout* > _layouts{};
    };

}  // namespace smgpc::game
