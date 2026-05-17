#include "Game/Util/ObjUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <string>

namespace MR {
    void requestMovementOn(NameObj* pObj) {
        NameObjFunction::requestMovementOn(pObj);
    }

    void requestMovementOff(NameObj* pObj) {
        NameObjFunction::requestMovementOff(pObj);
    }

    void connectToSceneMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObj, -1);
    }

    void connectToSceneMapObjMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_MapObj, -1, -1, -1);
    }

    void connectToSceneNpc(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_NPC, -1);
    }

    void connectToSceneLayout(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_Layout, MR::CalcAnimType_Layout, MR::DrawType_Layout);
        }
    }

    void connectToSceneLayoutDecoration(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_LayoutDecoration, MR::CalcAnimType_LayoutDecoration, MR::DrawType_LayoutDecoration);
        }
    }

    void connectToSceneTalkLayout(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_Layout, MR::CalcAnimType_Layout, MR::DrawType_TalkLayout);
        }
    }

    void connectToSceneLayoutOnPause(LayoutActor* pLayout) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance(); runtime != nullptr && pLayout != nullptr) {
            runtime->register_layout_actor(*pLayout, MR::MovementType_LayoutOnPause, MR::CalcAnimType_Layout, MR::DrawType_LayoutOnPause);
        }
    }

    bool tryRumblePadStrong(const void*, s32 channel) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("SMG requested strong pad rumble on channel " + std::to_string(channel));
        }
        return true;
    }

    bool tryRumblePadWeak(const void*, s32 channel) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("SMG requested weak pad rumble on channel " + std::to_string(channel));
        }
        return true;
    }

    void shakeCameraNormal() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event("SMG requested normal camera shake");
        }
    }
}  // namespace MR
