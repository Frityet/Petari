#include "Game/Map/LightDataHolder.hpp"

#include <cstring>

namespace {
    static const char* sDefaultAreaLightName = "デフォルト";
}

LightDataHolder::LightDataHolder() {
    mLightCount = 0;
    mLights = nullptr;
}

void LightDataHolder::initLightData() {
}

AreaLightInfo* LightDataHolder::findAreaLight(const char* pName) const {
    if (mLights == nullptr || mLightCount <= 0) {
        return nullptr;
    }

    for (s32 i = 0; i < mLightCount; i++) {
        AreaLightInfo* info = &mLights[i];

        if (pName != nullptr && info->mAreaLightName != nullptr && std::strcmp(pName, info->mAreaLightName) == 0) {
            return info;
        }
    }

    return mLights;
}

const char* LightDataHolder::getDefaultAreaLightName() const {
    return sDefaultAreaLightName;
}

s32 LightDataHolder::getDefaultStepInterpolate() const {
    return 0x1E;
}
