#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class JUTTexture;
class WhirlPoolPoint;

class WhirlPoolAccelerator : public LiveActor {
public:
    WhirlPoolAccelerator(const char*);
    virtual ~WhirlPoolAccelerator();
    virtual void init(const JMapInfoIter&);
    virtual void movement();
    virtual void draw() const;

    bool calcInfo(const TVec3f&, TVec3f*) const;
    void initPoints();
    void drawPlane(f32, f32, f32, f32, f32, f32) const;
    void loadMaterial() const;

    f32 mRadius;                 // 0x8C
    f32 mHeight;                 // 0x90
    TVec3f mUp;                  // 0x94
    s32 mPointCount;             // 0xA0
    WhirlPoolPoint** mPoints;    // 0xA4
    f32 mRotationAngle;          // 0xA8
    f32 mTexOffset1;             // 0xAC
    f32 mTexOffset2;             // 0xB0
    JUTTexture* mTexture;        // 0xB4
    TVec3f mClippingCenter;      // 0xB8
};
