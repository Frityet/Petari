#include "Game/Map/LightDirector.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <memory>

LightDirector::LightDirector() : NameObj("ライト管理") {
}

LightDirector::~LightDirector() {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
        runtime->scene_lights().clear_light(4U);
    }
    delete mPointCtrl;
    delete mZoneDataHolder;
    delete mDataHolder;
    mPointCtrl = nullptr;
    mZoneDataHolder = nullptr;
    mDataHolder = nullptr;
    mResourceHolder = nullptr;
}

void LightDirector::init(const JMapInfoIter&) {
    MR::connectToSceneMapObjMovement(this);
    LightFunction::loadAllLightWhite();

    auto holder = std::make_unique< LightDataHolder >();
    auto zoneHolder = std::make_unique< LightZoneDataHolder >();
    auto pointCtrl = std::make_unique< LightPointCtrl >();

    mDataHolder = holder.release();
    mZoneDataHolder = zoneHolder.release();
    mPointCtrl = pointCtrl.release();
}

void LightDirector::initData() {
    mResourceHolder = LightFunction::loadLightArchive();
    if (mDataHolder != nullptr) {
        mDataHolder->initLightData();
    }
    LightFunction::initLightData();
    mDefaultAreaLight = LightFunction::getAreaLightInfo(ZoneLightID{});
}

void LightDirector::loadLightPlayer() const {
    auto loadedActorLight = false;
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
        if (const auto* playerLight = runtime->scene_lights().player_light_ctrl(); playerLight != nullptr) {
            playerLight->loadLight();
            loadedActorLight = true;
        }
    }
    if (!loadedActorLight) {
        if (const auto* areaLight = LightFunction::getAreaLightInfo(ZoneLightID{}); areaLight != nullptr) {
            LightFunction::loadActorLightInfo(&areaLight->mPlayerLight);
        }
    }
    if (mPointCtrl != nullptr) {
        mPointCtrl->loadPointLight();
    }
}

void LightDirector::loadLightCoin() const {
    if (mDataHolder != nullptr) {
        LightFunction::loadLightInfoCoin(&mDataHolder->_8);
    }
}

void LightDirector::movement() {
    if (mPointCtrl != nullptr) {
        mPointCtrl->update();
    }
}
