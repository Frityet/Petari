#include "RuntimeContext.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <vector>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/SimpleLayout.hpp"

namespace smgpc::game {
    namespace {

        RuntimeContext *s_runtime_context = nullptr;

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

    }  // namespace

    RuntimeContext::RuntimeContext(logging::ILogger &logger, render::IWindowService &window_service)
        : _logger(logger), _window_service(window_service), _disc_files_root(resolve_disc_files_root()), _dvd(_disc_files_root),
          _hold_title_combo_frame(read_frame_index_environment("SMGPC_HOLD_TITLE_COMBO_FRAME")) {
        if (s_runtime_context != nullptr) {
            throw std::logic_error("Only one SMG runtime context may be active.");
        }

        s_runtime_context = this;
        _logger.info(logging::Category::APP, logging::Message{"Using SMG disc files from {}"}, _disc_files_root.string());
        if (_hold_title_combo_frame.has_value()) {
            _logger.info(logging::Category::APP, logging::Message{"Debug title A+B hold starts at frame {}"}, *_hold_title_combo_frame);
        }
    }

    RuntimeContext::~RuntimeContext() {
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
        _j3d_packet_trace.clear();
        _audio.begin_frame(_frame_index);
        _effects.begin_frame(_frame_index);
        _wpad.begin_frame();

        auto hold_mask = std::uint32_t{};
        if (_window_service.is_input_pressed(render::InputButton::CORE_PAD_A) ||
            (_hold_title_combo_frame.has_value() && _frame_index >= *_hold_title_combo_frame)) {
            hold_mask |= WPAD_BUTTON_A;
        }
        if (_window_service.is_input_pressed(render::InputButton::CORE_PAD_B) ||
            (_hold_title_combo_frame.has_value() && _frame_index >= *_hold_title_combo_frame)) {
            hold_mask |= WPAD_BUTTON_B;
        }
        _wpad.set_connected(WPAD_CHAN0, true);
        _wpad.set_button_mask(WPAD_CHAN0, hold_mask);

        _scheduler.execute_movement();
        _scheduler.execute_calc_anim();
        _scheduler.execute_calc_view_and_entry();
    }

    void RuntimeContext::draw_3d_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose) {
        _last_camera_pose = camera_pose;
        _scheduler.execute_draw_buffer_list_normal(renderer, camera_pose);
    }

    void RuntimeContext::draw_2d_normal(render::IRendererEngine &renderer) {
        _scheduler.execute_draw_list_2d_normal(renderer);
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

    const std::optional<CameraPoseCompat> &RuntimeContext::last_camera_pose() const {
        return _last_camera_pose;
    }

    std::span<const RuntimeContext::J3dRuntimePacketTrace> RuntimeContext::j3d_packet_trace() const {
        return _j3d_packet_trace;
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

    RflService &RuntimeContext::rfl() {
        return _rfl;
    }

    const RflService &RuntimeContext::rfl() const {
        return _rfl;
    }

    SceneScheduler &RuntimeContext::scheduler() {
        return _scheduler;
    }

    const SceneScheduler &RuntimeContext::scheduler() const {
        return _scheduler;
    }

    void RuntimeContext::start_stage_bgm(std::string_view name) {
        _audio.start_stage_bgm(name);
        _logger.info(logging::Category::APP, logging::Message{"SMG requested stage BGM {}"}, name);
    }

    void RuntimeContext::unlock_stage_bgm() {
        _audio.unlock_stage_bgm();
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct unlocked stage BGM"});
    }

    void RuntimeContext::stop_stage_bgm(s32 fade_frames) {
        _audio.stop_stage_bgm(fade_frames);
        _logger.info(logging::Category::APP, logging::Message{"SMG stopped stage BGM over {} frames"}, fade_frames);
    }

    void RuntimeContext::start_system_sound(std::string_view name) {
        _audio.start_system_sound(name);
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct requested system sound {}"}, name);
    }

    void RuntimeContext::start_cs_sound(std::string_view name) {
        _audio.start_controller_speaker_sound(name);
        _logger.info(logging::Category::APP, logging::Message{"TitleSequenceProduct requested controller speaker sound {}"}, name);
    }

    void RuntimeContext::emit_effect(std::string_view actor_name, std::string_view effect_name) {
        _effects.emit(actor_name, effect_name);
        _logger.info(logging::Category::APP, logging::Message{"{} emitted effect {}"}, actor_name, effect_name);
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

    void RuntimeContext::note_debug_event(std::string_view message) {
        _logger.info(logging::Category::APP, logging::Message{"SMG debug: {}"}, message);
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

    void RuntimeContext::register_layout(SimpleLayout &layout) {
        _scheduler.register_layout(layout, MR::MovementType_Layout, -1, MR::DrawType_Layout);
    }

    void RuntimeContext::unregister_layout(SimpleLayout &layout) {
        _scheduler.unregister_layout(layout);
    }

    void RuntimeContext::register_sky_actor(LiveActor &actor) {
        _scheduler.register_live_actor_model(actor, MR::MovementType_Sky, MR::CalcAnimType_MapObj, MR::DrawBufferType_Sky, -1);
    }

    void RuntimeContext::unregister_sky_actor(LiveActor &actor) {
        _scheduler.unregister_live_actor_model(actor);
    }

    std::filesystem::path RuntimeContext::resolve_disc_files_root() const {
        const auto cwd = std::filesystem::current_path();
        const std::vector<std::filesystem::path> candidates{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
            cwd / ".." / "orig" / "RMGK01" / "files",
        };

        for (const auto &candidate : candidates) {
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
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}

s64 OSTicksToSeconds(OSTime ticks) {
    return ticks;
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
