#pragma once

#include <JSystem/JGeometry/TUtil.hpp>
#include <JSystem/JGeometry/TVec.hpp>
#include <cmath>
#include <revolution/types.h>

namespace MR {
    f32 getRandom();
    f32 getRandom(f32 min, f32 max);
    s32 getRandom(s32 min, s32 max);
    f32 getRandomDegree();
    void getRandomVector(TVec3f* pDst, f32 range);
    void addRandomVector(TVec3f* pDst, const TVec3f& rSrc, f32 range);
    f32 repeat(f32 value, f32 min, f32 max);
    f32 mod(f32 x, f32 y);
    bool isNearZero(f32 x, f32 tolerance = 0.001f);
    bool isNearZero(const TVec3f& rVec, f32 tolerance = 0.001f);
    void makeAxisVerticalZX(TVec3f* pDst, const TVec3f& rAxis);
    bool calcReboundVelocity(TVec3f* pVelocity, const TVec3f& rNormal, f32 restitution);
    bool calcReboundVelocity(TVec3f* pVelocity, const TVec3f& rNormal, f32 restitution, f32 tangentScale);
    void rotateVecDegree(TVec3f* pDst, const TVec3f& rAxis, f32 degree);
    void rotateVecDegree(TVec3f* pDst, const TVec3f& rSrc, const TVec3f& rAxis, f32 degree);
    void normalize(TVec3f* pVec);
    void normalize(const TVec3f& rSrc, TVec3f* pDst);
    bool normalizeOrZero(TVec3f* pVec);
    bool normalizeOrZero(const TVec3f& rSrc, TVec3f* pDst);

    inline f32 abs(f32 x) {
        return std::fabs(x);
    }

    inline f32 clamp(f32 x, f32 min, f32 max) {
        return JGeometry::TUtil< f32 >::clamp(x, min, max);
    }
}  // namespace MR
