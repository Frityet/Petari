#pragma once

#include <cstdint>

namespace smgpc::game::title::runtime {
class NerveExecutor;
}

namespace smgpc::game::title::MR {

[[nodiscard]] bool is_step(const runtime::NerveExecutor *executor, std::int32_t step);
[[nodiscard]] bool is_first_step(const runtime::NerveExecutor *executor);

}  // namespace smgpc::game::title::MR
