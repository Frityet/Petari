#include "Game/Map/LightZoneDataHolder.hpp"

ZoneLightID::ZoneLightID() {
    _0 = -1;
    mLightID = -1;
}

void ZoneLightID::clear() {
    _0 = -1;
    mLightID = -1;
}

bool ZoneLightID::isTargetArea(const LightArea*) const {
    return false;
}

bool ZoneLightID::isOutOfArea() const {
    return mLightID < 0;
}
