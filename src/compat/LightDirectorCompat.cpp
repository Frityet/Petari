#include "Game/Map/LightDirector.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "render/light/LightData.hpp"

#include <memory>

LightDirector::LightDirector()
    : NameObj("ライト指揮"), _C(), mDataHolder(), mZoneDataHolder(), mDefaultAreaLight(), _1C(), mPointCtrl(), mResourceHolder() {
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
    mDefaultAreaLight = LightFunction::getAreaLightInfo(ZoneLightID{});
    // Retail's default coin records are independent of authored area rows.
    mDataHolder->_8.mColor = GXColor{255, 255, 0, 0};
    mDataHolder->_8.mPos.zero();
    mDataHolder->_8.mIsFollowCamera = true;
    mDataHolder->_8._14 = mDataHolder->_8._15 = mDataHolder->_8._16 = mDataHolder->_8._17 = 0;
    mDataHolder->_8._18 = 65.0f;
}

void LightDirector::loadLightPlayer() const {
    if (_1C != nullptr) {
        _1C->loadLight();
    } else if (const auto* areaLight = LightFunction::getAreaLightInfo(ZoneLightID{})) {
        LightFunction::loadActorLightInfo(&areaLight->mPlayerLight);
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
