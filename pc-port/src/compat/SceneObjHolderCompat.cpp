#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"

#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/MapObj/CoinHolder.hpp"
#include "Game/MapObj/CoinRotater.hpp"
#include "Game/MapObj/PurpleCoinHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Screen/CenterScreenBlur.hpp"
#include "Game/Screen/InformationObserver.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "compat/CapturedFrameBlurService.hpp"
#include "compat/GlobalGravityOwnership.hpp"
#include "compat/TalkRuntime.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace {

    SceneObjHolder *sCurrentSceneObjHolder = nullptr;
    smgpc::scene::SceneObjHolderBinding *sCurrentSceneObjHolderBinding = nullptr;

}  // namespace

namespace smgpc::scene {

    SceneObjHolderBinding::SceneObjHolderBinding(SceneObjHolder &holder)
        : _holder(&holder), _owned_objects(),
          _global_gravity_ownership(
              std::make_unique<smgpc::compat::GlobalGravityOwnership>(holder)),
          _area_obj_runtime(std::make_unique<AreaObjRuntime>()),
          _captured_frame_blur_service(std::make_unique<smgpc::compat::CapturedFrameBlurService>()) {
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
        // SceneObj dependency order is creation order. Retire each exact
        // object in reverse while the holder is already unavailable, which
        // also prevents destructor-time recreation through MR::createSceneObj.
        while (!_owned_objects.empty()) {
            _owned_objects.pop_back();
        }
        // The external holder storage outlives this binding in test and scene
        // hosts. Reconstruct its exact empty value so no slot retains a freed
        // SceneObj and a later generation can bind/recreate normally.
        *_holder = SceneObjHolder{};
        // PlanetGravityManager and BaseMatrixFollowTargetHolder retain raw
        // pointers into the retail scene heap. Only reclaim their registered
        // children after both SceneObjs have retired.
        _global_gravity_ownership->reclaim();
        _global_gravity_ownership.reset();
        _area_obj_runtime.reset();
        _captured_frame_blur_service.reset();
    }

    void SceneObjHolderBinding::init_after_placement() {
        for (std::size_t index = 0; index < _owned_objects.size(); ++index) {
            _owned_objects[index]->initAfterPlacement();
        }
        _area_obj_runtime->init_after_placement();
    }

    SceneObjHolder *current_scene_obj_holder() noexcept {
        return sCurrentSceneObjHolder;
    }

    AreaObjRuntime *current_area_obj_runtime() noexcept {
        return sCurrentSceneObjHolderBinding != nullptr ? sCurrentSceneObjHolderBinding->_area_obj_runtime.get() : nullptr;
    }

    smgpc::compat::CapturedFrameBlurService *current_captured_frame_blur_service() noexcept {
        return sCurrentSceneObjHolderBinding != nullptr
                   ? sCurrentSceneObjHolderBinding->_captured_frame_blur_service.get()
                   : nullptr;
    }

    smgpc::compat::GlobalGravityOwnership *
    current_global_gravity_ownership() noexcept {
        return sCurrentSceneObjHolderBinding != nullptr ?
                   sCurrentSceneObjHolderBinding->_global_gravity_ownership.get() :
                   nullptr;
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
    case SceneObj_PlanetGravityManager:
        return new PlanetGravityManager("重力");
    case SceneObj_BaseMatrixFollowTargetHolder:
        return new BaseMatrixFollowTargetHolder("行列追随先リスト", 256, 256);
    case SceneObj_MessageSensorHolder:
        return new MessageSensorHolder("システム汎用センサー");
    case SceneObj_StageSwitchContainer:
        return new StageSwitchContainer();
    case SceneObj_SwitchWatcherHolder:
        return new SwitchWatcherHolder();
    case SceneObj_SleepControllerHolder:
        return new SleepControllerHolder();
    case SceneObj_AreaObjContainer:
        return new AreaObjContainer("エリアオブジェクトコンテナ管理");
    case SceneObj_PlacementStateChecker:
        return new PlacementStateChecker("オブジェクト配置状態の監視");
    case SceneObj_MarioHolder:
        return new MarioHolder();
    case SceneObj_CoinHolder:
        return new CoinHolder("コイン管理");
    case SceneObj_PurpleCoinHolder:
        return new PurpleCoinHolder();
    case SceneObj_CoinRotater:
        return new CoinRotater("コイン回転管理");
    case SceneObj_PrologueHolder:
        return new PrologueHolder("プロローグ保持");
    case SceneObj_CenterScreenBlur:
        return new CenterScreenBlur();
    case SceneObj_InformationObserver:
        return new InformationObserver();
    case SceneObj_TalkDirector:
        return new smgpc::compat::TalkRuntime();
    case SceneObj_LensFlareDirector:
        return new LensFlareDirector();
    case SceneObj_SphereSelector:
        return new SphereSelector();
    case SceneObj_GroupCheckManager:
        return new GroupCheckManager("属性グループマネージャー");
    case SceneObj_PriorDrawAirHolder:
        return new PriorDrawAirHolder();
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
