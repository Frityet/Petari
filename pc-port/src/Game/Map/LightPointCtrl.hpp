#pragma once

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"

class PointLightInfo {
public:
    void operator=(const PointLightInfo& other) {
        mPosition = other.mPosition;
        mColor = other.mColor;
        mRadius = other.mRadius;
        mBrightness = other.mBrightness;
        mDistAttnFn = other.mDistAttnFn;
    }

    TVec3f mPosition;  // 0x0
    _GXColor mColor;   // 0xC
    f32 mRadius = 0.0F;
    f32 mBrightness = 0.0F;
    u32 mDistAttnFn = 0U;
};

class LightPointCtrl {
public:
    LightPointCtrl() = default;

    void loadPointLight() {}
    void update() {}
    void clearPointLight(PointLightInfo*) {}

    bool tryBlendStart() {
        return false;
    }

    void updatePointLight() {}

    s32 _0 = 0;
    u32 _4 = 0U;
    u32 _8 = 0U;
    u32 _C = 0U;
    u32 _10 = 0U;
    PointLightInfo* _14 = nullptr;
    PointLightInfo* _18 = nullptr;
    PointLightInfo* _1C = nullptr;
};
