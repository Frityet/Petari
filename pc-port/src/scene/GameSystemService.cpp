#include "scene/GameSystemService.hpp"

#include "runtime/RuntimeContext.hpp"
#include "scene/GameSystemSceneControllerService.hpp"
#include "scene/SequenceBootService.hpp"

namespace smgpc::scene {

    GameSystemService::GameSystemService(smgpc::runtime::RuntimeContext &runtime, GameSystemSceneControllerService &scene_controller,
                                         SequenceBootService &sequence_boot)
        : _runtime(runtime), _scene_controller(scene_controller), _sequence_boot(sequence_boot) {
    }

    GameSystemService::~GameSystemService() = default;

    void GameSystemService::begin_frame(const render::FrameContext &frame_context) {
        ensure_initial_sequence_requested();
        _runtime.begin_frame(frame_context);
    }

    void GameSystemService::update() {
        _scene_controller.check_request_and_change_scene();
        _sequence_boot.update_after_runtime_frame();
        _scene_controller.check_request_and_change_scene();
    }

    void GameSystemService::draw_3d_normal() {
        _runtime.draw_3d_normal();
    }

    void GameSystemService::draw_3d_normal(const smgpc::camera::CameraPose &camera_pose) {
        _runtime.draw_3d_normal(camera_pose);
    }

    void GameSystemService::draw_2d_normal() {
        _runtime.draw_2d_normal();
    }

    bool GameSystemService::has_boot_requested_initial_stage() const {
        return _sequence_boot.is_boot_requested();
    }

    bool GameSystemService::is_initial_stage_host_active() const {
        return _sequence_boot.is_initial_stage_host_active();
    }

    bool GameSystemService::has_sent_autorush_begin() const {
        return _sequence_boot.has_sent_autorush_begin();
    }

    void GameSystemService::ensure_initial_sequence_requested() {
        if (!_sequence_boot.is_boot_requested()) {
            _sequence_boot.request_boot_to_initial_stage();
        }
    }

}  // namespace smgpc::scene
