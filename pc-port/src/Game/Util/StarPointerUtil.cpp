#include "Game/Util/StarPointerUtil.hpp"

#include <revolution.h>

#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    bool isStarPointerPointingPane(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*) {
        if (!MR::isCorePadPointInScreen(WPAD_CHAN0)) {
            return false;
        }

        const auto* manager = pLayout != nullptr ? pLayout->getLayoutManager() : nullptr;
        auto pointer = TVec2f{};
        MR::getCorePadPointingPosBasedOnScreen(&pointer, WPAD_CHAN0);
        return manager != nullptr && manager->isPointingPane(pPaneName, pointer.x, pointer.y);
    }

    bool isStarPointerPointingPaneForMeterLayout(const LayoutActor* pLayout, const char* pPaneName, s32 param3, bool param4, const char* pParam5) {
        return isStarPointerPointingPane(pLayout, pPaneName, param3, param4, pParam5);
    }

    void startStarPointerModeTitle(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector started title star-pointer mode");
        }
    }

    void startStarPointerModeFileSelect(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector started file-select star-pointer mode");
        }
    }

    void requestStarPointerModeSaveLoad(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("SaveDataHandleSequence requested save/load star-pointer mode");
        }
    }

    void activeStarPointerGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector activated star-pointer guidance");
        }
    }

    void deactiveStarPointerGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("FileSelector deactivated star-pointer guidance");
        }
    }

    bool requestFileSelectGuidance() {
        return true;
    }

    bool requestFileSelectCopyGuidance() {
        return true;
    }
}  // namespace MR
