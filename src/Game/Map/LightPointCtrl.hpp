#pragma once

#include <cstdint>

#include <revolution.h>

#include "Game/LiveActor/LiveActor.hpp"

class Color8;

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
    LightPointCtrl();
    ~LightPointCtrl();

    void loadPointLight();
    void update();
    void clearPointLight(PointLightInfo*);
    void requestPointLight(const LiveActor*, TVec3f, Color8, f32, s32);

    bool tryBlendStart();
    void updatePointLight();
    void blendPointLight(PointLightInfo*, const PointLightInfo&, const PointLightInfo&, f32);
    bool isUpdateCandidateActor(const LiveActor*) const;

    s32 _0 = -1;
    s32 _4 = 30;
    const LiveActor* _8 = nullptr;
    const LiveActor* _C = nullptr;
    const LiveActor* _10 = nullptr;
    PointLightInfo* _14 = nullptr;
    PointLightInfo* _18 = nullptr;
    PointLightInfo* _1C = nullptr;

    // Native pointers are generation-qualified on the host. This preserves
    // the retail identity comparisons while rejecting a destroyed actor and
    // pointer-address reuse before any candidate is dereferenced.
    std::uint64_t _8Generation = 0U;
    std::uint64_t _CGeneration = 0U;
    std::uint64_t _10Generation = 0U;
};
