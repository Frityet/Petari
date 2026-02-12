#pragma once

#include "JSystem/JGeometry/TVec.hpp"

class JUTTexture;

class MarineSnow {
public:
    MarineSnow();
    void view();
    void clear();
    void draw(const TVec3f&, const TVec3f&, f32) const;

    u32 mPointNum;    // 0x0
    TVec3f* mPoints;  // 0x4
    f32 mRadius;      // 0x8
    u16 mViewCount;   // 0xC
    u16 _E;
    JUTTexture* mTexture;  // 0x10
};
