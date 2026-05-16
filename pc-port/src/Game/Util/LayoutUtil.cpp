#include "Game/Util/LayoutUtil.hpp"

#include "Game/Screen/SimpleLayout.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace MR {

bool isDead(const SimpleLayout* pLayout) {
    return pLayout == nullptr || pLayout->isDead();
}

void startAnim(SimpleLayout* pLayout, const char* pAnimName, u32 animLayer) {
    pLayout->startAnim(pAnimName, animLayer);
}

bool isAnimStopped(SimpleLayout* pLayout, u32 animLayer) {
    return pLayout->isAnimStopped(animLayer);
}

void setAnimFrameAndStop(SimpleLayout* pLayout, f32 frame, u32 animLayer) {
    pLayout->setAnimFrameAndStop(frame, animLayer);
}

void setAnimFrame(SimpleLayout* pLayout, f32 frame, u32 animLayer) {
    pLayout->setAnimFrame(frame, animLayer);
}

f32 getAnimFrame(SimpleLayout* pLayout, u32 animLayer) {
    return pLayout->getAnimFrame(animLayer);
}

void setAnimRate(SimpleLayout* pLayout, f32 rate, u32 animLayer) {
    pLayout->setAnimRate(rate, animLayer);
}

void emitEffect(SimpleLayout* pLayout, const char* pEffectName) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->emit_effect(pLayout->getName(), pEffectName);
    }
}

void deleteEffectAll(SimpleLayout* pLayout) {
    if (auto *runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->delete_effect_all(pLayout->getName());
    }
}

}  // namespace MR

