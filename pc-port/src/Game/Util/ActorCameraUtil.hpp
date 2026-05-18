#pragma once

#include <revolution/types.h>

class ActorCameraInfo;
class CameraTargetArg;
class LiveActor;
struct TVec3f;

namespace MR {
    void initAnimCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName);
    void startAnimCameraTargetSelf(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames, f32 speed);
    void startAnimCameraTargetOther(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, const CameraTargetArg& rTarget, s32 frames, f32 speed);
    void endAnimCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames, bool endForce);
    void initActorCameraProgrammable(const LiveActor* pActor);
    void startActorCameraProgrammable(const LiveActor* pActor, s32 frames);
    void endActorCameraProgrammable(const LiveActor* pActor, s32 frames, bool endForce);
    void setProgrammableCameraParam(const LiveActor* pActor, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec);
    void setProgrammableCameraParamFovy(const LiveActor* pActor, f32 fovy);
}  // namespace MR
