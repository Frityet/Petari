#include "Game/System/FileLoader.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/guest_thread.hpp>
#include "Game/AudioLib/AudBgm.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include <aurora/exception.hpp>
#include "RuntimeContext.hpp"
#include "compat/DisabledObjectAudioService.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "runtime/ConsoleNandImport.hpp"
#include <aurora/system_config.hpp>
#include "Game/System/RenderMode.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include <SDL3/SDL_mouse.h>

#include <JSystem/JUtility/JUTVideo.hpp>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/CaptureScreenDirector.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "layout/LayoutRuntime.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "camera/CameraParam.hpp"
#include "camera/CameraDirectorRuntime.hpp"

namespace smgpc::runtime {
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

#ifndef NDEBUG
        [[nodiscard]] std::string debug_wpad_button_mask_detail(std::uint32_t mask) {
            constexpr auto buttons = std::array{
                std::pair{WPAD_BUTTON_A, "A"},
                std::pair{WPAD_BUTTON_B, "B"},
                std::pair{WPAD_BUTTON_UP, "UP"},
                std::pair{WPAD_BUTTON_DOWN, "DOWN"},
                std::pair{WPAD_BUTTON_LEFT, "LEFT"},
                std::pair{WPAD_BUTTON_RIGHT, "RIGHT"},
                std::pair{WPAD_BUTTON_PLUS, "PLUS"},
                std::pair{WPAD_BUTTON_MINUS, "MINUS"},
                std::pair{WPAD_BUTTON_HOME, "HOME"},
                std::pair{WPAD_BUTTON_C, "C"},
                std::pair{WPAD_BUTTON_Z, "Z"},
                std::pair{WPAD_BUTTON_1, "ONE"},
                std::pair{WPAD_BUTTON_2, "TWO"},
            };

            auto detail = std::string{"channel=0;buttons="};
            auto appended = false;
            for (const auto &[button_mask, name] : buttons) {
                if ((mask & button_mask) == 0U) {
                    continue;
                }
                if (appended) {
                    detail += '+';
                }
                detail += name;
                appended = true;
            }
            if (!appended) {
                detail += "none";
            }
            return detail;
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
            return read_string_environment("SMGPC_STAGE_NAME").value_or("");
#else
            return "";
#endif
        }

        [[nodiscard]] smgpc::camera::CameraParamVec3 operator-(const smgpc::camera::CameraParamVec3 &lhs,
                                                               const smgpc::camera::CameraParamVec3 &rhs) {
            return {.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z};
        }

        [[nodiscard]] smgpc::camera::CameraParamVec3 operator+(const smgpc::camera::CameraParamVec3 &lhs,
                                                               const smgpc::camera::CameraParamVec3 &rhs) {
            return {.x = lhs.x + rhs.x, .y = lhs.y + rhs.y, .z = lhs.z + rhs.z};
        }

        [[nodiscard]] smgpc::camera::CameraParamVec3 operator*(const smgpc::camera::CameraParamVec3 &lhs, float rhs) {
            return {.x = lhs.x * rhs, .y = lhs.y * rhs, .z = lhs.z * rhs};
        }

        [[nodiscard]] smgpc::camera::CameraParamVec3 operator*(float lhs, const smgpc::camera::CameraParamVec3 &rhs) {
            return rhs * lhs;
        }

        [[nodiscard]] smgpc::camera::CameraParamVec3 camera_vec_cross(const smgpc::camera::CameraParamVec3 &lhs,
                                                                      const smgpc::camera::CameraParamVec3 &rhs) {
            return {
                .x = lhs.y * rhs.z - lhs.z * rhs.y,
                .y = lhs.z * rhs.x - lhs.x * rhs.z,
                .z = lhs.x * rhs.y - lhs.y * rhs.x,
            };
        }

        [[nodiscard]] float camera_vec_length(const smgpc::camera::CameraParamVec3 &value) {
            return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
        }

        [[nodiscard]] smgpc::camera::CameraParamVec3 camera_vec_normalized(const smgpc::camera::CameraParamVec3 &value) {
            const auto length = camera_vec_length(value);
            if (length <= 0.000001F) {
                aurora::throw_host_exception<std::logic_error>("free-camera basis is degenerate");
            }
            return value * (1.0F / length);
        }

    }  // namespace

    struct RuntimeContext::Registration {
        explicit Registration(RuntimeContext& runtime) : owner(&runtime) {
            if (s_runtime_context != nullptr) {
                aurora::throw_host_exception<std::logic_error>("Only one SMG runtime context may be active.");
            }
            s_runtime_context = owner;
        }

        ~Registration() {
            // Every RuntimeContext member, including the callback scheduler,
            // has retired before this first-declared member is destroyed.
            aurora::wpad_service().clear();
            if (s_runtime_context == owner) s_runtime_context = nullptr;
        }

        RuntimeContext* owner;
    };

    RuntimeContext::RuntimeContext(logging::ILogger &logger, render::AuroraWindow &window_service,
                                   resource::GameResourceRuntime &resources)
        : RuntimeContext(logger, window_service, resources, nullptr) {
    }

    RuntimeContext::RuntimeContext(
        logging::ILogger &logger, render::AuroraWindow &window_service,
        resource::GameResourceRuntime &resources,
        std::unique_ptr<JAudioPlaybackService> audio_playback)
        : _logger(logger), _window_service(window_service), _disc_files_root(resolve_disc_files_root()), _dvd(_disc_files_root),
          _host_heaps(resources.host_heaps()),
          _j_audio_playback(audio_playback != nullptr
                                ? std::move(audio_playback)
                                : std::make_unique<JAudioPlaybackService>(_dvd)),
          _disabled_object_audio(aurora::audio::make_disabled_object_audio_service(resources.host_heaps(), _j_audio_playback.get())), _rfl(_save_data.nand()),
          _current_stage_name(default_stage_name())
#ifndef NDEBUG
          ,
          _debug_wpad_input_script(DebugWpadInputScript::from_environment())
#endif
    {
        _registration = std::make_unique<Registration>(*this);
        try {
            if (const auto save_directory = read_path_environment("SMGPC_SAVE_DIR")) {
                _save_data.set_host_directory(*save_directory);
                _logger.info(logging::Category::APP, logging::Message{"Using SMG save files from {}"}, save_directory->string());
            }
            if (const auto nand_directory = read_path_environment("SMGPC_NAND_DIR")) {
                const auto imported = import_console_nand_directory(_save_data.nand(), *nand_directory, NandImportExisting::Preserve);
                _logger.info(logging::Category::APP, logging::Message{"Loaded {} console NAND files from {} ({} existing files preserved)"},
                             imported.imported_files, nand_directory->string(), imported.preserved_files);
            }
            _system_config = std::make_unique<aurora::SystemConfiguration>(_save_data.nand());
            _save_data.activate_nand();
            aurora::wpad_service().clear();
            _display = std::make_unique<OriginalDisplayLifetime>(_window_service, _host_heaps, *MR::getSuitableRenderMode());
            _capture_screen_director = std::make_unique<CaptureScreenDirector>();
            _logger.info(logging::Category::APP, logging::Message{"Using SMG disc image through Aurora DVD"});
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

#ifndef NDEBUG
            if (_debug_wpad_input_script.button_span_count() != 0) {
                _logger.info(logging::Category::APP, logging::Message{"Loaded {} debug WPAD button script spans"},
                             _debug_wpad_input_script.button_span_count());
            }
            if (_debug_wpad_input_script.pointer_span_count() != 0) {
                _logger.info(logging::Category::APP, logging::Message{"Loaded {} debug WPAD pointer script spans"},
                             _debug_wpad_input_script.pointer_span_count());
            }
            if (_debug_wpad_input_script.stick_span_count() != 0) {
                _logger.info(logging::Category::APP, logging::Message{"Loaded {} debug WPAD stick script spans"},
                             _debug_wpad_input_script.stick_span_count());
            }
            emit_semantic_trace_event("runtime", "runtime_context_created", "disc=aurora-dvd");
#endif
        } catch (...) {
            // These owners call back into the still-live scheduler and trace
            // services during destruction; retire them before member unwind.
            retire_owned_runtime_objects();
            throw;
        }
    }

    RuntimeContext::~RuntimeContext() {
        retire_owned_runtime_objects();
    }

    void RuntimeContext::retire_owned_runtime_objects() {
        const aurora::os::GuestThreadExecutionScope execution;
#ifndef NDEBUG
        _is_destroying = true;
#endif
        if (_freecam_mouse_relative_mode) {
            const auto native_handle = _window_service.native_handle();
            if (native_handle.window_handle != nullptr) {
                SDL_SetWindowRelativeMouseMode(static_cast<SDL_Window *>(native_handle.window_handle), false);
            }
        }
        _j_audio_playback->reset_scene();
        smgpc::compat::retire_audio_facade_state();
        _display.reset();
        _scheduler.clear();
        _capture_screen_director.reset();
        _disabled_object_audio.reset();
    }

    RuntimeContext &RuntimeContext::instance() {
        if (s_runtime_context == nullptr) {
            aurora::throw_host_exception<std::logic_error>("SMG runtime context is not active.");
        }

        return *s_runtime_context;
    }

    RuntimeContext *RuntimeContext::try_instance() {
        return s_runtime_context;
    }

    void RuntimeContext::begin_frame(const render::FrameContext &frame_context) {
        const aurora::os::GuestThreadExecutionScope execution;
        _frame_index = frame_context.frame_index;
        _dvd.begin_frame(_frame_index);
        _ios.begin_frame(_frame_index);
        _wii_platform.begin_frame(_frame_index);
        _copy_events.clear();
#ifndef NDEBUG
        _layout_packet_trace.clear();
#endif
        _j3d_pixel_update_state.reset();
        _scene_camera_pose.reset();
        if (const auto* camera = smgpc::camera::current_camera_director_runtime()) {
            _scene_camera_pose = camera->pose();
        }
        const auto debug_toggle_freecam = _window_service.is_debug_input_pressed(render::DebugInput::CORE_PAD_TOGGLE_FREECAM);
        if (debug_toggle_freecam && !_freecam_toggle_held_last_frame) {
            _freecam_enabled = !_freecam_enabled;
            if (_freecam_enabled && !_freecam_target_pose.has_value()) {
                _freecam_target_pose = _scene_camera_pose;
                if (!_freecam_target_pose.has_value()) {
                    _freecam_enabled = false;
                }
            }
            _freecam_look_initialized = false;
        }
        _freecam_toggle_held_last_frame = debug_toggle_freecam;
        if (_freecam_mouse_relative_mode != _freecam_enabled) {
            _freecam_mouse_relative_mode = _freecam_enabled;
            const auto native_handle = _window_service.native_handle();
            if (native_handle.window_handle != nullptr) {
                if (!SDL_SetWindowRelativeMouseMode(static_cast<SDL_Window *>(native_handle.window_handle), _freecam_mouse_relative_mode)) {
                    _logger.warning(logging::Category::APP,
                                   logging::Message{"Failed to set relative mouse mode {} for freecam"},
                                   _freecam_mouse_relative_mode ? "on" : "off");
                }
            }
        }

        if (_freecam_enabled && !_freecam_target_pose.has_value()) {
            _freecam_target_pose = _scene_camera_pose;
            if (!_freecam_target_pose.has_value()) {
                _freecam_enabled = false;
            }
        }
        if (!_freecam_enabled) {
            _freecam_target_pose.reset();
            _freecam_look_initialized = false;
        }
        _audio.begin_frame(_frame_index);
        _j_audio_playback->begin_frame(_frame_index);
        _scene_wipe.begin_frame(_frame_index);
        _system_wipe.begin_frame(_frame_index);
        _star_pointer.begin_frame(_frame_index);
        _rumble.begin_frame(_frame_index);
        _sequence_requests.begin_frame(_frame_index);
        _rfl.begin_frame(_frame_index);
        aurora::wpad_service().begin_frame();

        auto raw_hold_mask = std::uint32_t{};
        const auto append_input_button = [this, &raw_hold_mask](render::InputButton button, std::uint32_t mask) {
            if (_window_service.is_input_pressed(button)) {
                raw_hold_mask |= mask;
            }
        };
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
        auto core_gesture_pressed = _window_service.is_input_pressed(render::InputButton::CORE_PAD_SWING);
        auto hold_mask = raw_hold_mask;
        auto pointer = _window_service.input_pointer_state();
        const auto raw_pointer = pointer;
        auto sub_stick_x = 0.0F;
        auto sub_stick_y = 0.0F;

        if (_window_service.is_input_pressed(render::InputButton::SUB_STICK_LEFT)) {
            sub_stick_x -= 1.0F;
        }
        if (_window_service.is_input_pressed(render::InputButton::SUB_STICK_RIGHT)) {
            sub_stick_x += 1.0F;
        }
        if (_window_service.is_input_pressed(render::InputButton::SUB_STICK_UP)) {
            sub_stick_y += 1.0F;
        }
        if (_window_service.is_input_pressed(render::InputButton::SUB_STICK_DOWN)) {
            sub_stick_y -= 1.0F;
        }
        if (sub_stick_x != 0.0F && sub_stick_y != 0.0F) {
            constexpr auto cDiagonalStickScale = 0.70710678118F;
            sub_stick_x *= cDiagonalStickScale;
            sub_stick_y *= cDiagonalStickScale;
        }

#ifndef NDEBUG
        const auto raw_stick_x = sub_stick_x;
        const auto raw_stick_y = sub_stick_y;
        auto debug_script_applied = _debug_wpad_input_script.apply(_frame_index, hold_mask, pointer, sub_stick_x, sub_stick_y);
        const auto file_applied = _debug_wpad_input_file.apply(_frame_index, hold_mask, pointer, sub_stick_x, sub_stick_y);
        debug_script_applied.buttons |= file_applied.buttons;
        debug_script_applied.pointer |= file_applied.pointer;
        debug_script_applied.stick |= file_applied.stick;
        const auto debug_button_script_applied = debug_script_applied.buttons;
        const auto debug_pointer_script_applied = debug_script_applied.pointer;
#endif

        if (_freecam_enabled && _freecam_target_pose.has_value()) {
            auto freecam_pose = *_freecam_target_pose;
            constexpr auto cFreecamMoveSpeed = 60.0F;
            constexpr auto cFreecamMaxPitchRadians = 1.553343F;
            constexpr auto cFreecamMinPitchRadians = -1.553343F;
            constexpr auto cFreecamMouseYawSpeed = 0.0025F;
            constexpr auto cFreecamMousePitchSpeed = 0.0025F;

            const auto freecam_world_up = smgpc::camera::CameraParamVec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
            const auto distance = camera_vec_length(freecam_pose.watch - freecam_pose.eye);
            auto freecam_forward = camera_vec_normalized(freecam_pose.watch - freecam_pose.eye);

            if (!_freecam_look_initialized) {
                _freecam_yaw_radians = std::atan2(freecam_forward.x, -freecam_forward.z);
                _freecam_pitch_radians = std::asin(std::clamp(freecam_forward.y, -1.0F, 1.0F));
                _freecam_look_initialized = true;
            }

            const auto move_forward =
                _window_service.is_debug_input_pressed(render::DebugInput::CORE_PAD_FREECAM_MOVE_FORWARD) ? 1.0F : 0.0F;
            const auto move_backward =
                _window_service.is_debug_input_pressed(render::DebugInput::CORE_PAD_FREECAM_MOVE_BACKWARD) ? 1.0F : 0.0F;
            const auto move_left =
                _window_service.is_debug_input_pressed(render::DebugInput::CORE_PAD_FREECAM_MOVE_LEFT) ? 1.0F : 0.0F;
            const auto move_right =
                _window_service.is_debug_input_pressed(render::DebugInput::CORE_PAD_FREECAM_MOVE_RIGHT) ? 1.0F : 0.0F;
            const auto move_up =
                _window_service.is_debug_input_pressed(render::DebugInput::CORE_PAD_FREECAM_MOVE_UP) ? 1.0F : 0.0F;
            const auto move_down =
                _window_service.is_debug_input_pressed(render::DebugInput::CORE_PAD_FREECAM_MOVE_DOWN) ? 1.0F : 0.0F;

            auto freecam_mouse_delta_x = 0.0F;
            auto freecam_mouse_delta_y = 0.0F;
            SDL_GetRelativeMouseState(&freecam_mouse_delta_x, &freecam_mouse_delta_y);
            _freecam_yaw_radians += freecam_mouse_delta_x * cFreecamMouseYawSpeed;
            _freecam_pitch_radians += -freecam_mouse_delta_y * cFreecamMousePitchSpeed;
            _freecam_pitch_radians = std::clamp(_freecam_pitch_radians, cFreecamMinPitchRadians, cFreecamMaxPitchRadians);

            freecam_forward = {
                .x = std::sin(_freecam_yaw_radians) * std::cos(_freecam_pitch_radians),
                .y = std::sin(_freecam_pitch_radians),
                .z = -std::cos(_freecam_pitch_radians) * std::cos(_freecam_yaw_radians),
            };
            const auto right = camera_vec_normalized(camera_vec_cross(freecam_forward, freecam_world_up));

            const auto forward_delta = move_forward - move_backward;
            if (forward_delta != 0.0F) {
                freecam_pose.eye = freecam_pose.eye + freecam_forward * (forward_delta * cFreecamMoveSpeed);
            }
            const auto right_delta = move_right - move_left;
            if (right_delta != 0.0F) {
                freecam_pose.eye = freecam_pose.eye + right * (right_delta * cFreecamMoveSpeed);
            }
            const auto up_delta = move_up - move_down;
            if (up_delta != 0.0F) {
                freecam_pose.eye = freecam_pose.eye + freecam_world_up * (up_delta * cFreecamMoveSpeed);
            }

            freecam_pose.watch = freecam_pose.eye + freecam_forward * distance;
            freecam_pose.up = freecam_world_up;
            _freecam_target_pose = freecam_pose;
            _scene_camera_pose = freecam_pose;
            hold_mask = 0U;
            pointer = render::InputPointerState{};
            sub_stick_x = 0.0F;
            sub_stick_y = 0.0F;
            core_gesture_pressed = false;
        }
#ifndef NDEBUG
        _host_input_trace = HostInputTraceState{
            .frame_index = _frame_index,
            .raw_hold_mask = raw_hold_mask,
            .effective_hold_mask = hold_mask,
            .raw_pointer = raw_pointer,
            .effective_pointer = pointer,
            .raw_stick_x = raw_stick_x,
            .raw_stick_y = raw_stick_y,
            .effective_stick_x = sub_stick_x,
            .effective_stick_y = sub_stick_y,
            .debug_button_script_applied = debug_button_script_applied,
            .debug_pointer_script_applied = debug_pointer_script_applied,
            .debug_stick_script_applied = debug_script_applied.stick,
        };
#endif
        auto &wpad = aurora::wpad_service();
        wpad.set_device_type(WPAD_CHAN0, aurora::WpadDeviceType::Freestyle);
        wpad.set_connected(WPAD_CHAN0, true);
        wpad.set_button_mask(WPAD_CHAN0, hold_mask);
        wpad.set_pointer_resolution(WPAD_CHAN0, static_cast<f32>(MR::getFrameBufferWidth()), static_cast<f32>(MR::getFrameBufferHeight()));
        wpad.set_pointer(WPAD_CHAN0, pointer.x, pointer.y, pointer.valid, 1.0F, 0.0F);
        wpad.set_sub_stick(WPAD_CHAN0, sub_stick_x, sub_stick_y);
        const auto core_acceleration = _core_pad_gesture.sample(core_gesture_pressed);
        const auto sub_acceleration = _sub_pad_gesture.sample(false);
        wpad.set_core_acceleration(WPAD_CHAN0, core_acceleration.x, core_acceleration.y, core_acceleration.z);
        wpad.set_sub_acceleration(WPAD_CHAN0, sub_acceleration.x, sub_acceleration.y, sub_acceleration.z);
        wpad.set_distance_to_display(WPAD_CHAN0, pointer.valid ? 1.0F : 0.0F);
        // GameSystemObjHolder updates WPad, then original pointer controllers,
        // before the scene's camera and actor movement.
        refresh_scene_camera_pose();
#ifndef NDEBUG
        if (!_emitted_wpad_buttons_held_event && hold_mask != 0U) {
            _emitted_wpad_buttons_held_event = true;
            emit_semantic_trace_event("input", "wpad_buttons_held", debug_wpad_button_mask_detail(hold_mask));
        }
        if (!_emitted_wpad_sub_stick_event && (sub_stick_x != 0.0F || sub_stick_y != 0.0F)) {
            _emitted_wpad_sub_stick_event = true;
            emit_semantic_trace_event("input", "wpad_sub_stick",
                                      "channel=0;x=" + std::to_string(sub_stick_x) + ";y=" + std::to_string(sub_stick_y));
        }
#endif

        {
            const smgpc::compat::JkrAllocationScope game(_scheduler.allocation_domain());
            const J3DSys::ContextScope commands;
            _scheduler.begin_frame();
            SceneFunction::movementStopSceneController();
            SceneFunction::executeMovementList();
        }
        {
            const smgpc::compat::JkrAllocationScope game(_scheduler.allocation_domain());
            const J3DSys::ContextScope commands;
            SceneFunction::executeCalcAnimList();
            CategoryList::execute(MR::CalcAnimType_AnimParticleIgnorePause);
            SceneFunction::executeCalcViewAndEntryList();
        }
        _j_audio_playback->end_frame();
        smgpc::compat::advance_audio_facade_state();
    }

    void RuntimeContext::set_scene_camera_pose(const smgpc::camera::CameraPose &camera_pose) {
        if (_freecam_enabled) {
            return;
        }
        _scene_camera_pose = camera_pose;
    }

    void RuntimeContext::refresh_scene_camera_pose() {
        if (!_freecam_enabled) {
            if (const auto* camera = smgpc::camera::current_camera_director_runtime()) {
                _scene_camera_pose = camera->pose();
            }
        }
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

#ifndef NDEBUG
    void RuntimeContext::set_render_packet_trace_frame(std::optional<std::uint64_t> frame_index) {
        _render_packet_trace_frame = frame_index;
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

    void RuntimeContext::request_application_exit(std::string_view reason) {
        _application_exit_requested = true;
        _application_exit_reason = std::string(reason);
        _logger.info(logging::Category::APP, logging::Message{"Application exit requested: {}"}, _application_exit_reason);
    }

    bool RuntimeContext::is_core_pad_button_a(s32 channel) const {
        return aurora::wpad_service().is_button_held(channel, WPAD_BUTTON_A);
    }

    bool RuntimeContext::is_core_pad_button_b(s32 channel) const {
        return aurora::wpad_service().is_button_held(channel, WPAD_BUTTON_B);
    }

    bool RuntimeContext::should_exit_application() const {
        return _application_exit_requested;
    }

    std::string_view RuntimeContext::application_exit_reason() const {
        return _application_exit_reason;
    }

    std::uint64_t RuntimeContext::frame_index() const {
        return _frame_index;
    }

    bool RuntimeContext::is_freecam_enabled() const {
        return _freecam_enabled;
    }

    void RuntimeContext::set_freecam_enabled(bool enabled) {
        if (_freecam_enabled == enabled) {
            return;
        }

        _freecam_enabled = enabled;
        _freecam_look_initialized = false;
        if (!enabled) {
            _freecam_target_pose.reset();
        } else if (!_freecam_target_pose.has_value()) {
            _freecam_target_pose = _scene_camera_pose;
        }
    }

    const std::optional<smgpc::camera::CameraPose> &RuntimeContext::scene_camera_pose() const {
        return _scene_camera_pose;
    }

    const std::optional<smgpc::camera::CameraPose> &RuntimeContext::last_camera_pose() const {
        return _last_camera_pose;
    }

    std::span<const render::CopyEvent> RuntimeContext::copy_events() const {
        return _copy_events;
    }

#ifndef NDEBUG
    std::span<const RuntimeContext::LayoutRuntimePacketTrace> RuntimeContext::layout_packet_trace() const {
        return _layout_packet_trace;
    }

    std::span<const RuntimeContext::SemanticTraceEvent> RuntimeContext::semantic_trace_events() const {
        return _semantic_trace_events;
    }

    const RuntimeContext::HostInputTraceState &RuntimeContext::host_input_trace() const {
        return _host_input_trace;
    }

    bool RuntimeContext::should_record_render_packet_trace() const {
        return _render_packet_trace_frame.has_value() && _frame_index == *_render_packet_trace_frame;
    }

    bool RuntimeContext::is_destroying() const {
        return _is_destroying;
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
        return _j_audio_playback->is_bgm_prepared(smgpc::runtime::BgmLane::Stage);
    }

    std::string_view RuntimeContext::current_stage_bgm_name() const {
        return _j_audio_playback->bgm_name(smgpc::runtime::BgmLane::Stage);
    }

    std::optional<std::filesystem::path> RuntimeContext::find_layout_archive(std::string_view layout_name) const {
        return _dvd.find_layout_archive(layout_name);
    }

    std::optional<std::filesystem::path> RuntimeContext::find_object_archive(std::string_view object_name) const {
        return _dvd.find_object_archive(object_name);
    }


    void RuntimeContext::initialize_particle_resources(const resource::GameResourceRuntime &resources) {
        if (!SingletonHolder<FileLoader>::get())
            aurora::throw_host_exception<std::logic_error>("Resources require the original FileLoader");
        compat::JkrHostAllocationScope host;
        if (_particle_resources) {
            aurora::throw_host_exception<std::logic_error>("The process particle resources are already initialized.");
        }
        _particle_resources = std::make_shared<ParticleResourceOwnership>(
            resources.host_heaps(), resources.budget().particle_resource_bytes, *SingletonHolder<FileLoader>::get());
    }

    std::shared_ptr<ParticleResourceOwnership> RuntimeContext::retain_particle_resources() const {
        if (!_particle_resources) {
            aurora::throw_host_exception<std::logic_error>("The process particle resources have not reached resource-ready startup.");
        }
        return _particle_resources;
    }

    void RuntimeContext::initialize_scenario_catalog(const resource::GameResourceRuntime &resources) {
        if (!SingletonHolder<FileLoader>::get())
            aurora::throw_host_exception<std::logic_error>("Resources require the original FileLoader");
        compat::JkrHostAllocationScope host;
        if (_scenario_catalog) {
            aurora::throw_host_exception<std::logic_error>("The process scenario catalog is already initialized.");
        }
        _scenario_catalog = std::make_shared<ScenarioCatalogOwnership>(
            resources.host_heaps(), resources.budget().scenario_catalog_bytes, *SingletonHolder<FileLoader>::get(), _dvd);
    }

    std::shared_ptr<ScenarioCatalogOwnership> RuntimeContext::retain_scenario_catalog() const {
        if (!_scenario_catalog) {
            aurora::throw_host_exception<std::logic_error>("The process scenario catalog has not reached resource-ready startup.");
        }
        return _scenario_catalog;
    }

    DvdFileSystemService &RuntimeContext::dvd() {
        return _dvd;
    }

    const DvdFileSystemService &RuntimeContext::dvd() const {
        return _dvd;
    }

    WiiIosService &RuntimeContext::ios() {
        return _ios;
    }

    const WiiIosService &RuntimeContext::ios() const {
        return _ios;
    }

    WiiPlatformService &RuntimeContext::wii_platform() {
        return _wii_platform;
    }

    const WiiPlatformService &RuntimeContext::wii_platform() const {
        return _wii_platform;
    }

    OriginalDisplayLifetime& RuntimeContext::display() {
        return *_display;
    }

    const OriginalDisplayLifetime& RuntimeContext::display() const {
        return *_display;
    }

    WpadService &RuntimeContext::wpad() {
        return aurora::wpad_service();
    }

    const WpadService &RuntimeContext::wpad() const {
        return aurora::wpad_service();
    }

    AudioEventService &RuntimeContext::audio() {
        return _audio;
    }

    const AudioEventService &RuntimeContext::audio() const {
        return _audio;
    }

    JAudioPlaybackService &RuntimeContext::j_audio_playback() {
        return *_j_audio_playback;
    }

    const JAudioPlaybackService &RuntimeContext::j_audio_playback() const {
        return *_j_audio_playback;
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
        const auto pointing = _star_pointer.is_pointing(actor, aurora::wpad_service(), _scene_camera_pose, check_z);
#ifndef NDEBUG
        emit_star_pointer_target_trace_events();
#endif
        return pointing;
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

    NandFileSystemService &RuntimeContext::nand() {
        return _save_data.nand();
    }

    const NandFileSystemService &RuntimeContext::nand() const {
        return _save_data.nand();
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

    JAISoundHandle *RuntimeContext::start_sub_bgm(std::string_view name, bool prepared) {
        auto *handle = _j_audio_playback->start_bgm(BgmLane::Sub, name, prepared);
        if (handle == nullptr) {
            _audio.stop_sub_bgm(0);
            return nullptr;
        }
        _audio.start_sub_bgm(name, prepared, _j_audio_playback->bgm_id(BgmLane::Sub));
        return handle;
    }

    JAISoundHandle *RuntimeContext::start_sub_bgm(u32 sound_id, bool prepared) {
        auto *handle = _j_audio_playback->start_bgm(BgmLane::Sub, sound_id, prepared);
        if (handle == nullptr) {
            _audio.stop_sub_bgm(0);
            return nullptr;
        }
        _audio.start_sub_bgm({}, prepared, sound_id);
        return handle;
    }

    void RuntimeContext::stop_sub_bgm(u32 fade_frames) {
        _j_audio_playback->stop_bgm(BgmLane::Sub, fade_frames);
        _audio.stop_sub_bgm(fade_frames);
    }

    void RuntimeContext::unlock_sub_bgm() {
        _j_audio_playback->unlock_bgm(BgmLane::Sub);
        _audio.unlock_sub_bgm();
    }

    JAISoundHandle *RuntimeContext::start_stage_bgm(
        std::string_view name, bool prepared) {
        auto *handle = _j_audio_playback->start_bgm(smgpc::runtime::BgmLane::Stage, name, prepared);
        if (handle == nullptr) {
            _audio.resolve_stage_bgm_absent();
            return nullptr;
        }
        const auto sound_id = _j_audio_playback->bgm_id(smgpc::runtime::BgmLane::Stage);
        if (!sound_id.has_value()) {
            aurora::throw_host_exception<std::logic_error>(
                "A concrete stage-BGM voice has no retail sound ID");
        }
        _audio.start_stage_bgm(name, *sound_id);
        _logger.info(logging::Category::APP,
                     logging::Message{"SMG started retail stage BGM {} ({:#010x})"},
                     name, *sound_id);
        return handle;
    }

    JAISoundHandle *RuntimeContext::start_stage_bgm(
        u32 sound_id, bool prepared) {
        auto *handle = _j_audio_playback->start_bgm(smgpc::runtime::BgmLane::Stage, sound_id, prepared);
        if (handle == nullptr) {
            _audio.resolve_stage_bgm_absent();
            return nullptr;
        }
        _audio.start_stage_bgm(sound_id);
        _logger.info(logging::Category::APP,
                     logging::Message{"SMG started retail stage BGM {:#010x}"},
                     sound_id);
        return handle;
    }

    void RuntimeContext::unlock_stage_bgm() {
        _j_audio_playback->unlock_bgm(smgpc::runtime::BgmLane::Stage);
        _audio.unlock_stage_bgm();
        _logger.info(logging::Category::APP, logging::Message{"SMG unlocked stage BGM"});
    }

    void RuntimeContext::stop_stage_bgm(s32 fade_frames) {
        if (fade_frames < 0) {
            aurora::throw_host_exception<std::invalid_argument>(
                "A stage-BGM fade cannot use negative frames");
        }
        _j_audio_playback->stop_bgm(smgpc::runtime::BgmLane::Stage, static_cast<u32>(fade_frames));
        _audio.stop_stage_bgm(fade_frames);
        _logger.info(logging::Category::APP, logging::Message{"SMG stopped stage BGM over {} frames"}, fade_frames);
    }

    void RuntimeContext::set_stage_bgm_state(s32 state, u32 change_frames) {
        if (!_j_audio_playback->has_active_bgm(smgpc::runtime::BgmLane::Stage)) {
            return;
        }
        auto *bgm = AudWrap::getStageBgm();
        if (bgm != nullptr) {
            bgm->changeTrackMuteState(state, static_cast<s32>(change_frames));
        }
    }

    JAISoundHandle *RuntimeContext::start_system_sound(
        std::string_view name, s32 parameter_1, s32 parameter_2) {
        auto *handle = _j_audio_playback->start_sound_effect(
            name, parameter_1, parameter_2);
        if (handle == nullptr) {
            return nullptr;
        }
        _audio.start_system_sound(name);
        _logger.info(logging::Category::APP,
                     logging::Message{"SMG started retail system sound {}"}, name);
        return handle;
    }

    void RuntimeContext::stop_system_sound(std::string_view name, u32 delay_frames) {
        _j_audio_playback->stop_sound_effect(name, delay_frames);
        _audio.stop_system_sound(name, delay_frames);
        _logger.info(logging::Category::APP, logging::Message{"SMG stopped system sound {} after {} frames"}, name, delay_frames);
    }

    JAISoundHandle *RuntimeContext::start_system_level_sound(
        std::string_view name, s32 parameter_1, s32 parameter_2) {
        auto *handle = _j_audio_playback->start_level_sound(
            name, parameter_1, parameter_2);
        if (handle == nullptr) {
            return nullptr;
        }
        _audio.start_system_level_sound(name);
        _logger.info(logging::Category::APP,
                     logging::Message{"SMG started retail system level sound {}"}, name);
        return handle;
    }

    void RuntimeContext::submit_level_sound() {
        _j_audio_playback->set_level_sound_permitted(false);
        _audio.submit_level_sound();
        _logger.info(logging::Category::APP, logging::Message{"SMG submitted level sounds"});
    }

    void RuntimeContext::permit_level_sound() {
        _j_audio_playback->set_level_sound_permitted(true);
        _audio.permit_level_sound();
        _logger.info(logging::Category::APP, logging::Message{"SMG permitted level sounds"});
    }

    JAISoundHandle *RuntimeContext::start_atmosphere_sound(
        std::string_view name, s32 parameter_1, s32 parameter_2) {
        auto *handle = _j_audio_playback->start_sound_effect(
            name, parameter_1, parameter_2);
        if (handle == nullptr) {
            return nullptr;
        }
        _audio.start_atmosphere_sound(name);
        _logger.info(logging::Category::APP,
                     logging::Message{"SMG started retail atmosphere sound {}"}, name);
        return handle;
    }

    JAISoundHandle *RuntimeContext::start_atmosphere_level_sound(
        std::string_view name, s32 parameter_1, s32 parameter_2) {
        return _j_audio_playback->start_level_sound(
            name, parameter_1, parameter_2);
    }

    void RuntimeContext::start_system_me(std::string_view name) {
        aurora::throw_host_exception<std::logic_error>(
            "JAudio ME scheduler is unavailable for " + std::string(name));
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
        // Trace history belongs to the process and outlives the emitting scene.
        const compat::JkrHostAllocationScope host;
        if (_is_destroying) {
            return;
        }

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

    void RuntimeContext::record_layout_packet_trace(LayoutRuntimePacketTrace packet) {
        if (packet.frame_index == 0U) {
            packet.frame_index = _frame_index;
        }
        _layout_packet_trace.push_back(std::move(packet));
    }
#endif

    void RuntimeContext::register_layout(smgpc::layout::LayoutRuntime &layout) {
        _scheduler.register_layout(layout, MR::MovementType_Layout, -1, MR::DrawType_Layout);
    }

    void RuntimeContext::unregister_layout(smgpc::layout::LayoutRuntime &layout) {
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
        return std::filesystem::path("aurora-dvd");
    }

}  // namespace smgpc::runtime
