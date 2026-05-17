#include "Game/Util/NerveUtil.hpp"

#include "Game/Screen/LayoutActor.hpp"
#include "Game/System/NerveExecutor.hpp"

namespace MR {

    bool isFirstStep(const NerveExecutor* pExecutor) {
        return pExecutor->getNerveStep() == 0;
    }

    bool isStep(const NerveExecutor* pExecutor, s32 step) {
        return pExecutor->getNerveStep() == step;
    }

    bool isGreaterStep(const NerveExecutor* pExecutor, s32 step) {
        return pExecutor->getNerveStep() > step;
    }

    bool isFirstStep(const LayoutActor* pActor) {
        return pActor->getNerveStep() == 0;
    }

    bool isStep(const LayoutActor* pActor, s32 step) {
        return pActor->getNerveStep() == step;
    }

    bool isGreaterStep(const LayoutActor* pActor, s32 step) {
        return pActor->getNerveStep() > step;
    }

}  // namespace MR
