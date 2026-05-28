#include "Game/Scene/SceneFunction.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "runtime/RuntimeContext.hpp"

namespace MR {

    void connectToScene(NameObj* pObj, s32 movementType, s32 calcAnimType, s32 drawBufferType, s32 drawType) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pObj != nullptr) {
            if (auto* actor = dynamic_cast< LiveActor* >(pObj); actor != nullptr && drawBufferType >= 0) {
                runtime->register_live_actor_model(*actor, movementType, calcAnimType, drawBufferType, drawType);
                return;
            }
            if (auto* layout = dynamic_cast< LayoutActor* >(pObj); layout != nullptr && drawType >= 0) {
                runtime->register_layout_actor(*layout, movementType, calcAnimType, drawType);
                return;
            }

            runtime->scheduler().connect_name_obj(*pObj, movementType, calcAnimType, drawBufferType, drawType);
        }
    }

    void disconnectToScene(NameObj* pObj) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pObj != nullptr) {
            runtime->scheduler().disconnect_name_obj(*pObj);
        }
    }

    void connectToSceneSky(LiveActor* pActor) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pActor != nullptr) {
            runtime->register_sky_actor(*pActor);
        }
    }

}  // namespace MR
