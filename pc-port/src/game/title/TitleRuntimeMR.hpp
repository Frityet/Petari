#pragma once

#include <cstdint>

namespace smgpc::game::title {

class TitleLayoutActor;

namespace MR {

void begin_frame();
void set_input_source(void *renderer_service, void *logger);

[[nodiscard]] bool is_display_encourage_pal60_window();

void start_anim(TitleLayoutActor *actor, const char *animation_name, std::uint32_t layer);
[[nodiscard]] bool is_anim_stopped(const TitleLayoutActor *actor, std::uint32_t layer);
void set_anim_frame_and_stop(TitleLayoutActor *actor, float frame, std::uint32_t layer);
void emit_effect(TitleLayoutActor *actor, const char *effect_name);
void delete_effect_all(TitleLayoutActor *actor);

[[nodiscard]] bool is_dead(const TitleLayoutActor *actor);

[[nodiscard]] bool test_core_pad_button_a(int channel);
[[nodiscard]] bool test_core_pad_button_b(int channel);

void start_stage_bgm(const char *name, bool prepare);
[[nodiscard]] bool is_prepared_stage_bgm();
void unlock_stage_bgm();
void stop_stage_bgm(int fade_frames);
void start_system_se(const char *name, int, int);
void start_cs_sound(const char *name, int, int);
void try_rumble_pad_middle(void *, int);

}  // namespace MR

}  // namespace smgpc::game::title
