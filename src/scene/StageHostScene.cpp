#include "scene/StageHostScene.hpp"

#include "runtime/RuntimeContext.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/StageInitializationService.hpp"

#include <aurora/exception.hpp>
#include <stdexcept>
#include <utility>

namespace smgpc::scene {
    StageHostScene::StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request)
        : Scene(!request.stage_name.empty() ? request.stage_name.c_str() : "StageHostScene"),
          _runtime(runtime) {
    }

    StageHostScene::~StageHostScene() = default;

    void StageHostScene::bind_initialization(StageInitializationService &initialization) {
        if (_initialization) {
            aurora::throw_host_exception<std::logic_error>("Stage Scene already has an initialization owner");
        }
        _initialization = &initialization;
    }

    void StageHostScene::init() {
        if (!_initialization) {
            aurora::throw_host_exception<std::logic_error>("Stage Scene requires its native initialization owner");
        }
        _initialization->initialize_host_scene();
    }

    void StageHostScene::start() {
    }

    void StageHostScene::update() {
        _runtime.scene_execution().execute_movement();
    }

    void StageHostScene::calcAnim() {
        _runtime.scene_execution().execute_calc_anim_and_view();
    }

    void StageHostScene::draw3DNormal(const smgpc::camera::CameraPose &camera_pose) {
        _runtime.scene_execution().draw_3d_normal(camera_pose);
    }

    void StageHostScene::draw2DNormal() {
        _runtime.scene_execution().draw_2d_normal();
    }

    NameObj *StageHostScene::root() const {
        return _initialization->root();
    }
    std::string_view StageHostScene::scene_name() const {
        return _initialization->scene_name();
    }
    std::string_view StageHostScene::stage_name() const {
        return _initialization->stage_name();
    }
    s32 StageHostScene::scenario_no() const {
        return _initialization->scenario_no();
    }
}  // namespace smgpc::scene
