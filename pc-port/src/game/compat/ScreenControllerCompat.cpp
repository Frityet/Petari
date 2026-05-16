#include "compat/ScreenControllerCompat.hpp"

#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace MR {

    bool isExistEffectKeeper(const LayoutActor *pActor) {
        return pActor != nullptr;
    }

    void deleteEffect(LayoutActor *pActor, const char *) {
        deleteEffectAll(pActor);
    }

    void forceDeleteEffect(LayoutActor *pActor, const char *) {
        deleteEffectAll(pActor);
    }

    void setTextBoxSystemMessageRecursive(LayoutActor *pActor, const char *pPaneName, const char *pMessageName) {
        setTextBoxGameMessageRecursive(pActor, pPaneName, pMessageName);
    }

    void setTextBoxVerticalPositionCenterRecursive(LayoutActor *pActor, const char *pPaneName) {
        if (pActor != nullptr) {
            pActor->setTextBoxVerticalPositionRecursive(pPaneName, 1U);
        }
    }

    void setTextBoxVerticalPositionBottomRecursive(LayoutActor *pActor, const char *pPaneName) {
        if (pActor != nullptr) {
            pActor->setTextBoxVerticalPositionRecursive(pPaneName, 2U);
        }
    }

    void setNerveAtPaneAnimStopped(LayoutActor *pActor, const char *pPaneName, const Nerve *pNerve, u32 slot) {
        if (isPaneAnimStopped(pActor, pPaneName, slot)) {
            pActor->setNerve(pNerve);
        }
    }

    void startCSSound(const char *name, std::nullptr_t, int arg) {
        startCSSound(name, 0, arg);
    }

}  // namespace MR
