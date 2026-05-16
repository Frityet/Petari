#pragma once

#include "compat/Types.hpp"

class NerveExecutor;
class LayoutActor;
class Nerve;

namespace MR {

[[nodiscard]] bool isStep(const NerveExecutor *pExecutor, s32 step);
[[nodiscard]] bool isStep(const LayoutActor *pActor, s32 step);
[[nodiscard]] bool isFirstStep(const NerveExecutor *pExecutor);
[[nodiscard]] bool isFirstStep(const LayoutActor *pActor);
void setNerveAtAnimStopped(LayoutActor *pActor, const Nerve *pNerve, u32 layer);
void startAnimAtFirstStep(LayoutActor *pActor, const char *pAnimName, u32 layer);
void killAtAnimStopped(LayoutActor *pActor, u32 layer);

}  // namespace MR
