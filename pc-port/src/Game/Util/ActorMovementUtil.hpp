#pragma once

#include "JSystem/JGeometry/TVec.hpp"
#include <revolution/mtx.h>

class LiveActor;

namespace MR {
    void makeMtxTRS(MtxPtr pMtx, const LiveActor* pActor);
    void resetPosition(LiveActor* pActor);
    void resetPosition(LiveActor* pActor, const TVec3f& rPosition);
    bool isInDeath(const LiveActor* pActor, const TVec3f& rOffset);
    void calcActorAxisY(TVec3f* pOut, const LiveActor* pActor);
    bool isNearPlayer(const LiveActor* pActor, f32 distance);
    void calcVecToPlayerH(TVec3f* pOut, const LiveActor* pActor, const TVec3f* pUp);
    void attenuateVelocity(LiveActor* pActor, f32 scalar);
    void addVelocityMoveToDirection(LiveActor* pActor, const TVec3f& rDirection, f32 speed);
    void addVelocityJump(LiveActor* pActor, f32 speed);
    void addVelocityToGravity(LiveActor* pActor, f32 acceleration);
    void addVelocityToGravityOrGround(LiveActor* pActor, f32 acceleration);
    bool reboundVelocityFromCollision(LiveActor* pActor, f32 restitution = 0.0F, f32 threshold = 0.0F, f32 tangentScale = 1.0F);
    void turnDirectionDegree(const LiveActor* pActor, TVec3f* pDirection, const TVec3f& rTargetDirection, f32 degree);
    void turnDirectionToPlayerDegree(const LiveActor* pActor, TVec3f* pDirection, f32 degree);
    void zeroVelocity(LiveActor* pActor);
}  // namespace MR
