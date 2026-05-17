#pragma once

#include <revolution.h>

class Color8;
class DrawBuffer;
class LiveActor;
struct TVec3f;

namespace MR {
    void loadLight(s32);
    void initActorLightInfoLightType(LiveActor*, s32);
    void initActorLightInfoDrawBuffer(LiveActor*, DrawBuffer*);
    void requestPointLight(const LiveActor*, TVec3f, Color8, f32, s32);
    void loadLightPlayer();
}  // namespace MR
