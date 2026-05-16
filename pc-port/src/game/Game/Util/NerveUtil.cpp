#include "Game/Util/NerveUtil.hpp"

#include "Game/Screen/LayoutActor.hpp"
#include "Game/System/NerveExecutor.hpp"

namespace MR {

bool isStep(const NerveExecutor *pExecutor, s32 step) {
    if (pExecutor == nullptr) {
        return false;
    }

    return pExecutor->getNerveStep() == step;
}

bool isStep(const LayoutActor *pActor, s32 step) {
    if (pActor == nullptr) {
        return false;
    }

    return pActor->getNerveStep() == step;
}

bool isFirstStep(const NerveExecutor *pExecutor) {
    return isStep(pExecutor, 0);
}

bool isFirstStep(const LayoutActor *pActor) {
    return isStep(pActor, 0);
}

void setNerveAtAnimStopped(LayoutActor *pActor, const Nerve *pNerve, u32 layer) {
    if (pActor != nullptr && pActor->isAnimStopped(layer)) {
        pActor->setNerve(pNerve);
    }
}

void startAnimAtFirstStep(LayoutActor *pActor, const char *pAnimName, u32 layer) {
    if (isFirstStep(pActor) && pActor != nullptr) {
        pActor->startAnim(pAnimName, layer);
    }
}

void killAtAnimStopped(LayoutActor *pActor, u32 layer) {
    if (pActor != nullptr && pActor->isAnimStopped(layer)) {
        pActor->kill();
    }
}

}  // namespace MR
