#include "Game/AreaObj/LightAreaHolder.hpp"
#include "Game/AreaObj/LightArea.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/LightZoneDataHolder.hpp"

LightAreaHolder::LightAreaHolder(s32 maxNum, const char* pName) : AreaObjMgr(maxNum, pName) {
    LightFunction::registerLightAreaHolder(this);
}

// for some reason the register movement for isTargetArea are wrong
bool LightAreaHolder::tryFindLightID(const TVec3f& rArea, ZoneLightID* pLightID) const {
    const LightArea* lightArea = static_cast< LightArea* >(find_in(rArea));

    if (lightArea == nullptr) {
        if (pLightID->isOutOfArea()) {
            pLightID->clear();
            return false;
        } else {
            pLightID->clear();
            return true;
        }
    } else {
        if (pLightID->isTargetArea(lightArea)) {
            return false;
        } else {
            pLightID->_0 = lightArea->mPlacedZoneID;
            pLightID->mLightID = lightArea->mObjArg0;
            return true;
        }
    }
}

void LightAreaHolder::initAfterPlacement() {
    sort();
}

void LightAreaHolder::sort() {
    for (s32 i = 0; i < mArray.size() - 1; i++) {
        s32 swapIndex = i;
        AreaObj* swapObj = getAreaObj(i);
        AreaObj* curObj = swapObj;
        for (s32 j = i + 1; j < mArray.size(); j++) {
            AreaObj* nextObj = getAreaObj(j);
            if (swapObj->mObjArg1 > nextObj->mObjArg1) {
                swapIndex = j;
                swapObj = nextObj;
            }
        }

        if (swapIndex != i) {
            mArray[i] = swapObj;
            mArray[swapIndex] = curObj;
        }
    }
}
