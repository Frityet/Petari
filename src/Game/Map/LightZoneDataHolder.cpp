#include "Game/Map/LightZoneDataHolder.hpp"

#include "Game/AreaObj/LightArea.hpp"

ZoneLightID::ZoneLightID() {
    _0 = -1;
    mLightID = -1;
}

void ZoneLightID::clear() {
    _0 = -1;
    mLightID = -1;
}

bool ZoneLightID::isTargetArea(const LightArea* pLightArea) const {
    if (_0 == pLightArea->mPlacedZoneID && mLightID == pLightArea->mObjArg0) {
        return true;
    }

    return false;
}

bool ZoneLightID::isOutOfArea() const {
    return mLightID < 0;
}
