#include "RuntimeContext.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <vector>

#include "Game/Screen/SimpleLayout.hpp"

namespace smgpc::game {
    namespace {

        RuntimeContext* s_runtime_context = nullptr;

        [[nodiscard]] bool exists_regular_file(const std::filesystem::path& path) {
            std::error_code error{};
            return std::filesystem::is_regular_file(path, error);
        }

    }  // namespace

    RuntimeContext::RuntimeContext(logging::ILogger& logger, render::IWindowService& window_service)
        : _logger(logger), _window_service(window_service), _disc_files_root(resolve_disc_files_root()) {
        if (s_runtime_context != nullptr) {
            throw std::logic_error("Only one SMG runtime context may be active.");
        }

        s_runtime_context = this;
        _logger.info(logging::Category::APP, logging::Message{"Using SMG disc files from {}"}, _disc_files_root.string());
    }

    RuntimeContext::~RuntimeContext() {
        if (s_runtime_context == this) {
            s_runtime_context = nullptr;
        }
    }

    RuntimeContext& RuntimeContext::instance() {
        if (s_runtime_context == nullptr) {
            throw std::logic_error("SMG runtime context is not active.");
        }

        return *s_runtime_context;
    }

    RuntimeContext* RuntimeContext::try_instance() {
        return s_runtime_context;
    }

    void RuntimeContext::begin_frame(const render::FrameContext& frame_context) {
        _frame_index = frame_context.frame_index;
        for (auto* layout : _layouts) {
            if (layout != nullptr) {
                layout->update();
            }
        }
    }

    void RuntimeContext::draw_layouts(render::IRendererEngine& renderer) {
        for (auto* layout : _layouts) {
            if (layout != nullptr) {
                layout->draw(renderer);
            }
        }
    }

    bool RuntimeContext::is_core_pad_button_a(s32 channel) const {
        return channel == WPAD_CHAN0 && _window_service.is_input_pressed(render::InputButton::CORE_PAD_A);
    }

    bool RuntimeContext::is_core_pad_button_b(s32 channel) const {
        return channel == WPAD_CHAN0 && _window_service.is_input_pressed(render::InputButton::CORE_PAD_B);
    }

    bool RuntimeContext::is_stage_bgm_prepared() const {
        return _stage_bgm_requested && _frame_index > _bgm_start_frame;
    }

    std::optional< std::filesystem::path > RuntimeContext::find_layout_archive(std::string_view layout_name) const {
        const auto archive_name = std::string(layout_name) + ".arc";
        const std::vector< std::filesystem::path > candidates{
            _disc_files_root / "KrKorean" / "LayoutData" / archive_name,
            _disc_files_root / "LayoutData" / archive_name,
        };

        for (const auto& candidate : candidates) {
            if (exists_regular_file(candidate)) {
                return candidate;
            }
        }

        return std::nullopt;
    }

    std::optional< std::filesystem::path > RuntimeContext::find_object_archive(std::string_view object_name) const {
        const auto archive_name = std::string(object_name) + ".arc";
        const auto candidate = _disc_files_root / "ObjectData" / archive_name;
        if (exists_regular_file(candidate)) {
            return candidate;
        }

        return std::nullopt;
    }

    void RuntimeContext::start_stage_bgm(std::string_view name) {
        _stage_bgm_requested = true;
        _bgm_start_frame = _frame_index;
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct requested stage BGM {}"}, name);
    }

    void RuntimeContext::unlock_stage_bgm() {
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct unlocked stage BGM"});
    }

    void RuntimeContext::stop_stage_bgm(s32 fade_frames) {
        _stage_bgm_requested = false;
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct stopped stage BGM over {} frames"}, fade_frames);
    }

    void RuntimeContext::start_system_sound(std::string_view name) {
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct requested system sound {}"}, name);
    }

    void RuntimeContext::start_cs_sound(std::string_view name) {
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct requested controller speaker sound {}"}, name);
    }

    void RuntimeContext::emit_effect(std::string_view actor_name, std::string_view effect_name) {
        _logger.info(logging::Category::APP, logging::Message{"{} emitted effect {}"}, actor_name, effect_name);
    }

    void RuntimeContext::delete_effect_all(std::string_view actor_name) {
        _logger.info(logging::Category::APP, logging::Message{"{} deleted all effects"}, actor_name);
    }

    void RuntimeContext::note_layout_archive(std::string_view layout_name, const std::filesystem::path& path) {
        _logger.info(logging::Category::APP, logging::Message{"Resolved original layout archive {} -> {}"}, layout_name, path.string());
    }

    void RuntimeContext::note_missing_layout_archive(std::string_view layout_name) {
        _logger.warning(logging::Category::APP, logging::Message{"Missing original layout archive for {}"}, layout_name);
    }

    void RuntimeContext::note_layout_texture_decode_failed(std::string_view layout_name, std::string_view texture_name, std::string_view reason) {
        _logger.warning(logging::Category::APP, logging::Message{"Skipped unsupported layout texture {} from {}: {}"}, texture_name, layout_name,
                        reason);
    }

    void RuntimeContext::note_object_archive(std::string_view object_name, const std::filesystem::path& path) {
        _logger.info(logging::Category::APP, logging::Message{"Resolved original object archive {} -> {}"}, object_name, path.string());
    }

    void RuntimeContext::note_missing_object_archive(std::string_view object_name) {
        _logger.warning(logging::Category::APP, logging::Message{"Missing original object archive for {}"}, object_name);
    }

    void RuntimeContext::note_object_texture_decode_failed(std::string_view object_name, std::string_view reason) {
        _logger.warning(logging::Category::APP, logging::Message{"Skipped object texture data from {}: {}"}, object_name, reason);
    }

    void RuntimeContext::note_debug_event(std::string_view message) {
        _logger.info(logging::Category::APP, logging::Message{"SMG debug: {}"}, message);
    }

    void RuntimeContext::register_layout(SimpleLayout& layout) {
        if (std::ranges::find(_layouts, &layout) == _layouts.end()) {
            _layouts.push_back(&layout);
        }
    }

    void RuntimeContext::unregister_layout(SimpleLayout& layout) {
        std::erase(_layouts, &layout);
    }

    std::filesystem::path RuntimeContext::resolve_disc_files_root() const {
        const auto cwd = std::filesystem::current_path();
        const std::vector< std::filesystem::path > candidates{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
            cwd / ".." / "orig" / "RMGK01" / "files",
        };

        for (const auto& candidate : candidates) {
            std::error_code error{};
            auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        return candidates.front();
    }

}  // namespace smgpc::game

OSTime OSGetTime() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast< std::chrono::seconds >(now).count();
}

s64 OSTicksToSeconds(OSTime ticks) {
    return ticks;
}
