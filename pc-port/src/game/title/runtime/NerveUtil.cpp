#include "NerveUtil.hpp"

#include "NerveExecutor.hpp"

namespace smgpc::game::title::MR {

bool isStep(const runtime::NerveExecutor *executor, std::int32_t step) {
    if (executor == nullptr) {
        return false;
    }
    return executor->getNerveStep() == step;
}

bool isFirstStep(const runtime::NerveExecutor *executor) {
    return isStep(executor, 0);
}

}  // namespace smgpc::game::title::MR
