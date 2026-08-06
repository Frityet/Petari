#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution/types.h>

class ActorCameraInfo;
class CameraTargetArg;
class LiveActor;
class JMapInfoIter;

namespace MR {
    void initAnimCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName);
    bool initActorCamera(const LiveActor* pActor, const JMapInfoIter& rIter, ActorCameraInfo** pInfo);
    bool isExistActorCamera(const ActorCameraInfo* pInfo);
    bool startActorCameraTargetSelf(const LiveActor* pActor, const ActorCameraInfo* pInfo, s32 frames);
    bool endActorCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, bool endForce, s32 frames);
    void startAnimCameraTargetSelf(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames, f32 speed);
    void startAnimCameraTargetOther(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, const CameraTargetArg& rTarget, s32 frames, f32 speed);
    void endAnimCamera(const LiveActor* pActor, const ActorCameraInfo* pInfo, const char* pCameraName, s32 frames, bool endForce);
    void initActorCameraProgrammable(const LiveActor* pActor);
    void startActorCameraProgrammable(const LiveActor* pActor, s32 frames);
    void endActorCameraProgrammable(const LiveActor* pActor, s32 frames, bool endForce);
    void setProgrammableCameraParam(const LiveActor* pActor, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec);
    void setProgrammableCameraParamFovy(const LiveActor* pActor, f32 fovy);
}  // namespace MR
