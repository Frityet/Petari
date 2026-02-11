#include "NerveUtil.hpp"

#include "NerveExecutor.hpp"

namespace smgpc::game::title::MR {

bool is_step(const runtime::NerveExecutor *executor, std::int32_t step) {
    if (executor == nullptr) {
        return false;
    }
    return executor->nerve_step() == step;
}

bool is_first_step(const runtime::NerveExecutor *executor) {
    return is_step(executor, 0);
}

}  // namespace smgpc::game::title::MR
