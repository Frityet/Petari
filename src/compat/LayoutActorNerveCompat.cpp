#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/LayoutUtil.hpp"

namespace MR {
    bool isStep(const LayoutActor* pActor, s32 step) {
        return pActor->getNerveStep() == step;
    }

    bool isFirstStep(const LayoutActor* pActor) {
        return isStep(pActor, 0);
    }

    bool isGreaterStep(const LayoutActor* pActor, s32 step) {
        return pActor->getNerveStep() > step;
    }
}  // namespace MR
