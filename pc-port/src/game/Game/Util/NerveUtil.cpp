#include "Game/Util/NerveUtil.hpp"

#include "Game/System/NerveExecutor.hpp"

namespace MR {

bool isStep(const NerveExecutor *pExecutor, s32 step) {
    if (pExecutor == nullptr) {
        return false;
    }

    return pExecutor->getNerveStep() == step;
}

bool isFirstStep(const NerveExecutor *pExecutor) {
    return isStep(pExecutor, 0);
}

}  // namespace MR
