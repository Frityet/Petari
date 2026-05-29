#pragma once

#include "RendererService.hpp"
#include "camera/CameraPose.hpp"

namespace smgpc::runtime {
    class RuntimeContext;
}  // namespace smgpc::runtime

namespace smgpc::scene {

    class GameSystemSceneControllerService;
    class SequenceBootService;

    class GameSystemService final {
    public:
        GameSystemService(smgpc::runtime::RuntimeContext &runtime, GameSystemSceneControllerService &scene_controller, SequenceBootService &sequence_boot);
        ~GameSystemService();

        GameSystemService(const GameSystemService &) = delete;
        GameSystemService &operator=(const GameSystemService &) = delete;

        void begin_frame(const render::FrameContext &frame_context);
        void update();
        void draw_3d_normal();
        void draw_3d_normal(const smgpc::camera::CameraPose &camera_pose);
        void draw_2d_normal();

        [[nodiscard]] bool has_boot_requested_initial_stage() const;
        [[nodiscard]] bool is_initial_stage_host_active() const;
        [[nodiscard]] bool has_sent_autorush_begin() const;

    private:
        void ensure_initial_sequence_requested();

        smgpc::runtime::RuntimeContext &_runtime;
        GameSystemSceneControllerService &_scene_controller;
        SequenceBootService &_sequence_boot;
    };

}  // namespace smgpc::scene
