#include "Game/Scene/SceneFunction.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "runtime/RuntimeContext.hpp"

namespace MR {

    void connectToScene(NameObj* pObj, s32 movementType, s32 calcAnimType, s32 drawBufferType, s32 drawType) {
        if (pObj == nullptr) return;
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        auto* scheduler = smgpc::runtime::try_active_scene_scheduler();
        if (scheduler == nullptr && runtime != nullptr) scheduler = &runtime->scheduler();
        if (scheduler == nullptr) return;
        if (runtime != nullptr && scheduler == &runtime->scheduler()) {
            if (auto* actor = dynamic_cast< LiveActor* >(pObj); actor != nullptr && drawBufferType >= 0) {
                runtime->register_live_actor_model(*actor, movementType, calcAnimType, drawBufferType, drawType);
                return;
            }
            if (auto* layout = dynamic_cast< LayoutActor* >(pObj); layout != nullptr && drawType >= 0) {
                runtime->register_layout_actor(*layout, movementType, calcAnimType, drawType);
                return;
            }
        }
        scheduler->connect_name_obj(*pObj, movementType, calcAnimType, drawBufferType, drawType);
    }

    void disconnectToScene(NameObj* pObj) {
        if (pObj == nullptr) return;
        if (auto* scheduler = smgpc::runtime::try_active_scene_scheduler()) {
            scheduler->disconnect_name_obj(*pObj);
        } else if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->scheduler().disconnect_name_obj(*pObj);
        }
    }

}  // namespace MR
