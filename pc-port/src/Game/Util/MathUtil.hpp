#pragma once

#include "Game/LiveActor/LiveActor.hpp"

#include <revolution/types.h>

namespace MR {
    f32 getRandom();
    f32 getRandom(f32 min, f32 max);
    s32 getRandom(s32 min, s32 max);
    f32 getRandomDegree();
    f32 repeat(f32 value, f32 min, f32 max);
    void normalize(TVec3f* pVec);
    void normalize(const TVec3f& rSrc, TVec3f* pDst);
}  // namespace MR
