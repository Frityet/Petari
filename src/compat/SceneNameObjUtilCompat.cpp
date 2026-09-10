#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/StopSceneController.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/TalkRuntime.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include "scene/StageInitializationService.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
    NameObjHolder &scene_objects() {
        if (auto *registry = smgpc::scene::current_scene_name_obj_registry())
            return registry->holder();
        aurora::throw_host_exception<std::logic_error>("Scene object operations require the actual scene NameObjHolder");
    }
}  // namespace

namespace MR {
    void callMethodAllSceneNameObj(NameObjMethod method) {
        if (method == &NameObj::initAfterPlacement) {
            if (auto *initialization = smgpc::scene::current_stage_initialization_service()) {
                initialization->finish_actor_placement();
                return;
            }
        }
        scene_objects().callMethodAllObj(method);
    }

    void suspendAllSceneNameObj() {
        scene_objects().suspendAllObj();
    }

    void resumeAllSceneNameObj() {
        scene_objects().resumeAllObj();
    }

    void syncWithFlagsAllSceneNameObj() {
        scene_objects().syncWithFlags();
    }

    void stopScene(s32 frame) {
        MR::getSceneObj<StopSceneController>(SceneObj_StopSceneController)->requestStopScene(frame);
    }

    void stopSceneForDefaultHit(s32 frame) {
        MR::getSceneObj<StopSceneController>(SceneObj_StopSceneController)->requestStopSceneDelay(frame, 2);
    }

    void requestEffectStopSceneStart() {
        Effect::requestMovementOffAllLoopEmitters();
    }

    void requestEffectStopSceneEnd() {
        Effect::requestMovementOnAllEmitters();
    }

    void pauseOffTalkDirector() {
        // The native talk service currently owns its graph, input and rendered
        // presentation together. Resume that exact scheduled owner; its slot
        // is not an original TalkDirector and must never be cast to one.
        requestMovementOn(&smgpc::compat::require_talk_runtime("pauseOffTalkDirector"));
    }
}  // namespace MR
