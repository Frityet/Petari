#pragma once

#include <revolution/types.h>

class NerveExecutor;

namespace MR {
    bool isFirstStep(const NerveExecutor* pExecutor);
    bool isStep(const NerveExecutor* pExecutor, s32 step);
}

