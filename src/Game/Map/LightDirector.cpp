#include "resource/TextEncoding.hpp"
#include "Game/Map/LightDirector.hpp"
#include "Game/AreaObj/LightAreaHolder.hpp"
#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/Map/LightDataHolder.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/LightPointCtrl.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include <memory>

LightDirector::LightDirector()
    : NameObj(CP932("ライト指揮")), _C(), mDataHolder(), mZoneDataHolder(), mDefaultAreaLight(), _1C(), mPointCtrl(), mResourceHolder() {
}

LightDirector::~LightDirector() {
    if (_1C != nullptr) {
        _1C->mRegisteredLightDirector = nullptr;
    }
    if (_C != nullptr) {
        _C->mRegisteredLightDirector = nullptr;
    }
    delete mPointCtrl;
    delete mZoneDataHolder;
    delete mDataHolder;
}

void LightDirector::init(const JMapInfoIter& rIter) {
    MR::connectToSceneMapObjMovement(this);
    LightFunction::loadAllLightWhite();

    auto data = std::make_unique<LightDataHolder>();
    auto zones = std::make_unique<LightZoneDataHolder>();
    auto point = std::make_unique<LightPointCtrl>();
    mDataHolder = data.release();
    mZoneDataHolder = zones.release();
    mPointCtrl = point.release();
}

void LightDirector::initData() {
    mResourceHolder = LightFunction::loadLightArchive();
    mDataHolder->initLightData();
    mZoneDataHolder->initZoneData();
    mDefaultAreaLight = mDataHolder->findAreaLight(mZoneDataHolder->getDefaultStageAreaLightName());
}

void LightDirector::loadLightPlayer() const {
    _1C->loadLight();
    mPointCtrl->loadPointLight();
}

void LightDirector::loadLightCoin() const {
    LightFunction::loadLightInfoCoin(&mDataHolder->_8);
}

void LightDirector::movement() {
    mPointCtrl->update();
}
