#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneDataInitializer.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "scene/OriginalSceneSupport.hpp"
#include "scene/OriginalPlacementCoverage.hpp"
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
    auto* initializer = ::getSceneDataInitializer();
    if (!initializer->mDataHolder)
        aurora::throw_host_exception<std::logic_error>("Original placement requires its loaded stage holder");
    smgpc::scene::report_original_placement_coverage(*initializer->mDataHolder);
    initializer->startActorPlacement();
}

void SceneFunction::initAfterScenarioSelected() {
    ::getSceneDataInitializer()->startStageFileLoadAfterScenarioSelected();
    ::getSceneDataInitializer()->initAfterScenarioSelected();
    // The original holder tree now includes the selected scenario's zones.
    // Retain their authored lighting before original Game creates any actors.
    smgpc::scene::initialize_original_scene_lights();
}

void SceneFunction::initEffectSystem(u32 particles, u32 emitters) {
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
