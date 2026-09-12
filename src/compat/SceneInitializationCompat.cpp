#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneDataInitializer.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "scene/StageInitializationService.hpp"
#include "scene/OriginalSceneSupport.hpp"
#include "scene/SceneExecutionBinding.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
    SceneDataInitializer* getSceneDataInitializer() {
        return MR::getSceneObj< SceneDataInitializer >(SceneObj_SceneDataInitializer);
    }
};  // namespace

void SceneFunction::startStageFileLoad() {
    MR::createSceneObj(SceneObj_SceneDataInitializer);
    ::getSceneDataInitializer()->startStageFileLoad();
}

void SceneFunction::waitDoneStageFileLoad() {
    ::getSceneDataInitializer()->waitDoneStageFileLoad();
}

void SceneFunction::startActorFileLoadCommon() {
    ::getSceneDataInitializer()->startActorFileLoadCommon();
}

void SceneFunction::startActorFileLoadScenario() {
    ::getSceneDataInitializer()->startActorFileLoadScenario();
}

void SceneFunction::startActorPlacement() {
    ::getSceneDataInitializer()->startActorPlacement();
}

void SceneFunction::initAfterScenarioSelected() {
    ::getSceneDataInitializer()->startStageFileLoadAfterScenarioSelected();
    ::getSceneDataInitializer()->initAfterScenarioSelected();
}

void SceneFunction::initEffectSystem(u32 particles, u32 emitters) {
    if (auto* initializer = smgpc::scene::current_stage_initialization_service())
        initializer->initialize_effect_system(particles, emitters);
    else
        smgpc::scene::initialize_original_scene_effects(particles, emitters);
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

void SceneFunction::initForLiveActor() {
    MR::createSceneObj(SceneObj_AllLiveActorGroup);
    MR::createSceneObj(SceneObj_ClippingDirector);
    MR::createSceneObj(SceneObj_DemoDirector);
    MR::createSceneObj(SceneObj_SensorHitChecker);
    MR::createSceneObj(SceneObj_CollisionDirector);
    MR::createSceneObj(SceneObj_MessageSensorHolder);
    MR::createSceneObj(SceneObj_LiveActorGroupArray);
    MR::createSceneObj(SceneObj_MovementOnOffGroupHolder);
    MR::createSceneObj(SceneObj_LightDirector);
    MR::createSceneObj(SceneObj_AreaObjContainer);
    MR::createSceneObj(SceneObj_CaptureScreenActor);
    MR::createSceneObj(SceneObj_StageSwitchContainer);
    MR::createSceneObj(SceneObj_SwitchWatcherHolder);
    MR::createSceneObj(SceneObj_SleepControllerHolder);
    MR::createSceneObj(SceneObj_TalkDirector);
    MR::createSceneObj(SceneObj_NPCDirector);
}

void SceneFunction::createHioBasicNode(Scene*) {
}
