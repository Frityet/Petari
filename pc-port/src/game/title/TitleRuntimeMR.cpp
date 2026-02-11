#include "TitleRuntimeMR.hpp"

#include <cstdint>

#include "Logger.hpp"
#include "RenderWindow.hpp"
#include "TitleLayoutActor.hpp"

namespace smgpc::game::title::MR {
namespace {

constexpr int ENTER_KEY = 257;
constexpr int A_KEY = 90;
constexpr int B_KEY = 88;

render::IRendererService *s_renderer_service {};
logging::ILogger *s_logger {};
bool s_stage_bgm_prepared {};

}  // namespace

void begin_frame() {
}

void set_input_source(void *renderer_service, void *logger) {
    s_renderer_service = static_cast<render::IRendererService *>(renderer_service);
    s_logger = static_cast<logging::ILogger *>(logger);
}

bool is_display_encourage_pal60_window() {
    return false;
}

void start_anim(TitleLayoutActor *actor, const char *animation_name, std::uint32_t layer) {
    if (actor == nullptr || animation_name == nullptr) {
        return;
    }
    actor->start_anim(animation_name, layer);
}

bool is_anim_stopped(const TitleLayoutActor *actor, std::uint32_t layer) {
    if (actor == nullptr) {
        return true;
    }
    return actor->is_anim_stopped(layer);
}

void set_anim_frame_and_stop(TitleLayoutActor *actor, float frame, std::uint32_t layer) {
    if (actor == nullptr) {
        return;
    }
    actor->set_anim_frame_and_stop(frame, layer);
}

void emit_effect(TitleLayoutActor *actor, const char *effect_name) {
    if (actor == nullptr) {
        return;
    }
    actor->emit_effect(effect_name);
}

void delete_effect_all(TitleLayoutActor *actor) {
    if (actor == nullptr) {
        return;
    }
    actor->delete_effect_all();
}

bool is_dead(const TitleLayoutActor *actor) {
    if (actor == nullptr) {
        return true;
    }
    return actor->is_dead();
}

bool test_core_pad_button_a(int channel) {
    (void)channel;
    return s_renderer_service != nullptr && (s_renderer_service->is_key_down(ENTER_KEY) || s_renderer_service->is_key_down(A_KEY));
}

bool test_core_pad_button_b(int channel) {
    (void)channel;
    return s_renderer_service != nullptr && (s_renderer_service->is_key_down(ENTER_KEY) || s_renderer_service->is_key_down(B_KEY));
}

void start_stage_bgm(const char *name, bool prepare) {
    s_stage_bgm_prepared = prepare;
    if (s_logger != nullptr) {
        s_logger->debug(__FILE__, __LINE__, logging::Category::GAME, "start_stage_bgm name={} prepare={}", name != nullptr ? name : "<null>", prepare);
    }
}

bool is_prepared_stage_bgm() {
    return s_stage_bgm_prepared;
}

void unlock_stage_bgm() {
    if (s_logger != nullptr) {
        s_logger->debug(__FILE__, __LINE__, logging::Category::GAME, "unlock_stage_bgm");
    }
}

void stop_stage_bgm(int fade_frames) {
    if (s_logger != nullptr) {
        s_logger->debug(__FILE__, __LINE__, logging::Category::GAME, "stop_stage_bgm fade_frames={}", fade_frames);
    }
}

void start_system_se(const char *name, int, int) {
    if (s_logger != nullptr) {
        s_logger->debug(__FILE__, __LINE__, logging::Category::GAME, "start_system_se {}", name != nullptr ? name : "<null>");
    }
}

void start_cs_sound(const char *name, int, int) {
    if (s_logger != nullptr) {
        s_logger->debug(__FILE__, __LINE__, logging::Category::GAME, "start_cs_sound {}", name != nullptr ? name : "<null>");
    }
}

void try_rumble_pad_middle(void *, int) {
    if (s_logger != nullptr) {
        s_logger->debug(__FILE__, __LINE__, logging::Category::GAME, "try_rumble_pad_middle");
    }
}

}  // namespace smgpc::game::title::MR
