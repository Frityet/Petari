#include "Game/NameObj/NameObjHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/StopSceneController.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
    NameObjHolder &scene_objects() {
        if (auto* system = SingletonHolder<GameSystem>::get(); system && system->mSceneController && system->mSceneController->mObjHolder)
            return *system->mSceneController->mObjHolder;
        if (auto *registry = smgpc::scene::current_scene_name_obj_registry())
            return registry->holder();
        aurora::throw_host_exception<std::logic_error>("Scene object operations require the actual scene NameObjHolder");
    }
}  // namespace

namespace MR {
    void callMethodAllSceneNameObj(NameObjMethod method) {
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

}  // namespace MR
