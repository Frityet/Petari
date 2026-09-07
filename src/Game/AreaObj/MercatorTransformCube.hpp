#pragma once

#include "JSystem/JGeometry/TVec.hpp"

#include <revolution/types.h>

class LiveActor;

class DivideMercatorRailPosInfo {
public:
    inline DivideMercatorRailPosInfo() {
    }

    virtual void setPosition(s32, const TVec3f&) = 0;
};

namespace MR {
    void getDivideMercatorRailPosition(DivideMercatorRailPosInfo*, const LiveActor*, u32, f32, u32);
}  // namespace MR
