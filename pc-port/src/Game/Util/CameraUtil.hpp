#pragma once

struct TVec3f;
struct TVec2f;

#include <revolution/types.h>

namespace MR {
    const TVec3f getCamPos();
    TVec3f getCamXdir();
    TVec3f getCamYdir();
    TVec3f getCamZdir();
    f32 getAspect();
    f32 getNearZ();
    f32 getFarZ();
    f32 getFovy();
    void resetCameraMan();
    void pauseOnCameraDirector();
    void pauseOffCameraDirector();
    void declareEventCameraProgrammable(const char* pEventName);
    void startGlobalEventCameraNoTarget(const char* pEventName, s32 frames);
    void endGlobalEventCamera(const char* pEventName, s32 frames, bool endForce);
    void setProgrammableCameraParam(const char* pEventName, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec,
                                    bool doZeroWOffset);
    void setProgrammableCameraParamFovy(const char* pEventName, f32 fovy);
    bool calcScreenPosition(TVec2f* pResult, const TVec3f& rWorldPos);
    bool calcScreenPosition(TVec3f* pResult, const TVec3f& rWorldPos);
    bool calcWorldPositionFromScreen(TVec3f* pResult, const TVec2f& rScreenPos, f32 distance);
    bool calcWorldRayDirectionFromScreen(TVec3f* pResult, const TVec2f& rScreenPos);
}  // namespace MR
