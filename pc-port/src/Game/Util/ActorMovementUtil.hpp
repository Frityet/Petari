#pragma once

#include "JSystem/JGeometry/TVec.hpp"

class LiveActor;

namespace MR {
    bool isInDeath(const LiveActor* pActor, const TVec3f& rOffset);
    void calcActorAxisY(TVec3f* pOut, const LiveActor* pActor);
    void attenuateVelocity(LiveActor* pActor, f32 scalar);
    void zeroVelocity(LiveActor* pActor);
}  // namespace MR
