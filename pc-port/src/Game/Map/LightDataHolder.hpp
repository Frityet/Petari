#pragma once

#include <cstring>

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"

class LightInfo {
public:
    _GXColor mColor;       // 0x0
    TVec3f mPos;           // 0x4
    bool mIsFollowCamera;  // 0x10
};

class LightInfoCoin : public LightInfo {
public:
    u8 _14 = 0U;
    u8 _15 = 0U;
    u8 _16 = 0U;
    u8 _17 = 0U;
    f32 _18 = 0.0F;
};

class ActorLightInfo {
public:
    void operator=(const ActorLightInfo& other) {
        mInfo0 = other.mInfo0;
        mInfo1 = other.mInfo1;
        mAlpha2 = other.mAlpha2;
        mColor = other.mColor;
    }

    LightInfo mInfo0;  // 0x0
    LightInfo mInfo1;  // 0x14
    u8 mAlpha2 = 0U;   // 0x28
    _GXColor mColor;   // 0x29
};

struct AreaLightInfo {
    const char* mAreaLightName = nullptr;  // 0x0
    s32 mInterpolate = -1;                 // 0x4
    bool mFix = false;                     // 0x8
    ActorLightInfo mPlayerLight;           // 0xC
    ActorLightInfo mStrongLight;           // 0x3C
    ActorLightInfo mWeakLight;             // 0x6C
    ActorLightInfo mPlanetLight;           // 0x9C
};

class LightDataHolder {
public:
    LightDataHolder() = default;

    void initLightData() {}

    AreaLightInfo* findAreaLight(const char* pName) const {
        if (mLights == nullptr || mLightCount <= 0) {
            return nullptr;
        }

        for (auto i = 0; i < mLightCount; ++i) {
            auto* info = &mLights[i];
            if (pName != nullptr && info->mAreaLightName != nullptr && std::strcmp(pName, info->mAreaLightName) == 0) {
                return info;
            }
        }

        return mLights;
    }

    const char* getDefaultAreaLightName() const {
        return mLights != nullptr && mLightCount > 0 ? mLights[0].mAreaLightName : nullptr;
    }

    s32 getDefaultStepInterpolate() const {
        return 0x1E;
    }

    s32 mLightCount = 0;          // 0x0
    AreaLightInfo* mLights = {};  // 0x4
    LightInfoCoin _8;
};
