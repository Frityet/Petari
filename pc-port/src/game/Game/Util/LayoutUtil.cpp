#include "Game/Util/LayoutUtil.hpp"

#include "Game/Screen/LayoutActor.hpp"
#include "compat/DecompIntegration.hpp"

namespace {

// SMGPC_INTEGRATION_BEGIN
SMGPC_STUB(src/Game/Effect/MultiEmitter.cpp);
// SMGPC_INTEGRATION_END

}  // namespace

namespace MR {

void startAnim(LayoutActor *pActor, const char *pAnimationName, u32 layer) {
    if (pActor == nullptr || pAnimationName == nullptr) {
        return;
    }

    pActor->startAnim(pAnimationName, layer);
}

bool isAnimStopped(const LayoutActor *pActor, u32 layer) {
    if (pActor == nullptr) {
        return true;
    }

    return pActor->isAnimStopped(layer);
}

void setAnimFrameAndStop(LayoutActor *pActor, f32 frame, u32 layer) {
    if (pActor == nullptr) {
        return;
    }

    pActor->setAnimFrameAndStop(frame, layer);
}

void emitEffect(LayoutActor *pActor, const char *pEffectName) {
    if (pActor == nullptr) {
        return;
    }

    pActor->emitEffect(pEffectName);
}

void deleteEffectAll(LayoutActor *pActor) {
    if (pActor == nullptr) {
        return;
    }

    pActor->deleteEffectAll();
}

bool isDead(const LayoutActor *pActor) {
    if (pActor == nullptr) {
        return true;
    }

    return pActor->isDead();
}

}  // namespace MR
