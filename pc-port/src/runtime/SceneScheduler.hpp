#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <revolution.h>

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"

class LiveActor;
class LayoutActor;
class NameObj;
class SimpleLayout;

namespace smgpc::runtime {

    enum class SceneEntryKind {
        NameObj,
        Layout,
        LayoutActor,
        LiveActorModel,
    };

    enum class SceneSchedulerPhase {
        None,
        Movement,
        CalcAnim,
        CalcViewAndEntry,
        DrawBufferOpa,
        DrawBufferXlu,
        DrawType,
    };

    enum class SceneDrawBufferPass {
        None,
        Opaque,
        Translucent,
    };

#ifndef NDEBUG
    struct SceneSchedulerEntryState {
        SceneEntryKind kind = SceneEntryKind::NameObj;
        SceneSchedulerPhase phase = SceneSchedulerPhase::None;
        std::string name;
        s32 movement_type = -1;
        s32 calc_anim_type = -1;
        s32 draw_buffer_type = -1;
        s32 draw_type = -1;
        SceneDrawBufferPass draw_buffer_pass = SceneDrawBufferPass::None;
        std::size_t order = 0U;
        bool suspended = false;
        bool dead = false;
        bool has_live_actor_state = false;
        s32 live_actor_nerve_step = 0;
        std::array<float, 3U> live_actor_position{};
        std::array<float, 3U> live_actor_rotation{};
        std::array<float, 3U> live_actor_scale{};
        std::array<float, 12U> live_actor_base_matrix{};
        std::string live_actor_bck_name;
        std::string live_actor_brk_name;
        std::string live_actor_btk_name;
    };

    struct SceneLayoutAnimationDebugState {
        std::size_t layer_index = 0U;
        std::string name;
        float frame = 0.0F;
        float end_frame = 0.0F;
        float rate = 0.0F;
        bool stopped = true;
        bool looping = false;
    };

    struct SceneLayoutPaneControlAnimationDebugState {
        std::size_t layer_index = 0U;
        std::string name;
        float frame = 0.0F;
        float end_frame = 0.0F;
        float rate = 0.0F;
        bool stopped = true;
        bool looping = false;
    };

    struct SceneLayoutPaneControlDebugState {
        std::string pane_name;
        bool exists_in_layout = false;
        bool visible = true;
        std::vector<SceneLayoutPaneControlAnimationDebugState> animations;
    };

    struct SceneLayoutButtonControllerDebugState {
        std::string pane_name;
        std::string bounding_pane_name;
        std::string nerve;
        u32 anim_layer = 0U;
        bool active = false;
        bool selected = false;
        bool pointing = false;
        bool appearance_enabled = true;
        bool decide_enabled = true;
        float pointing_anim_start_frame = 0.0F;
    };

    struct SceneLayoutPaneContentDebugState {
        std::string kind;
        std::string name;
        s32 material_index = -1;
        std::string material_name;
        std::string texture_name;
        std::string font_name;
        bool visible = true;
    };

    struct SceneLayoutPaneRuntimeDebugState {
        std::size_t index = 0U;
        std::string name;
        s32 parent_index = -1;
        bool base_visible = true;
        bool effective_visible = true;
        float translate_x = 0.0F;
        float translate_y = 0.0F;
        float scale_x = 1.0F;
        float scale_y = 1.0F;
        float alpha = 255.0F;
        float width = 0.0F;
        float height = 0.0F;
        std::vector<SceneLayoutPaneContentDebugState> contents;
    };

    struct SceneLayoutMaterialTextureDebugState {
        std::size_t slot = 0U;
        u16 texture_index = 0U;
        std::string texture_name;
        u8 wrap_s = 0U;
        u8 wrap_t = 0U;
        u8 min_filter = 0U;
        u8 mag_filter = 0U;
    };

    struct SceneLayoutMaterialDebugState {
        std::size_t index = 0U;
        std::string name;
        std::size_t texture_count = 0U;
        std::size_t tex_coord_gen_count = 0U;
        std::size_t tev_stage_count = 0U;
        bool alpha_compare_enabled = false;
        bool blend_enabled = false;
        std::vector<SceneLayoutMaterialTextureDebugState> textures;
    };

    struct SceneLayoutTextureDebugState {
        std::size_t index = 0U;
        std::string name;
        u16 width = 0U;
        u16 height = 0U;
        u32 format_raw = 0U;
        std::string format_name;
        bool uploaded = false;
        std::size_t rgba_byte_count = 0U;
    };

    struct SceneLayoutRuntimeDebugState {
        std::string name;
        std::string layout_name;
        bool has_archive_path = false;
        std::string archive_path;
        s32 movement_type = -1;
        s32 calc_anim_type = -1;
        s32 draw_type = -1;
        std::size_t order = 0U;
        bool suspended = false;
        bool dead = false;
        std::size_t pane_count = 0U;
        std::size_t picture_count = 0U;
        std::size_t text_box_count = 0U;
        std::size_t material_count = 0U;
        std::size_t texture_count = 0U;
        std::size_t font_count = 0U;
        std::size_t committed_pane_frame_count = 0U;
        std::vector<SceneLayoutAnimationDebugState> animations;
        std::vector<SceneLayoutPaneControlDebugState> pane_controls;
        std::vector<SceneLayoutButtonControllerDebugState> button_controllers;
        std::vector<SceneLayoutPaneRuntimeDebugState> panes;
        std::vector<SceneLayoutMaterialDebugState> materials;
        std::vector<SceneLayoutTextureDebugState> textures;
    };

    struct SceneSchedulerMessageTraceEntry {
        std::uint64_t sequence = 0U;
        u32 message = 0U;
        std::string target_name;
        SceneEntryKind target_kind = SceneEntryKind::NameObj;
        s32 target_movement_type = -1;
        s32 target_calc_anim_type = -1;
        s32 target_draw_buffer_type = -1;
        s32 target_draw_type = -1;
        std::size_t target_order = 0U;
        bool target_dead = false;
        bool target_suspended = false;
        bool excluded = false;
        bool delivered = false;
        bool accepted = false;
        bool sender_sensor_present = false;
        bool receiver_sensor_present = false;
        u32 sender_sensor_type = 0U;
        u32 receiver_sensor_type = 0U;
        std::string sender_sensor_host_name;
        std::string receiver_sensor_host_name;
    };
#endif

    class SceneScheduler final {
    public:
        void connect_name_obj(NameObj &obj, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type);
        void disconnect_name_obj(NameObj &obj);
        void register_layout(SimpleLayout &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type);
        void unregister_layout(SimpleLayout &layout);
        void register_layout_actor(LayoutActor &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type);
        void unregister_layout_actor(LayoutActor &layout);
        void register_live_actor_model(LiveActor &actor, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type);
        void unregister_live_actor_model(LiveActor &actor);

        void execute_movement();
        void execute_calc_anim();
        void execute_calc_view_and_entry();
        void execute_draw_buffer_opa(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose, s32 draw_buffer_type);
        void execute_draw_buffer_xlu(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose, s32 draw_buffer_type);
        void execute_draw_buffer_list_normal_opa_before_volume_shadow(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose,
                                                                      bool prior_draw_air);
        void execute_draw_buffer_list_normal_opa_before_silhouette(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose);
        void execute_draw_buffer_list_normal_opa(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose, bool prior_draw_air);
        void execute_draw_buffer_list_normal_xlu(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose);
        void execute_draw_buffer_list_normal(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose, bool prior_draw_air = false);
        void execute_draw_type(render::AuroraRenderer &renderer, s32 draw_type);
        void execute_draw_list_2d_normal(render::AuroraRenderer &renderer);
        std::size_t send_message_to_live_actors(u32 msg, LiveActor *exclude_actor);

#ifndef NDEBUG
        [[nodiscard]] std::vector<SceneSchedulerEntryState> snapshot() const;
        [[nodiscard]] std::span<const SceneSchedulerEntryState> last_execution_trace() const;
        [[nodiscard]] std::span<const SceneSchedulerMessageTraceEntry> message_trace() const;
        [[nodiscard]] std::vector<SceneLayoutRuntimeDebugState> debug_layout_runtime_snapshot() const;
#endif
        void clear();

        struct Entry {
            SceneEntryKind kind = SceneEntryKind::NameObj;
            NameObj *name_obj = nullptr;
            SimpleLayout *layout = nullptr;
            LayoutActor *layout_actor = nullptr;
            LiveActor *live_actor = nullptr;
            s32 movement_type = -1;
            s32 calc_anim_type = -1;
            s32 draw_buffer_type = -1;
            s32 draw_type = -1;
            std::size_t order = 0U;
        };

    private:
        [[nodiscard]] Entry *find_entry(SceneEntryKind kind, const void *ptr);
        [[nodiscard]] const Entry *find_entry(SceneEntryKind kind, const void *ptr) const;
        [[nodiscard]] std::vector<Entry *> sorted_entries_for_movement();
        [[nodiscard]] std::vector<Entry *> sorted_entries_for_calc_anim();
        [[nodiscard]] std::vector<Entry *> sorted_entries_for_calc_view_and_entry();
        [[nodiscard]] static bool entry_is_dead(const Entry &entry);
        [[nodiscard]] static bool entry_is_suspended(const Entry &entry);
        [[nodiscard]] static std::string entry_name(const Entry &entry);
        void execute_draw_buffer(render::AuroraRenderer &renderer, const smgpc::camera::CameraPose &camera_pose, s32 draw_buffer_type,
                                 SceneDrawBufferPass pass);
#ifndef NDEBUG
        void push_trace(const Entry &entry, SceneSchedulerPhase phase, SceneDrawBufferPass pass = SceneDrawBufferPass::None);
        void push_message_trace(SceneSchedulerMessageTraceEntry trace);
#endif

        std::vector<Entry> _entries;
#ifndef NDEBUG
        std::vector<SceneSchedulerEntryState> _last_execution_trace;
        std::vector<SceneSchedulerMessageTraceEntry> _message_trace;
#endif
        std::size_t _next_order = 0U;
#ifndef NDEBUG
        std::uint64_t _next_message_sequence = 0U;
#endif
    };

}  // namespace smgpc::runtime
