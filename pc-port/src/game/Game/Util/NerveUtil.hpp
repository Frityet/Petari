#pragma once

#include "compat/Types.hpp"

class NerveExecutor;

namespace MR {

[[nodiscard]] bool isStep(const NerveExecutor *pExecutor, s32 step);
[[nodiscard]] bool isFirstStep(const NerveExecutor *pExecutor);

}  // namespace MR
