#pragma once

#include "compat/Types.hpp"

class LayoutActor;

namespace MR {

void startAnim(LayoutActor *pActor, const char *pAnimationName, u32 layer);
[[nodiscard]] bool isAnimStopped(const LayoutActor *pActor, u32 layer);
void setAnimFrameAndStop(LayoutActor *pActor, f32 frame, u32 layer);
void emitEffect(LayoutActor *pActor, const char *pEffectName);
void deleteEffectAll(LayoutActor *pActor);

[[nodiscard]] bool isDead(const LayoutActor *pActor);

}  // namespace MR
