#pragma once

struct TVec3f;

#include <revolution/types.h>

namespace MR {
    void resetCameraMan();
    void pauseOnCameraDirector();
    void pauseOffCameraDirector();
    void declareEventCameraProgrammable(const char* pEventName);
    void startGlobalEventCameraNoTarget(const char* pEventName, s32 frames);
    void endGlobalEventCamera(const char* pEventName, s32 frames, bool endForce);
    void setProgrammableCameraParam(const char* pEventName, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec,
                                    bool doZeroWOffset);
    void setProgrammableCameraParamFovy(const char* pEventName, f32 fovy);
}  // namespace MR
