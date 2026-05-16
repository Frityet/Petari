#pragma once

#include <cstddef>

#include "compat/Types.hpp"

class LayoutActor;
class Nerve;

namespace MR {

    bool isExistEffectKeeper(const LayoutActor *pActor);
    void deleteEffect(LayoutActor *pActor, const char *pEffectName);
    void forceDeleteEffect(LayoutActor *pActor, const char *pEffectName);
    void setTextBoxSystemMessageRecursive(LayoutActor *pActor, const char *pPaneName, const char *pMessageName);
    void setTextBoxVerticalPositionCenterRecursive(LayoutActor *pActor, const char *pPaneName);
    void setTextBoxVerticalPositionBottomRecursive(LayoutActor *pActor, const char *pPaneName);
    void setNerveAtPaneAnimStopped(LayoutActor *pActor, const char *pPaneName, const Nerve *pNerve, u32 slot);
    void setTextBoxArgNumberRecursive(LayoutActor *pActor, const char *pPaneName, s32 arg, s32 index);
    void startCSSound(const char *name, std::nullptr_t, int arg);

}  // namespace MR
