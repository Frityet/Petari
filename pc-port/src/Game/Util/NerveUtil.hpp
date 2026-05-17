#pragma once

#include <revolution/types.h>

class NerveExecutor;
class LayoutActor;

namespace MR {
    bool isFirstStep(const NerveExecutor* pExecutor);
    bool isStep(const NerveExecutor* pExecutor, s32 step);
    bool isGreaterStep(const NerveExecutor* pExecutor, s32 step);
    bool isFirstStep(const LayoutActor* pActor);
    bool isStep(const LayoutActor* pActor, s32 step);
    bool isGreaterStep(const LayoutActor* pActor, s32 step);
}  // namespace MR
