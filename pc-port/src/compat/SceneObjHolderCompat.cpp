#include "Game/Scene/SceneObjHolder.hpp"

#include "Game/Demo/PrologueDirector.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/MapObj/CoinHolder.hpp"
#include "Game/MapObj/CoinRotater.hpp"
#include "Game/MapObj/PurpleCoinHolder.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

    SceneObjHolder *sCurrentSceneObjHolder = nullptr;
    smgpc::scene::SceneObjHolderBinding *sCurrentSceneObjHolderBinding = nullptr;

}  // namespace

namespace smgpc::scene {

    SceneObjHolderBinding::SceneObjHolderBinding(SceneObjHolder &holder) : _holder(&holder), _owned_objects() {
        if (sCurrentSceneObjHolder != nullptr) {
            throw std::logic_error("a SceneObjHolder is already bound to the active scene");
        }

        sCurrentSceneObjHolder = _holder;
        sCurrentSceneObjHolderBinding = this;
    }

    SceneObjHolderBinding::~SceneObjHolderBinding() {
        if (sCurrentSceneObjHolder == _holder) {
            sCurrentSceneObjHolder = nullptr;
            sCurrentSceneObjHolderBinding = nullptr;
        }
        _owned_objects.clear();
    }

    SceneObjHolder *current_scene_obj_holder() noexcept {
        return sCurrentSceneObjHolder;
    }

}  // namespace smgpc::scene

SceneObjHolder::SceneObjHolder() {
    for (auto &object : mObj) {
        object = nullptr;
    }
}

NameObj *SceneObjHolder::create(int id) {
    if (this != smgpc::scene::current_scene_obj_holder() || sCurrentSceneObjHolderBinding == nullptr ||
        id < 0 || id >= SceneObj_NumMax) {
        return nullptr;
    }

    if (mObj[id] != nullptr) {
        return mObj[id];
    }

    auto object = std::unique_ptr<NameObj>(newEachObj(id));
    if (object == nullptr) {
        return nullptr;
    }

    object->initWithoutIter();
    auto *result = object.get();
    sCurrentSceneObjHolderBinding->_owned_objects.push_back(std::move(object));
    mObj[id] = result;
    return result;
}

NameObj *SceneObjHolder::getObj(int id) const {
    if (id < 0 || id >= SceneObj_NumMax) {
        return nullptr;
    }
    return mObj[id];
}

bool SceneObjHolder::isExist(int id) const {
    return id >= 0 && id < SceneObj_NumMax && mObj[id] != nullptr;
}

NameObj *SceneObjHolder::newEachObj(int id) {
    switch (id) {
    case SceneObj_ClippingDirector:
        return new ClippingDirector();
    case SceneObj_MessageSensorHolder:
        return new MessageSensorHolder("システム汎用センサー");
    case SceneObj_StageSwitchContainer:
        return new StageSwitchContainer();
    case SceneObj_SwitchWatcherHolder:
        return new SwitchWatcherHolder();
    case SceneObj_SleepControllerHolder:
        return new SleepControllerHolder();
    case SceneObj_AreaObjContainer:
        throw std::logic_error(
            "AreaObjContainer construction is unavailable until retail managers and parsed stage placement are hosted.");
    case SceneObj_CoinHolder:
        return new CoinHolder("コイン管理");
    case SceneObj_PurpleCoinHolder:
        return new PurpleCoinHolder();
    case SceneObj_CoinRotater:
        return new CoinRotater("コイン回転管理");
    case SceneObj_MiiFacePartsHolder:
        return new MiiFacePartsHolder(128);
    case SceneObj_PrologueHolder:
        return new PrologueHolder("プロローグ保持");
    default:
        return nullptr;
    }
}

namespace MR {

    NameObj *createSceneObj(int id) {
        auto *holder = getSceneObjHolder();
        return holder != nullptr ? holder->create(id) : nullptr;
    }

    SceneObjHolder *getSceneObjHolder() {
        return smgpc::scene::current_scene_obj_holder();
    }

    bool isExistSceneObj(int id) {
        auto *holder = getSceneObjHolder();
        return holder != nullptr && holder->isExist(id);
    }

}  // namespace MR
