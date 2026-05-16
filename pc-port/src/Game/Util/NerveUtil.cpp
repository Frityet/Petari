#include "Game/Util/NerveUtil.hpp"

#include "Game/System/NerveExecutor.hpp"

namespace MR {

bool isFirstStep(const NerveExecutor* pExecutor) {
    return pExecutor->getNerveStep() == 0;
}

bool isStep(const NerveExecutor* pExecutor, s32 step) {
    return pExecutor->getNerveStep() == step;
}

}  // namespace MR

