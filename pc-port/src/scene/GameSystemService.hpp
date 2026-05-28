#pragma once

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"

namespace smgpc::compat {

    class RuntimeContext;
    class GameSystemSceneControllerService;
    class SequenceBootService;

    class GameSystemService final {
    public:
        GameSystemService(RuntimeContext &runtime, GameSystemSceneControllerService &scene_controller, SequenceBootService &sequence_boot);
        ~GameSystemService();

        GameSystemService(const GameSystemService &) = delete;
        GameSystemService &operator=(const GameSystemService &) = delete;

        void begin_frame(const render::FrameContext &frame_context);
        void update();
        void draw_3d_normal(render::IRendererEngine &renderer);
        void draw_3d_normal(render::IRendererEngine &renderer, const CameraPoseCompat &camera_pose);
        void draw_2d_normal(render::IRendererEngine &renderer);

        [[nodiscard]] bool has_boot_requested_initial_stage() const;
        [[nodiscard]] bool is_initial_stage_host_active() const;
        [[nodiscard]] bool has_sent_autorush_begin() const;

    private:
        void ensure_initial_sequence_requested();

        RuntimeContext &_runtime;
        GameSystemSceneControllerService &_scene_controller;
        SequenceBootService &_sequence_boot;
    };

}  // namespace smgpc::compat
