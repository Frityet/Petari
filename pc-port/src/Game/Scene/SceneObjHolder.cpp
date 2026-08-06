#include "Game/Scene/SceneObjHolder.hpp"

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"

namespace {
    SceneObjHolder* sCurrentSceneObjHolder = nullptr;

    SceneObjHolder* fallback_scene_obj_holder() {
        static auto holder = SceneObjHolder();
        return &holder;
    }
}  // namespace

SceneObjHolder::~SceneObjHolder() {
    delete _mii_face_parts_holder;
    delete _prologue_holder;
    delete _stage_switch_container;
    delete _switch_watcher_holder;
    delete _sleep_controller_holder;
}

NameObj* SceneObjHolder::create(int id) {
    if (id == SceneObj_StageSwitchContainer) {
        if (_stage_switch_container == nullptr) {
            _stage_switch_container = new StageSwitchContainer();
        }
        return _stage_switch_container;
    }

    if (id == SceneObj_SwitchWatcherHolder) {
        if (_switch_watcher_holder == nullptr) {
            _switch_watcher_holder = new SwitchWatcherHolder();
        }
        return _switch_watcher_holder;
    }

    if (id == SceneObj_SleepControllerHolder) {
        if (_sleep_controller_holder == nullptr) {
            _sleep_controller_holder = new SleepControllerHolder();
        }
        return _sleep_controller_holder;
    }

    if (id == SceneObj_PrologueHolder) {
        if (_prologue_holder == nullptr) {
            _prologue_holder = new PrologueHolder("プロローグ保持");
        }
        return _prologue_holder;
    }

    static_cast<void>(getObj(id));
    return nullptr;
}

void* SceneObjHolder::getObj(int id) {
    if (id == SceneObj_StageSwitchContainer) {
        return create(id);
    }

    if (id == SceneObj_SwitchWatcherHolder) {
        return create(id);
    }

    if (id == SceneObj_SleepControllerHolder) {
        return create(id);
    }

    if (id == SceneObj_MiiFacePartsHolder) {
        if (_mii_face_parts_holder == nullptr) {
            _mii_face_parts_holder = new MiiFacePartsHolder();
        }
        return _mii_face_parts_holder;
    }

    if (id == SceneObj_PrologueHolder) {
        return create(id);
    }

    return nullptr;
}

namespace MR {
    NameObj* createSceneObj(int id) {
        return getSceneObjHolder()->create(id);
    }

    SceneObjHolder* getSceneObjHolder() {
        return sCurrentSceneObjHolder != nullptr ? sCurrentSceneObjHolder : fallback_scene_obj_holder();
    }

    void setCurrentSceneObjHolder(SceneObjHolder* pHolder) {
        sCurrentSceneObjHolder = pHolder;
    }
}  // namespace MR
