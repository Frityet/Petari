#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "scene/StageInitializationService.hpp"
#include "scene/SceneExecutionBinding.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

void SceneFunction::startStageFileLoad() {
    smgpc::scene::require_stage_initialization_service().start_stage_file_load();
}

void SceneFunction::waitDoneStageFileLoad() {
    smgpc::scene::require_stage_initialization_service().wait_done_stage_file_load();
}

void SceneFunction::startActorFileLoadCommon() {
    smgpc::scene::require_stage_initialization_service().start_actor_file_load_common();
}

void SceneFunction::startActorFileLoadScenario() {
    smgpc::scene::require_stage_initialization_service().start_actor_file_load_scenario();
}

void SceneFunction::startActorPlacement() {
    smgpc::scene::require_stage_initialization_service().place_actors();
}

void SceneFunction::initEffectSystem(u32 particles, u32 emitters) {
    smgpc::scene::require_stage_initialization_service().initialize_effect_system(particles, emitters);
}

void SceneFunction::allocateDrawBufferActorList() {
    auto *binding = smgpc::scene::current_scene_execution_binding();
    if (!binding)
        aurora::throw_host_exception<std::logic_error>("Scene buffer allocation requires its actual execution binding");
    binding->complete_initialization();
}

void SceneFunction::initForNameObj() {
    MR::createSceneObj(SceneObj_NameObjExecuteHolder);
    MR::createSceneObj(SceneObj_StopSceneController);
    MR::createSceneObj(SceneObj_SceneNameObjMovementController);
}

void SceneFunction::createHioBasicNode(Scene*) {
}

namespace MR {
    void completeCameraParameters() {
        auto *camera = smgpc::camera::current_camera_director_runtime();
        if (!camera)
            aurora::throw_host_exception<std::logic_error>("Camera parameter completion requires the actual scene CameraDirector");
        camera->close_creating_chunks();
    }

    bool isStageDisablePauseMenu() {
        return isStageFileSelect() || isStageEpilogueDemo();
    }

    bool isStageFileSelect() {
        return isEqualStageName("FileSelect");
    }

    bool isStageEpilogueDemo() {
        return isEqualStageName("EpilogueDemoStage");
    }
}
