#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <revolution.h>

#include "Game/compat/CameraPose.hpp"
#include "RendererService.hpp"

class LiveActor;
class NameObj;
class SimpleLayout;

namespace smgpc::game {

    enum class SceneEntryKind {
        NameObj,
        Layout,
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
        std::vector<SceneLayoutAnimationDebugState> animations;
    };

    class SceneScheduler final {
    public:
        void connect_name_obj(NameObj &obj, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type);
        void disconnect_name_obj(NameObj &obj);
        void register_layout(SimpleLayout &layout, s32 movement_type, s32 calc_anim_type, s32 draw_type);
        void unregister_layout(SimpleLayout &layout);
        void register_live_actor_model(LiveActor &actor, s32 movement_type, s32 calc_anim_type, s32 draw_buffer_type, s32 draw_type);
        void unregister_live_actor_model(LiveActor &actor);

        void execute_movement();
        void execute_calc_anim();
        void execute_calc_view_and_entry();
        void execute_draw_buffer_opa(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, s32 draw_buffer_type);
        void execute_draw_buffer_xlu(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, s32 draw_buffer_type);
        void execute_draw_buffer_list_normal_opa_before_volume_shadow(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose,
                                                                      bool prior_draw_air);
        void execute_draw_buffer_list_normal_opa_before_silhouette(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose);
        void execute_draw_buffer_list_normal_opa(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, bool prior_draw_air);
        void execute_draw_buffer_list_normal_xlu(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose);
        void execute_draw_buffer_list_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, bool prior_draw_air = false);
        void execute_draw_list_2d_normal(render::IRendererEngine &renderer);

        [[nodiscard]] std::vector<SceneSchedulerEntryState> snapshot() const;
        [[nodiscard]] std::span<const SceneSchedulerEntryState> last_execution_trace() const;
        [[nodiscard]] std::vector<SceneLayoutRuntimeDebugState> debug_layout_runtime_snapshot() const;
        void clear();

        struct Entry {
            SceneEntryKind kind = SceneEntryKind::NameObj;
            NameObj *name_obj = nullptr;
            SimpleLayout *layout = nullptr;
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
        void execute_draw_buffer(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose, s32 draw_buffer_type,
                                 SceneDrawBufferPass pass);
        void execute_draw_type(render::IRendererEngine &renderer, s32 draw_type);
        void push_trace(const Entry &entry, SceneSchedulerPhase phase, SceneDrawBufferPass pass = SceneDrawBufferPass::None);

        std::vector<Entry> _entries;
        std::vector<SceneSchedulerEntryState> _last_execution_trace;
        std::size_t _next_order = 0U;
    };

}  // namespace smgpc::game
