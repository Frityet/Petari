#include "Game/Scene/SceneObjHolder.hpp"

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/MapObj/CoinHolder.hpp"
#include "Game/MapObj/CoinRotater.hpp"
#include "Game/MapObj/PurpleCoinHolder.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/NameObj/NameObj.hpp"

namespace {
    SceneObjHolder* sCurrentSceneObjHolder = nullptr;

    SceneObjHolder* fallback_scene_obj_holder() {
        static auto holder = SceneObjHolder();
        return &holder;
    }
}  // namespace

SceneObjHolder::~SceneObjHolder() {
    delete _mii_face_parts_holder;
    for (auto* object : mObjects) {
        delete object;
    }
}

NameObj* SceneObjHolder::create(int id) {
    if (id < 0 || id >= SceneObj_NumMax || id == SceneObj_MiiFacePartsHolder) {
        return nullptr;
    }

    if (mObjects[static_cast< std::size_t >(id)] != nullptr) {
        return mObjects[static_cast< std::size_t >(id)];
    }

    auto* object = newEachObj(id);
    if (object != nullptr) {
        object->initWithoutIter();
        mObjects[static_cast< std::size_t >(id)] = object;
    }
    return object;
}

NameObj* SceneObjHolder::newEachObj(int id) {
    switch (id) {
    case SceneObj_StageSwitchContainer:
        return new StageSwitchContainer();
    case SceneObj_SwitchWatcherHolder:
        return new SwitchWatcherHolder();
    case SceneObj_SleepControllerHolder:
        return new SleepControllerHolder();
    case SceneObj_CoinHolder:
        return new CoinHolder("コイン管理");
    case SceneObj_PurpleCoinHolder:
        return new PurpleCoinHolder();
    case SceneObj_CoinRotater:
        return new CoinRotater("コイン回転管理");
    case SceneObj_PrologueHolder:
        return new PrologueHolder("プロローグ保持");
    default:
        return nullptr;
    }
}

void* SceneObjHolder::getObj(int id) {
    if (id == SceneObj_MiiFacePartsHolder) {
        if (_mii_face_parts_holder == nullptr) {
            _mii_face_parts_holder = new MiiFacePartsHolder();
        }
        return _mii_face_parts_holder;
    }

    return create(id);
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
