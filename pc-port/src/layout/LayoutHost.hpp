#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>
#include <revolution.h>

class ButtonPaneController;
class LayoutActor;
class LayoutManager;
class LayoutPaneCtrl;
class NameObj;

namespace nw4r::lyt {
    class TexMap;
}

namespace smgpc::layout {

class LayoutRuntime;

struct PaneControlAnimationDebugState {
    u32 layer_index = 0U;
    std::string name;
    f32 frame = 0.0F;
    f32 end_frame = 0.0F;
    f32 rate = 0.0F;
    bool stopped = true;
    bool looping = false;
};

struct PaneControlDebugState {
    std::string pane_name;
    bool exists_in_layout = false;
    bool visible = false;
    std::vector< PaneControlAnimationDebugState > animations;
};

struct ButtonControllerDebugState {
    std::string pane_name;
    std::string bounding_pane_name;
    std::string nerve;
    u32 anim_layer = 0U;
    bool active = false;
    bool selected = false;
    bool pointing = false;
    bool appearance_enabled = true;
    bool decide_enabled = true;
    f32 pointing_anim_start_frame = 0.0F;
};

[[nodiscard]] LayoutRuntime* layout_runtime(LayoutActor* actor);
[[nodiscard]] const LayoutRuntime* layout_runtime(const LayoutActor* actor);
[[nodiscard]] LayoutRuntime& require_layout_runtime(LayoutActor* actor, std::string_view operation);
[[nodiscard]] const LayoutRuntime& require_layout_runtime(const LayoutActor* actor, std::string_view operation);

[[nodiscard]] bool is_layout_actor_dead(const LayoutActor* actor);
void release_layout_actor_if_registered(NameObj* object);
void draw_layout_actor(const LayoutActor* actor);
void start_layout_anim(LayoutActor* actor, const char* animation_name, u32 layer);
void set_layout_anim_frame(LayoutActor* actor, f32 frame, u32 layer);
void set_layout_anim_frame_and_stop(LayoutActor* actor, f32 frame, u32 layer);
void set_layout_anim_rate(LayoutActor* actor, f32 rate, u32 layer);
[[nodiscard]] f32 layout_anim_frame(const LayoutActor* actor, u32 layer);
[[nodiscard]] f32 layout_anim_frame_max(const LayoutActor* actor, u32 layer);
[[nodiscard]] bool is_layout_anim_stopped(LayoutActor* actor, u32 layer);
[[nodiscard]] J3DFrameCtrl* layout_anim_ctrl(LayoutActor* actor, u32 layer);

void set_text_box_number(LayoutActor* actor, const char* pane_name, s32 number);
void set_text_box_string(LayoutActor* actor, const char* pane_name, std::u16string_view text);
void set_layout_scale(LayoutActor* actor, f32 x, f32 y);

void set_pane_visible(LayoutManager* manager, const char* pane_name, bool visible, bool recursive);
[[nodiscard]] bool is_pane_visible(const LayoutManager* manager, const char* pane_name);
[[nodiscard]] bool is_pointing_pane(const LayoutManager* manager, const char* pane_name, f32 screen_x, f32 screen_y);
void set_pane_alpha(LayoutManager* manager, const char* pane_name, f32 alpha);
void replace_pane_texture(LayoutManager* manager, const char* pane_name, const nw4r::lyt::TexMap& texture, u8 texture_index);
void set_text_box_tagged_string(LayoutManager* manager, const char* pane_name, std::u16string_view raw_text,
                                std::u16string_view display_text);
void set_text_box_arg_number(LayoutManager* manager, const char* pane_name, s32 number, s32 arg_index);
void set_text_box_arg_string(LayoutManager* manager, const char* pane_name, std::u16string_view text, s32 arg_index);
void set_text_box_horizontal_position(LayoutManager* manager, const char* pane_name, u8 position);
void set_text_box_vertical_position(LayoutManager* manager, const char* pane_name, u8 position);

void set_pane_anim_frame(LayoutPaneCtrl* pane_ctrl, f32 frame, u32 layer);
void set_pane_anim_rate(LayoutPaneCtrl* pane_ctrl, f32 rate, u32 layer);
[[nodiscard]] f32 pane_anim_frame(const LayoutPaneCtrl* pane_ctrl, u32 layer);
[[nodiscard]] f32 pane_anim_frame_max(const LayoutPaneCtrl* pane_ctrl, u32 layer);

[[nodiscard]] f32 animation_duration(const LayoutManager* manager, const char* animation_name);
[[nodiscard]] f32 pane_animation_frame(const LayoutManager* manager, const char* pane_name, u32 layer);
[[nodiscard]] f32 pane_animation_frame_max(const LayoutManager* manager, const char* pane_name, u32 layer);
[[nodiscard]] bool is_pane_animation_stopped(const LayoutManager* manager, const char* pane_name, u32 layer);

void register_button_controller(LayoutManager* manager, ButtonPaneController* controller);
void unregister_button_controller(LayoutManager* manager, ButtonPaneController* controller);
void refresh_pane_matrices(LayoutManager* manager);

#ifndef NDEBUG
[[nodiscard]] std::vector< PaneControlDebugState > debug_pane_controls(const LayoutManager* manager);
[[nodiscard]] std::vector< ButtonControllerDebugState > debug_button_controllers(const LayoutManager* manager);
#endif

}  // namespace smgpc::layout
