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
            runtime->star_pointer().start_mode(smgpc::game::StarPointerMode::Title);
        }
    }

    void startStarPointerModeFileSelect(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().start_mode(smgpc::game::StarPointerMode::FileSelect);
        }
    }

    void requestStarPointerModeSaveLoad(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().start_mode(smgpc::game::StarPointerMode::SaveLoad);
        }
    }

    void requestStarPointerModePictureBook(void*) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().start_mode(smgpc::game::StarPointerMode::PictureBook);
        }
    }

    void activeStarPointerGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().set_guidance_active(true);
        }
    }

    void deactiveStarPointerGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().set_guidance_active(false);
        }
    }

    bool requestFileSelectGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().request_file_select_guidance();
        }
        return true;
    }

    bool requestFileSelectCopyGuidance() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->star_pointer().request_file_select_copy_guidance();
        }
        return true;
    }
}  // namespace MR
