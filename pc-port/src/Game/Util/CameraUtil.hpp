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
}  // namespace MR
