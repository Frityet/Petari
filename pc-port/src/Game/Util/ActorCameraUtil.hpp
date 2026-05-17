#pragma once

#include <revolution/types.h>

class LiveActor;
struct TVec3f;

namespace MR {
    void initActorCameraProgrammable(const LiveActor* pActor);
    void startActorCameraProgrammable(const LiveActor* pActor, s32 frames);
    void endActorCameraProgrammable(const LiveActor* pActor, s32 frames, bool endForce);
    void setProgrammableCameraParam(const LiveActor* pActor, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec);
    void setProgrammableCameraParamFovy(const LiveActor* pActor, f32 fovy);
}  // namespace MR
