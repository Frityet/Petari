#pragma once

#include <revolution/types.h>

class SimpleLayout;

namespace MR {
    bool isDead(const SimpleLayout* pLayout);
    void startAnim(SimpleLayout* pLayout, const char* pAnimName, u32 animLayer);
    bool isAnimStopped(SimpleLayout* pLayout, u32 animLayer);
    void setAnimFrameAndStop(SimpleLayout* pLayout, f32 frame, u32 animLayer);
    void setAnimFrame(SimpleLayout* pLayout, f32 frame, u32 animLayer);
    f32 getAnimFrame(SimpleLayout* pLayout, u32 animLayer);
    void setAnimRate(SimpleLayout* pLayout, f32 rate, u32 animLayer);
    void emitEffect(SimpleLayout* pLayout, const char* pEffectName);
    void deleteEffectAll(SimpleLayout* pLayout);
}

