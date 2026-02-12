#pragma once

#include <cstdint>

namespace smgpc::game::title::runtime {
class NerveExecutor;
}

namespace smgpc::game::title::MR {

[[nodiscard]] bool isStep(const runtime::NerveExecutor *executor, std::int32_t step);
[[nodiscard]] bool isFirstStep(const runtime::NerveExecutor *executor);

}  // namespace smgpc::game::title::MR
