#include "Game/LiveActor/ClippingDirector.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <stdexcept>

ClippingDirector::ClippingDirector()
    : NameObj("クリッピング指揮"), mJudge(nullptr), mActorHolder(nullptr), mGroupHolder(nullptr) {
    MR::connectToScene(this, MR::MovementType_ClippingDirector, -1, -1, -1);
}

void ClippingDirector::movement() {
    // The host scene scheduler evaluates registered actor clipping from the
    // current real camera before this retail movement category executes.
}

void ClippingDirector::endInitActorSystemInfo() {
}

void ClippingDirector::registerActor(LiveActor* pActor) {
    if (pActor == nullptr) {
        throw std::invalid_argument("ClippingDirector actor registration requires a LiveActor.");
    }

    // These are the retail ClippingActorInfo constructor values.
    smgpc::compat::configure_actor_clipping_sphere(pActor, 300.0F, nullptr);
    smgpc::compat::configure_actor_clipping_far_level(pActor, 6);
}

void ClippingDirector::initActorSystemInfo(LiveActor*, const JMapInfoIter& rIter) {
    auto viewGroupId = s32{-1};
    if (MR::getJMapInfoViewGroupID(rIter, &viewGroupId)) {
        throw std::logic_error("View-group clipping is unavailable without a real ViewGroupCtrl table.");
    }
}

void ClippingDirector::joinToGroupClipping(LiveActor*, const JMapInfoIter& rIter, int) {
    auto clippingGroupId = s32{-1};
    if (MR::getJMapInfoClippingGroupID(rIter, &clippingGroupId) && clippingGroupId >= 0) {
        throw std::logic_error("Group clipping is unavailable without a real ClippingGroupHolder.");
    }
}

void ClippingDirector::entryLodCtrl(LodCtrl*, const JMapInfoIter& rIter) {
    auto viewGroupId = s32{-1};
    if (MR::getJMapInfoViewGroupID(rIter, &viewGroupId)) {
        throw std::logic_error("LOD view-group control is unavailable without a real ViewGroupCtrl table.");
    }
}

namespace MR {

    ClippingDirector* getClippingDirector() {
        auto* holder = MR::getSceneObjHolder();
        if (holder == nullptr) {
            throw std::logic_error("ClippingDirector is unavailable without a scene-owned SceneObjHolder.");
        }

        auto* director = dynamic_cast< ClippingDirector* >(holder->getObj(SceneObj_ClippingDirector));
        if (director == nullptr) {
            throw std::logic_error("The active scene has not created its ClippingDirector.");
        }
        return director;
    }

    void addToClippingTarget(LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("Clipping target registration requires a LiveActor.");
        }
        pActor->mFlag.mIsInvalidClipping = false;
    }

    void removeFromClippingTarget(LiveActor*) {
        throw std::logic_error("Clipping target removal is unavailable without real ClippingActorHolder list ownership.");
    }

}  // namespace MR
