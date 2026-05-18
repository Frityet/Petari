#include "RuntimeContext.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/CaptureScreenDirector.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/ScreenAlphaCapture.hpp"
#include "Game/Screen/SimpleLayout.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/compat/CameraParam.hpp"
#include "Game/compat/SceneLifecycleService.hpp"

namespace smgpc::game {
    namespace {

        RuntimeContext *s_runtime_context = nullptr;

        [[nodiscard]] std::filesystem::path weakly_canonical_or_normal(const std::filesystem::path &path) {
            std::error_code error{};
            const auto canonical = std::filesystem::weakly_canonical(path, error);
            if (!error) {
                return canonical;
            }

            return path.lexically_normal();
        }

        [[nodiscard]] std::optional<std::filesystem::path> read_path_environment(std::string_view name) {
            const auto key = std::string(name);
            const auto *value = std::getenv(key.c_str());
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            return weakly_canonical_or_normal(std::filesystem::path(value));
        }

        void append_disc_root_candidates_from_anchor(std::vector<std::filesystem::path> &candidates, std::filesystem::path anchor) {
            if (anchor.empty()) {
                return;
            }

            auto directory = weakly_canonical_or_normal(anchor);
            while (!directory.empty()) {
                candidates.push_back(directory / "orig" / "RMGK01" / "files");
                if (directory == directory.root_path()) {
                    break;
                }

                directory = directory.parent_path();
            }
        }

        [[nodiscard]] std::optional<std::filesystem::path> executable_directory() {
#if defined(__linux__)
            std::error_code error{};
            const auto executable_path = std::filesystem::read_symlink("/proc/self/exe", error);
            if (!error && executable_path.has_parent_path()) {
                return executable_path.parent_path();
            }
#endif

            return std::nullopt;
        }

#ifndef NDEBUG
        [[nodiscard]] std::string_view trim(std::string_view text) {
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
                text.remove_prefix(1U);
            }
            while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
                text.remove_suffix(1U);
            }
            return text;
        }

        [[nodiscard]] std::optional<std::uint64_t> read_frame_index_environment(std::string_view name) {
            const auto key = std::string(name);
            const auto *value = std::getenv(key.c_str());
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            auto frame = std::uint64_t{};
            const auto text = std::string_view(value);
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, frame);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }

            return frame;
        }

        [[nodiscard]] std::optional<std::uint64_t> parse_frame_index(std::string_view text) {
            text = trim(text);
            if (text.empty()) {
                return std::nullopt;
            }

            auto frame = std::uint64_t{};
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, frame);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }
            return frame;
        }

        [[nodiscard]] std::optional<float> parse_float(std::string_view text) {
            text = trim(text);
            if (text.empty()) {
                return std::nullopt;
            }

            auto value = 0.0F;
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }
            return value;
        }

        [[nodiscard]] std::optional<bool> parse_bool(std::string_view text) {
            text = trim(text);
            if (text == "1" || text == "true" || text == "TRUE" || text == "True" || text == "on" || text == "ON") {
                return true;
            }
            if (text == "0" || text == "false" || text == "FALSE" || text == "False" || text == "off" || text == "OFF") {
                return false;
            }
            return std::nullopt;
        }

        struct DebugFrameRange {
            std::uint64_t first_frame = 0U;
            std::uint64_t last_frame = std::numeric_limits<std::uint64_t>::max();
        };

        [[nodiscard]] std::optional<DebugFrameRange> parse_debug_frame_range(std::string_view text) {
            text = trim(text);
            if (text.empty()) {
                return std::nullopt;
            }

            const auto dash = text.find('-');
            if (dash == std::string_view::npos) {
                const auto frame = parse_frame_index(text);
                if (!frame.has_value()) {
                    return std::nullopt;
                }
                return DebugFrameRange{.first_frame = *frame, .last_frame = *frame};
            }

            const auto first = parse_frame_index(text.substr(0U, dash));
            if (!first.has_value()) {
                return std::nullopt;
            }

            auto last = std::numeric_limits<std::uint64_t>::max();
            const auto last_text = trim(text.substr(dash + 1U));
            if (!last_text.empty()) {
                const auto parsed_last = parse_frame_index(last_text);
                if (!parsed_last.has_value() || *parsed_last < *first) {
                    return std::nullopt;
                }
                last = *parsed_last;
            }

            return DebugFrameRange{.first_frame = *first, .last_frame = last};
        }

        [[nodiscard]] std::optional<std::uint32_t> debug_wpad_button_mask(std::string_view text) {
            text = trim(text);
            if (text == "A") {
                return WPAD_BUTTON_A;
            }
            if (text == "B") {
                return WPAD_BUTTON_B;
            }
            if (text == "UP") {
                return WPAD_BUTTON_UP;
            }
            if (text == "DOWN") {
                return WPAD_BUTTON_DOWN;
            }
            if (text == "LEFT") {
                return WPAD_BUTTON_LEFT;
            }
            if (text == "RIGHT") {
                return WPAD_BUTTON_RIGHT;
            }
            if (text == "PLUS" || text == "+") {
                return WPAD_BUTTON_PLUS;
            }
            if (text == "MINUS") {
                return WPAD_BUTTON_MINUS;
            }
            if (text == "HOME") {
                return WPAD_BUTTON_HOME;
            }
            if (text == "C") {
                return WPAD_BUTTON_C;
            }
            if (text == "Z") {
                return WPAD_BUTTON_Z;
            }
            if (text == "ONE" || text == "1") {
                return WPAD_BUTTON_1;
            }
            if (text == "TWO" || text == "2") {
                return WPAD_BUTTON_2;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::uint32_t parse_debug_wpad_button_mask(std::string_view text) {
            auto mask = std::uint32_t{};
            while (true) {
                const auto plus = text.find('+');
                const auto token = trim(text.substr(0U, plus));
                if (const auto button = debug_wpad_button_mask(token)) {
                    mask |= *button;
                }
                if (plus == std::string_view::npos) {
                    break;
                }
                text.remove_prefix(plus + 1U);
            }
            return mask;
        }

        [[nodiscard]] std::optional<std::string> read_debug_string_environment(std::string_view name) {
            const auto key = std::string(name);
            const auto *value = std::getenv(key.c_str());
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }
            return std::string(value);
        }

        [[nodiscard]] std::vector<RuntimeContext::DebugWpadButtonScriptSpan> read_debug_wpad_button_script_environment() {
            auto spans = std::vector<RuntimeContext::DebugWpadButtonScriptSpan>{};
            const auto script = read_debug_string_environment("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT");
            if (!script.has_value()) {
                return spans;
            }

            auto text = std::string_view(*script);
            while (!text.empty()) {
                const auto separator = text.find(';');
                const auto entry = trim(text.substr(0U, separator));
                if (!entry.empty()) {
                    const auto colon = entry.find(':');
                    if (colon != std::string_view::npos) {
                        const auto range = parse_debug_frame_range(entry.substr(0U, colon));
                        const auto mask = parse_debug_wpad_button_mask(entry.substr(colon + 1U));
                        if (range.has_value() && mask != 0U) {
                            spans.push_back(RuntimeContext::DebugWpadButtonScriptSpan{
                                .first_frame = range->first_frame,
                                .last_frame = range->last_frame,
                                .button_mask = mask,
                            });
                        }
                    }
                }
                if (separator == std::string_view::npos) {
                    break;
                }
                text.remove_prefix(separator + 1U);
            }
            return spans;
        }

        [[nodiscard]] std::vector<RuntimeContext::DebugWpadPointerScriptSpan> read_debug_wpad_pointer_script_environment() {
            auto spans = std::vector<RuntimeContext::DebugWpadPointerScriptSpan>{};
            const auto script = read_debug_string_environment("SMGPC_DEBUG_WPAD_POINTER_SCRIPT");
            if (!script.has_value()) {
                return spans;
            }

            auto text = std::string_view(*script);
            while (!text.empty()) {
                const auto separator = text.find(';');
                const auto entry = trim(text.substr(0U, separator));
                if (!entry.empty()) {
                    const auto colon = entry.find(':');
                    if (colon != std::string_view::npos) {
                        const auto range = parse_debug_frame_range(entry.substr(0U, colon));
                        auto values = entry.substr(colon + 1U);
                        const auto first_comma = values.find(',');
                        if (range.has_value() && first_comma != std::string_view::npos) {
                            const auto x = parse_float(values.substr(0U, first_comma));
                            values.remove_prefix(first_comma + 1U);
                            const auto second_comma = values.find(',');
                            const auto y = parse_float(values.substr(0U, second_comma));
                            auto valid = true;
                            if (second_comma != std::string_view::npos) {
                                if (const auto parsed_valid = parse_bool(values.substr(second_comma + 1U))) {
                                    valid = *parsed_valid;
                                }
                            }
                            if (x.has_value() && y.has_value()) {
                                spans.push_back(RuntimeContext::DebugWpadPointerScriptSpan{
                                    .first_frame = range->first_frame,
                                    .last_frame = range->last_frame,
                                    .x = *x,
                                    .y = *y,
                                    .valid = valid,
                                });
                            }
                        }
                    }
                }
                if (separator == std::string_view::npos) {
                    break;
                }
                text.remove_prefix(separator + 1U);
            }
            return spans;
        }

        [[nodiscard]] bool debug_span_active(std::uint64_t frame_index, std::uint64_t first_frame, std::uint64_t last_frame) {
            return frame_index >= first_frame && frame_index <= last_frame;
        }

        [[nodiscard]] std::optional<std::uint64_t> read_debug_hold_title_combo_frame_environment() {
            return read_frame_index_environment("SMGPC_HOLD_TITLE_COMBO_FRAME");
        }
#endif

        [[nodiscard]] std::optional<std::string> read_string_environment(std::string_view name) {
            const auto key = std::string(name);
            const auto *value = std::getenv(key.c_str());
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            return std::string(value);
        }

        [[nodiscard]] CameraPoseCompat default_scene_camera_pose() {
            return CameraPoseCompat{
                .eye = {0.0F, 0.0F, 0.0F},
                .watch = {0.0F, 0.0F, -1.0F},
                .up = {0.0F, 1.0F, 0.0F},
                .fovy_degrees = 45.0F,
                .aspect_ratio = 608.0F / 456.0F,
                .near_clip = 100.0F,
                .far_clip = 800000.0F,
            };
        }

#ifndef NDEBUG
        [[nodiscard]] std::string_view wipe_state_name(WipeState state) {
            switch (state) {
            case WipeState::Open:
                return "Open";
            case WipeState::Closed:
                return "Closed";
            case WipeState::Opening:
                return "Opening";
            case WipeState::Closing:
                return "Closing";
            }

            return "Unknown";
        }

        [[nodiscard]] std::string_view star_pointer_target_event_name(StarPointerTargetEventKind kind) {
            switch (kind) {
            case StarPointerTargetEventKind::Enter:
                return "target_enter";
            case StarPointerTargetEventKind::Leave:
                return "target_leave";
            case StarPointerTargetEventKind::Select:
                return "target_select";
            }

            return "target_unknown";
        }
#endif

        [[nodiscard]] std::string default_stage_name() {
#ifndef NDEBUG
            return read_string_environment("SMGPC_STAGE_NAME").value_or("FileSelect");
#else
            return "FileSelect";
#endif
        }

    }  // namespace

    RuntimeContext::RuntimeContext(logging::ILogger &logger, render::IWindowService &window_service)
        : _logger(logger), _window_service(window_service), _disc_files_root(resolve_disc_files_root()), _dvd(_disc_files_root), _current_stage_name(default_stage_name())
#ifndef NDEBUG
          ,
          _hold_title_combo_frame(read_debug_hold_title_combo_frame_environment()),
          _debug_wpad_button_script(read_debug_wpad_button_script_environment()),
          _debug_wpad_pointer_script(read_debug_wpad_pointer_script_environment())
#endif
    {
        if (s_runtime_context != nullptr) {
            throw std::logic_error("Only one SMG runtime context may be active.");
        }

        s_runtime_context = this;
        _scene_lifecycle = std::make_unique<SceneLifecycleService>(*this);
        _capture_screen_director = std::make_unique<CaptureScreenDirector>();
        MR::createScreenAlphaSceneObj(0, 1.0F);
        _capture_screen_indirect_actor = std::make_unique<CaptureScreenActor>(MR::DrawType_CaptureScreenIndirect, "Indirect");
        _capture_screen_camera_actor = std::make_unique<CaptureScreenActor>(MR::DrawType_CaptureScreenCamera, "Camera");
        _logger.info(logging::Category::APP, logging::Message{"Using SMG disc files from {}"}, _disc_files_root.string());
        if (const auto save_directory = read_path_environment("SMGPC_SAVE_DIR")) {
            _save_data.set_host_directory(*save_directory);
            _logger.info(logging::Category::APP, logging::Message{"Using SMG save files from {}"}, save_directory->string());
        }
        if (const auto message_archive = _dvd.find_first({
                std::filesystem::path("KrKorean") / "MessageData" / "Message.arc",
                std::filesystem::path("MessageData") / "Message.arc",
            })) {
            try {
                const auto count = _messages.load_message_archive(_dvd.archive_for_path(*message_archive));
                _logger.info(logging::Category::APP, logging::Message{"Loaded {} messages from {}"}, count, message_archive->string());
            } catch (const std::exception &error) {
                _logger.warning(logging::Category::APP, logging::Message{"Could not load original message archive {}: {}"}, message_archive->string(),
                                error.what());
            }
        }
        if (const auto effect_archive = _dvd.find_first({
                std::filesystem::path("ParticleData") / "Effect.arc",
            })) {
            try {
                _effects.load_resources(_dvd.archive_for_path(*effect_archive));
                if (const auto *resources = _effects.resource_library(); resources != nullptr) {
                    _logger.info(logging::Category::APP, logging::Message{"Loaded {} particle names, {} particle resources, and {} particle textures from {}"},
                                 resources->particle_name_count(), resources->resource_count(), resources->texture_count(), effect_archive->string());
                }
            } catch (const std::exception &error) {
                _logger.warning(logging::Category::APP, logging::Message{"Could not load original effect archive {}: {}"}, effect_archive->string(),
                                error.what());
            }
        }
#ifndef NDEBUG
        if (_hold_title_combo_frame.has_value()) {
            _logger.info(logging::Category::APP, logging::Message{"Debug title A+B hold starts at frame {}"}, *_hold_title_combo_frame);
        }
        if (!_debug_wpad_button_script.empty()) {
            _logger.info(logging::Category::APP, logging::Message{"Loaded {} debug WPAD button script spans"},
                         _debug_wpad_button_script.size());
        }
        if (!_debug_wpad_pointer_script.empty()) {
            _logger.info(logging::Category::APP, logging::Message{"Loaded {} debug WPAD pointer script spans"},
                         _debug_wpad_pointer_script.size());
        }
        emit_semantic_trace_event("runtime", "runtime_context_created", "disc_files_root=" + _disc_files_root.generic_string());
#endif
    }

    RuntimeContext::~RuntimeContext() {
        _scene_lifecycle.reset();
        _capture_screen_camera_actor.reset();
        _capture_screen_indirect_actor.reset();
        _capture_screen_director.reset();
        if (s_runtime_context == this) {
            s_runtime_context = nullptr;
        }
    }

    RuntimeContext &RuntimeContext::instance() {
        if (s_runtime_context == nullptr) {
            throw std::logic_error("SMG runtime context is not active.");
        }

        return *s_runtime_context;
    }

    RuntimeContext *RuntimeContext::try_instance() {
        return s_runtime_context;
    }

    void RuntimeContext::begin_frame(const render::FrameContext &frame_context) {
        _frame_index = frame_context.frame_index;
        _copy_events.clear();
#ifndef NDEBUG
        _j3d_packet_trace.clear();
        _layout_packet_trace.clear();
#endif
        _j3d_pixel_update_state.reset();
        _scene_camera_pose.reset();
        if (const auto camera_pose = _camera_system.active_programmable_camera_pose()) {
            _scene_camera_pose = *camera_pose;
        }
        _audio.begin_frame(_frame_index);
        _effects.begin_frame(_frame_index);
        _scene_wipe.begin_frame(_frame_index);
        _system_wipe.begin_frame(_frame_index);
        _star_pointer.begin_frame(_frame_index);
        _rumble.begin_frame(_frame_index);
        _sequence_requests.begin_frame(_frame_index);
        _wpad.begin_frame();

        auto hold_mask = std::uint32_t{};
        const auto append_input_button = [this, &hold_mask](render::InputButton button, std::uint32_t mask) {
            if (_window_service.is_input_pressed(button)) {
                hold_mask |= mask;
            }
        };
        const auto debug_title_combo_held =
#ifndef NDEBUG
            _hold_title_combo_frame.has_value() && _frame_index >= *_hold_title_combo_frame;
#else
            false;
#endif
        append_input_button(render::InputButton::CORE_PAD_A, WPAD_BUTTON_A);
        append_input_button(render::InputButton::CORE_PAD_B, WPAD_BUTTON_B);
        append_input_button(render::InputButton::CORE_PAD_UP, WPAD_BUTTON_UP);
        append_input_button(render::InputButton::CORE_PAD_DOWN, WPAD_BUTTON_DOWN);
        append_input_button(render::InputButton::CORE_PAD_LEFT, WPAD_BUTTON_LEFT);
        append_input_button(render::InputButton::CORE_PAD_RIGHT, WPAD_BUTTON_RIGHT);
        append_input_button(render::InputButton::CORE_PAD_PLUS, WPAD_BUTTON_PLUS);
        append_input_button(render::InputButton::CORE_PAD_MINUS, WPAD_BUTTON_MINUS);
        append_input_button(render::InputButton::CORE_PAD_HOME, WPAD_BUTTON_HOME);
        append_input_button(render::InputButton::CORE_PAD_C, WPAD_BUTTON_C);
        append_input_button(render::InputButton::CORE_PAD_Z, WPAD_BUTTON_Z);
        if (debug_title_combo_held) {
            hold_mask |= WPAD_BUTTON_A | WPAD_BUTTON_B;
        }
        auto pointer = _window_service.input_pointer_state();
#ifndef NDEBUG
        for (const auto &span : _debug_wpad_button_script) {
            if (debug_span_active(_frame_index, span.first_frame, span.last_frame)) {
                hold_mask |= span.button_mask;
            }
        }
        for (const auto &span : _debug_wpad_pointer_script) {
            if (debug_span_active(_frame_index, span.first_frame, span.last_frame)) {
                pointer = render::InputPointerState{
                    .x = span.x,
                    .y = span.y,
                    .valid = span.valid,
                };
            }
        }
#endif
        _wpad.set_connected(WPAD_CHAN0, true);
        _wpad.set_button_mask(WPAD_CHAN0, hold_mask);
        _wpad.set_pointer(WPAD_CHAN0, pointer.x, pointer.y, pointer.valid);
        _wpad.set_distance_to_display(WPAD_CHAN0, pointer.valid ? 1.0F : 0.0F);
#ifndef NDEBUG
        if (!_emitted_title_combo_held_event && (hold_mask & (WPAD_BUTTON_A | WPAD_BUTTON_B)) == (WPAD_BUTTON_A | WPAD_BUTTON_B)) {
            _emitted_title_combo_held_event = true;
            emit_semantic_trace_event("input", "title_combo_held", "WPAD_CHAN0 A+B held");
        }
#endif

        smgpc::game::save_data_handle_sequence().update();
        if (_scene_lifecycle->active_scene() != nullptr) {
            _scene_lifecycle->update_scene();
            _scene_lifecycle->calc_anim_scene();
        } else {
            _scheduler.execute_movement();
            _scheduler.execute_calc_anim();
            _scheduler.execute_calc_view_and_entry();
        }
    }

    void RuntimeContext::set_scene_camera_pose(const CameraPoseCompat &camera_pose) {
        _scene_camera_pose = camera_pose;
    }

    void RuntimeContext::record_copy_event(render::CopyEvent event) {
        event.index = _copy_events.size();
        if (event.event_index == 0U) {
            event.event_index = _frame_index;
        }
        if (event.presenter_frame_count == 0U) {
            event.presenter_frame_count = _frame_index;
        }
        _copy_events.push_back(std::move(event));
    }

    void RuntimeContext::draw_3d_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose) {
        _last_camera_pose = camera_pose;
        if (!_game_layout.is_game_scene_draw_3d_active()) {
            return;
        }
#ifndef NDEBUG
        if (should_record_j3d_packet_trace()) {
            emit_sequence_state_trace_event("draw_3d_normal", {}, "3d_normal");
        }
#endif
        if (_scene_lifecycle->active_scene() != nullptr) {
            _scene_lifecycle->draw_3d_normal(renderer, camera_pose);
            return;
        }

        _scheduler.execute_draw_buffer_list_normal(renderer, camera_pose);
        _scheduler.execute_draw_type(renderer, MR::DrawType_EffectDraw3D);
        _scheduler.execute_draw_type(renderer, MR::DrawType_EffectDrawForBloomEffect);
        _scheduler.execute_draw_type(renderer, MR::DrawType_CaptureScreenIndirect);
    }

    void RuntimeContext::draw_3d_normal(render::IRendererEngine &renderer) {
        if (!_scene_camera_pose.has_value()) {
#ifndef NDEBUG
            emit_semantic_trace_event("camera", "missing_scene_camera_pose", "using default_scene_camera_pose");
#endif
            draw_3d_normal(renderer, default_scene_camera_pose());
            return;
        }

        draw_3d_normal(renderer, *_scene_camera_pose);
    }

    void RuntimeContext::draw_2d_normal(render::IRendererEngine &renderer) {
#ifndef NDEBUG
        if (should_record_j3d_packet_trace()) {
            emit_sequence_state_trace_event("draw_2d_normal", {}, "2d_normal");
        }
#endif
        if (_scene_lifecycle->active_scene() != nullptr) {
            _scene_lifecycle->draw_2d_normal(renderer);
            return;
        }

        _scheduler.execute_draw_list_2d_normal(renderer);
    }

#ifndef NDEBUG
    void RuntimeContext::set_j3d_packet_trace_frame(std::optional<std::uint64_t> frame_index) {
        _j3d_packet_trace_frame = frame_index;
    }
#endif

    void RuntimeContext::set_j3d_pixel_update_state(std::optional<GxPixelUpdateState> state) {
        _j3d_pixel_update_state = state;
    }

    void RuntimeContext::set_current_stage_name(std::string_view stage_name) {
        _current_stage_name = stage_name;
    }

    void RuntimeContext::set_current_sequence_scene_name(std::string_view scene_name) {
        _current_sequence_scene_name = scene_name;
    }

    void RuntimeContext::set_next_sequence_scene_name(std::string_view scene_name) {
        _next_sequence_scene_name = scene_name;
    }

    bool RuntimeContext::is_core_pad_button_a(s32 channel) const {
        return _wpad.is_button_held(channel, WPAD_BUTTON_A);
    }

    bool RuntimeContext::is_core_pad_button_b(s32 channel) const {
        return _wpad.is_button_held(channel, WPAD_BUTTON_B);
    }

    std::uint64_t RuntimeContext::frame_index() const {
        return _frame_index;
    }

    const std::optional<CameraPoseCompat> &RuntimeContext::scene_camera_pose() const {
        return _scene_camera_pose;
    }

    const std::optional<CameraPoseCompat> &RuntimeContext::last_camera_pose() const {
        return _last_camera_pose;
    }

    std::span<const render::CopyEvent> RuntimeContext::copy_events() const {
        return _copy_events;
    }

#ifndef NDEBUG
    std::span<const RuntimeContext::J3dRuntimePacketTrace> RuntimeContext::j3d_packet_trace() const {
        return _j3d_packet_trace;
    }

    std::span<const RuntimeContext::LayoutRuntimePacketTrace> RuntimeContext::layout_packet_trace() const {
        return _layout_packet_trace;
    }

    std::span<const RuntimeContext::SemanticTraceEvent> RuntimeContext::semantic_trace_events() const {
        return _semantic_trace_events;
    }

    bool RuntimeContext::should_record_j3d_packet_trace() const {
        return _j3d_packet_trace_frame.has_value() && _frame_index == *_j3d_packet_trace_frame;
    }

    bool RuntimeContext::should_record_render_packet_trace() const {
        return should_record_j3d_packet_trace();
    }
#endif

    const std::optional<RuntimeContext::GxPixelUpdateState> &RuntimeContext::j3d_pixel_update_state() const {
        return _j3d_pixel_update_state;
    }

    std::string_view RuntimeContext::current_stage_name() const {
        return _current_stage_name;
    }

    std::string_view RuntimeContext::current_sequence_scene_name() const {
        return _current_sequence_scene_name;
    }

    std::string_view RuntimeContext::next_sequence_scene_name() const {
        return _next_sequence_scene_name;
    }

    bool RuntimeContext::is_stage_bgm_prepared() const {
        return _audio.is_stage_bgm_prepared();
    }

    std::string_view RuntimeContext::current_stage_bgm_name() const {
        return _audio.current_stage_bgm_name();
    }

    std::optional<std::filesystem::path> RuntimeContext::find_layout_archive(std::string_view layout_name) const {
        return _dvd.find_layout_archive(layout_name);
    }

    std::optional<std::filesystem::path> RuntimeContext::find_object_archive(std::string_view object_name) const {
        return _dvd.find_object_archive(object_name);
    }

    DvdFileSystemService &RuntimeContext::dvd() {
        return _dvd;
    }

    const DvdFileSystemService &RuntimeContext::dvd() const {
        return _dvd;
    }

    WpadService &RuntimeContext::wpad() {
        return _wpad;
    }

    const WpadService &RuntimeContext::wpad() const {
        return _wpad;
    }

    AudioEventService &RuntimeContext::audio() {
        return _audio;
    }

    const AudioEventService &RuntimeContext::audio() const {
        return _audio;
    }

    EffectService &RuntimeContext::effects() {
        return _effects;
    }

    const EffectService &RuntimeContext::effects() const {
        return _effects;
    }

    WipeService &RuntimeContext::scene_wipe() {
        return _scene_wipe;
    }

    const WipeService &RuntimeContext::scene_wipe() const {
        return _scene_wipe;
    }

    WipeService &RuntimeContext::system_wipe() {
        return _system_wipe;
    }

    const WipeService &RuntimeContext::system_wipe() const {
        return _system_wipe;
    }

    StarPointerService &RuntimeContext::star_pointer() {
        return _star_pointer;
    }

    const StarPointerService &RuntimeContext::star_pointer() const {
        return _star_pointer;
    }

    bool RuntimeContext::sample_star_pointer_target(const LiveActor &actor, bool check_z) {
        const auto pointing = _star_pointer.is_pointing(actor, _wpad, _scene_camera_pose, check_z);
#ifndef NDEBUG
        emit_star_pointer_target_trace_events();
#endif
        return pointing;
    }

    CameraSystemService &RuntimeContext::camera_system() {
        return _camera_system;
    }

    const CameraSystemService &RuntimeContext::camera_system() const {
        return _camera_system;
    }

    PlayerSystemService &RuntimeContext::player_system() {
        return _player_system;
    }

    const PlayerSystemService &RuntimeContext::player_system() const {
        return _player_system;
    }

    GameLayoutService &RuntimeContext::game_layout() {
        return _game_layout;
    }

    const GameLayoutService &RuntimeContext::game_layout() const {
        return _game_layout;
    }

    RumbleService &RuntimeContext::rumble() {
        return _rumble;
    }

    const RumbleService &RuntimeContext::rumble() const {
        return _rumble;
    }

    SequenceRequestService &RuntimeContext::sequence_requests() {
        return _sequence_requests;
    }

    const SequenceRequestService &RuntimeContext::sequence_requests() const {
        return _sequence_requests;
    }

    SaveDataService &RuntimeContext::save_data() {
        return _save_data;
    }

    const SaveDataService &RuntimeContext::save_data() const {
        return _save_data;
    }

    MessageService &RuntimeContext::messages() {
        return _messages;
    }

    const MessageService &RuntimeContext::messages() const {
        return _messages;
    }

    SceneLightService &RuntimeContext::scene_lights() {
        return _scene_lights;
    }

    const SceneLightService &RuntimeContext::scene_lights() const {
        return _scene_lights;
    }

    RflService &RuntimeContext::rfl() {
        return _rfl;
    }

    const RflService &RuntimeContext::rfl() const {
        return _rfl;
    }

    CaptureScreenDirector &RuntimeContext::capture_screen_director() {
        return *_capture_screen_director;
    }

    const CaptureScreenDirector &RuntimeContext::capture_screen_director() const {
        return *_capture_screen_director;
    }

    SceneScheduler &RuntimeContext::scheduler() {
        return _scheduler;
    }

    const SceneScheduler &RuntimeContext::scheduler() const {
        return _scheduler;
    }

    SceneLifecycleService &RuntimeContext::scene_lifecycle() {
        return *_scene_lifecycle;
    }

    const SceneLifecycleService &RuntimeContext::scene_lifecycle() const {
        return *_scene_lifecycle;
    }

    void RuntimeContext::start_stage_bgm(std::string_view name) {
        _audio.start_stage_bgm(name);
        _logger.info(logging::Category::APP, logging::Message{"SMG requested stage BGM {}"}, name);
    }

    void RuntimeContext::unlock_stage_bgm() {
        _audio.unlock_stage_bgm();
        _logger.info(logging::Category::APP, logging::Message{"SMG unlocked stage BGM"});
    }

    void RuntimeContext::stop_stage_bgm(s32 fade_frames) {
        _audio.stop_stage_bgm(fade_frames);
        _logger.info(logging::Category::APP, logging::Message{"SMG stopped stage BGM over {} frames"}, fade_frames);
    }

    void RuntimeContext::set_stage_bgm_state(s32 state, u32 change_frames) {
        _audio.set_stage_bgm_state(state, change_frames);
        _logger.info(logging::Category::APP, logging::Message{"SMG set stage BGM state {} over {} frames"}, state, change_frames);
    }

    void RuntimeContext::start_system_sound(std::string_view name) {
        _audio.start_system_sound(name);
        _logger.info(logging::Category::APP, logging::Message{"SMG requested system sound {}"}, name);
    }

    void RuntimeContext::start_cs_sound(std::string_view name) {
        _audio.start_controller_speaker_sound(name);
        _logger.info(logging::Category::APP, logging::Message{"SMG requested controller speaker sound {}"}, name);
    }

    void RuntimeContext::register_effect_keeper(EffectKeeperHostKind host_kind, std::string_view host_name, s32 requested_capacity,
                                                std::string_view resource_group_name, bool sort_enabled) {
        _effects.register_keeper(host_kind, host_name, requested_capacity, resource_group_name, sort_enabled);
        _logger.info(logging::Category::APP, logging::Message{"Registered effect keeper {} group {} capacity {}"}, host_name,
                     resource_group_name, requested_capacity);
    }

    void RuntimeContext::unregister_effect_keeper(std::string_view host_name) {
        _effects.unregister_keeper(host_name);
    }

    void RuntimeContext::emit_effect(std::string_view actor_name, std::string_view effect_name) {
        _effects.emit(actor_name, effect_name);
        _logger.info(logging::Category::APP, logging::Message{"{} emitted effect {}"}, actor_name, effect_name);
    }

    void RuntimeContext::delete_effect(std::string_view actor_name, std::string_view effect_name) {
        _effects.delete_effect(actor_name, effect_name);
        _logger.info(logging::Category::APP, logging::Message{"{} deleted effect {}"}, actor_name, effect_name);
    }

    void RuntimeContext::delete_effect_all(std::string_view actor_name) {
        _effects.delete_all(actor_name);
        _logger.info(logging::Category::APP, logging::Message{"{} deleted all effects"}, actor_name);
    }

    void RuntimeContext::note_layout_archive(std::string_view layout_name, const std::filesystem::path &path) {
        _logger.info(logging::Category::APP, logging::Message{"Resolved original layout archive {} -> {}"}, layout_name, path.string());
    }

    void RuntimeContext::note_missing_layout_archive(std::string_view layout_name) {
        _logger.warning(logging::Category::APP, logging::Message{"Missing original layout archive for {}"}, layout_name);
    }

    void RuntimeContext::note_layout_texture_decode_failed(std::string_view layout_name, std::string_view texture_name, std::string_view reason) {
        _logger.warning(logging::Category::APP, logging::Message{"Skipped unsupported layout texture {} from {}: {}"}, texture_name, layout_name,
                        reason);
    }

    void RuntimeContext::note_object_archive(std::string_view object_name, const std::filesystem::path &path) {
        _logger.info(logging::Category::APP, logging::Message{"Resolved original object archive {} -> {}"}, object_name, path.string());
    }

    void RuntimeContext::note_missing_object_archive(std::string_view object_name) {
        _logger.warning(logging::Category::APP, logging::Message{"Missing original object archive for {}"}, object_name);
    }

    void RuntimeContext::note_object_texture_decode_failed(std::string_view object_name, std::string_view reason) {
        _logger.warning(logging::Category::APP, logging::Message{"Skipped object texture data from {}: {}"}, object_name, reason);
    }

#ifndef NDEBUG
    void RuntimeContext::note_debug_event(std::string_view message) {
        _logger.info(logging::Category::APP, logging::Message{"SMG debug: {}"}, message);
    }

    void RuntimeContext::emit_semantic_trace_event(std::string_view category, std::string_view name, std::string_view detail) {
        _semantic_trace_events.push_back(SemanticTraceEvent{
            .index = _next_semantic_trace_event_index++,
            .frame_index = _frame_index,
            .category = std::string(category),
            .name = std::string(name),
            .detail = std::string(detail),
            .stage_name = _current_stage_name,
        });
        if (detail.empty()) {
            _logger.info(logging::Category::APP, logging::Message{"SMG semantic event {}:{}"}, category, name);
        } else {
            _logger.info(logging::Category::APP, logging::Message{"SMG semantic event {}:{} ({})"}, category, name, detail);
        }
    }

    void RuntimeContext::emit_sequence_state_trace_event(std::string_view name, std::string_view detail, std::string_view draw_phase) {
        auto full_detail = std::ostringstream();
        full_detail << "current_scene=" << _current_sequence_scene_name << ";next_scene=" << _next_sequence_scene_name
                    << ";current_stage=" << _current_stage_name << ";scene_wipe=" << wipe_state_name(_scene_wipe.state())
                    << ";system_wipe=" << wipe_state_name(_system_wipe.state()) << ";draw_phase=" << draw_phase
                    << ";frame=" << _frame_index;
        if (!detail.empty()) {
            full_detail << ';' << detail;
        }

        emit_semantic_trace_event("sequence_state", name, full_detail.str());
    }

    void RuntimeContext::emit_star_pointer_target_trace_events() {
        const auto events = _star_pointer.target_events();
        while (_next_star_pointer_target_trace_event_index < events.size()) {
            const auto &event = events[_next_star_pointer_target_trace_event_index++];
            auto detail = std::ostringstream();
            detail << "actor=" << event.actor_name << ";channel=" << event.channel << ";pointer_x=" << event.pointer_x
                   << ";pointer_y=" << event.pointer_y << ";target_x=" << event.target_x << ";target_y=" << event.target_y
                   << ";radius=" << event.projected_radius << ";check_z=" << (event.check_z ? "true" : "false");
            emit_semantic_trace_event("star_pointer", star_pointer_target_event_name(event.kind), detail.str());
        }
    }

    void RuntimeContext::record_j3d_packet_trace(std::string_view model_name, std::uint64_t frame_index, std::string_view draw_pass,
                                                 const J3dRendererPacketState &packet) {
        _j3d_packet_trace.push_back(J3dRuntimePacketTrace{
            .model_name = std::string(model_name),
            .frame_index = frame_index,
            .draw_pass = std::string(draw_pass),
            .state = packet,
        });
    }

    void RuntimeContext::record_layout_packet_trace(LayoutRuntimePacketTrace packet) {
        if (packet.frame_index == 0U) {
            packet.frame_index = _frame_index;
        }
        _layout_packet_trace.push_back(std::move(packet));
    }
#endif

    void RuntimeContext::register_layout(SimpleLayout &layout) {
        _scheduler.register_layout(layout, MR::MovementType_Layout, -1, MR::DrawType_Layout);
    }

    void RuntimeContext::unregister_layout(SimpleLayout &layout) {
        _scheduler.unregister_layout(layout);
    }

    void RuntimeContext::register_layout_actor(LayoutActor &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type) {
        _scheduler.register_layout_actor(layout, movement_type, calc_anim_type, draw_type);
    }

    void RuntimeContext::unregister_layout_actor(LayoutActor &layout) {
        _scheduler.unregister_layout_actor(layout);
    }

    void RuntimeContext::register_live_actor_model(LiveActor &actor, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type) {
        _scheduler.register_live_actor_model(actor, movement_type, calc_anim_type, draw_buffer_type, draw_type);
    }

    void RuntimeContext::unregister_live_actor_model(LiveActor &actor) {
        _scheduler.unregister_live_actor_model(actor);
    }

    void RuntimeContext::register_sky_actor(LiveActor &actor) {
        register_live_actor_model(actor, MR::MovementType_Sky, MR::CalcAnimType_MapObj, MR::DrawBufferType_Sky, -1);
    }

    void RuntimeContext::unregister_sky_actor(LiveActor &actor) {
        unregister_live_actor_model(actor);
    }

    std::filesystem::path RuntimeContext::resolve_disc_files_root() const {
        if (const auto explicit_root = read_path_environment("SMGPC_DISC_FILES_ROOT")) {
            return *explicit_root;
        }

        const auto cwd = std::filesystem::current_path();
        auto candidates = std::vector<std::filesystem::path>{};
        append_disc_root_candidates_from_anchor(candidates, cwd);
        if (const auto exe_directory = executable_directory()) {
            append_disc_root_candidates_from_anchor(candidates, *exe_directory);
        }

        for (const auto &candidate : candidates) {
            std::error_code error{};
            auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        return cwd / "orig" / "RMGK01" / "files";
    }

}  // namespace smgpc::game

OSTime OSGetTime() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}

s64 OSTicksToSeconds(OSTime ticks) {
    return ticks;
}

void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime *pTime) {
    if (pTime == nullptr) {
        return;
    }

    const auto time = std::chrono::sys_seconds(std::chrono::seconds(ticks));
    const auto days = std::chrono::floor<std::chrono::days>(time);
    const auto date = std::chrono::year_month_day(days);
    const auto year_start = std::chrono::sys_days(std::chrono::year(static_cast<int>(date.year())) / std::chrono::January / 1);
    const auto day_time = std::chrono::hh_mm_ss(time - days);

    pTime->sec = static_cast<s32>(day_time.seconds().count());
    pTime->min = static_cast<s32>(day_time.minutes().count());
    pTime->hour = static_cast<s32>(day_time.hours().count());
    pTime->mday = static_cast<s32>(static_cast<unsigned>(date.day()));
    pTime->mon = static_cast<s32>(static_cast<unsigned>(date.month())) - 1;
    pTime->year = static_cast<s32>(static_cast<int>(date.year()));
    pTime->wday = static_cast<s32>(std::chrono::weekday(days).c_encoding());
    pTime->yday = static_cast<s32>((days - year_start).count());
    pTime->msec = 0;
    pTime->usec = 0;
}

s32 KPADRead(s32 channel, KPADStatus sampling_bufs[], u32 length) {
    if (sampling_bufs == nullptr || length == 0U) {
        return 0;
    }

    sampling_bufs[0] = KPADStatus{};
    auto *runtime = smgpc::game::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        return 0;
    }

    const auto *state = runtime->wpad().channel_state(channel);
    if (state == nullptr || !state->connected) {
        return 0;
    }

    sampling_bufs[0].hold = state->hold | (state->repeat != 0U ? KPAD_BUTTON_RPT : 0U);
    sampling_bufs[0].trig = state->trigger;
    sampling_bufs[0].release = state->release;
    sampling_bufs[0].acc = KPADVec3{
        .x = state->core_acceleration.x,
        .y = state->core_acceleration.y,
        .z = state->core_acceleration.z,
    };
    sampling_bufs[0].pos = KPADVec2{
        .x = state->pointer.x,
        .y = state->pointer.y,
    };
    sampling_bufs[0].dist = state->distance_to_display;
    sampling_bufs[0].wpad_err = WPAD_ERR_NONE;
    sampling_bufs[0].dpd_valid_fg = state->pointer.valid ? 1 : 0;
    return 1;
}

s32 DVDConvertPathToEntrynum(const char *path) {
    if (path == nullptr) {
        return -1;
    }

    auto *runtime = smgpc::game::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        return -1;
    }

    try {
        if (!runtime->dvd().exists(path)) {
            return -1;
        }

        auto hash = std::uint32_t{2166136261U};
        for (const auto byte : runtime->dvd().resolve(path).generic_string()) {
            hash ^= static_cast<std::uint8_t>(byte);
            hash *= 16777619U;
        }
        return static_cast<s32>(hash & 0x7fffffffU);
    } catch (const std::exception &) {
        return -1;
    }
}

BOOL DVDOpen(const char *path, DVDFileInfo *file_info) {
    if (path == nullptr || file_info == nullptr) {
        return 0;
    }

    auto *runtime = smgpc::game::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        return 0;
    }

    auto resolved = std::filesystem::path();
    try {
        if (!runtime->dvd().exists(path)) {
            return 0;
        }
        resolved = runtime->dvd().resolve(path);
    } catch (const std::exception &) {
        return 0;
    }

    std::error_code error{};
    const auto size = std::filesystem::file_size(resolved, error);
    if (error || size > static_cast<std::uintmax_t>(UINT32_MAX)) {
        return 0;
    }
    if (file_info->internal != nullptr) {
        (void)DVDClose(file_info);
    }

    file_info->entry_num = DVDConvertPathToEntrynum(path);
    file_info->length = static_cast<u32>(size);
    file_info->position = 0U;
    file_info->internal = new std::filesystem::path(resolved);
    return 1;
}

BOOL DVDClose(DVDFileInfo *file_info) {
    if (file_info == nullptr) {
        return 0;
    }

    delete static_cast<std::filesystem::path *>(file_info->internal);
    file_info->entry_num = -1;
    file_info->length = 0U;
    file_info->position = 0U;
    file_info->internal = nullptr;
    return 1;
}

u32 DVDGetLength(const DVDFileInfo *file_info) {
    return file_info == nullptr ? 0U : file_info->length;
}

s32 DVDReadPrio(DVDFileInfo *file_info, void *destination, s32 length, s32 offset, s32) {
    if (file_info == nullptr || file_info->internal == nullptr || destination == nullptr || length < 0 || offset < 0) {
        return -1;
    }

    const auto &path = *static_cast<const std::filesystem::path *>(file_info->internal);
    auto file = std::ifstream(path, std::ios::binary);
    if (!file) {
        return -1;
    }

    file.seekg(offset, std::ios::beg);
    file.read(static_cast<char *>(destination), length);
    const auto read = file.gcount();
    if (read < 0) {
        return -1;
    }

    file_info->position = static_cast<u32>(offset + read);
    return static_cast<s32>(read);
}
