#include <aurora/allocation.hpp>
#include "resource/TextEncoding.hpp"
#include <aurora/exception.hpp>
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingJudge.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <stdexcept>

namespace {
    // These stable Game names outlive every owner that borrows their bytes.
    std::string encode_owner_name(std::string_view name) {
        aurora::allocation::HostAllocationScope host;
        return smgpc::resource::encode_cp932(name);
    }

    const std::string cClippingDirectorName = encode_owner_name("クリッピング指揮");
    const std::string cClippingJudgeName = encode_owner_name("クリッピング判定者");
}  // namespace

ClippingDirector::ClippingDirector()
    : NameObj(cClippingDirectorName.c_str()), mJudge(nullptr), mActorHolder(nullptr), mGroupHolder(nullptr) {
    mJudge = new ClippingJudge(cClippingJudgeName.c_str());
    mJudge->initWithoutIter();
    MR::connectToScene(this, MR::MovementType_ClippingDirector, -1, -1, -1);
}

void ClippingDirector::movement() {
    mJudge->movement();
}

void ClippingDirector::endInitActorSystemInfo() {
}

void ClippingDirector::registerActor(LiveActor* pActor) {
    if (pActor == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("ClippingDirector actor registration requires a LiveActor.");
    }

    // These are the retail ClippingActorInfo constructor values.
    smgpc::compat::configure_actor_clipping_sphere(pActor, 300.0F, nullptr);
    smgpc::compat::configure_actor_clipping_far_level(pActor, 6);
    smgpc::compat::set_actor_clipping_target(pActor, false);
}

void ClippingDirector::initActorSystemInfo(LiveActor*, const JMapInfoIter& rIter) {
    auto viewGroupId = s32{-1};
    // Keep authored IDs readable even though the current host has no mutable
    // ViewGroupCtrl table. Its safe initial state is the retail all-visible
    // state; scene data never gets rewritten or converted into object-specific
    // clipping policy here.
    (void)MR::getJMapInfoViewGroupID(rIter, &viewGroupId);
}

void ClippingDirector::joinToGroupClipping(LiveActor*, const JMapInfoIter& rIter, int) {
    auto clippingGroupId = s32{-1};
    if (MR::getJMapInfoClippingGroupID(rIter, &clippingGroupId) && clippingGroupId >= 0) {
        aurora::throw_host_exception<std::logic_error>("Group clipping is unavailable without a real ClippingGroupHolder.");
    }
}

void ClippingDirector::entryLodCtrl(LodCtrl* pLod, const JMapInfoIter& rIter) {
    if (pLod == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("LOD clipping registration requires a LodCtrl.");
    }
    auto viewGroupId = s32{-1};
    if (MR::getJMapInfoViewGroupID(rIter, &viewGroupId)) {
        pLod->mViewGroupID = static_cast<s16>(viewGroupId);
    }
}

namespace MR {

    ClippingDirector* getClippingDirector() {
        auto* holder = MR::getSceneObjHolder();
        if (holder == nullptr) {
            aurora::throw_host_exception<std::logic_error>("ClippingDirector is unavailable without a scene-owned SceneObjHolder.");
        }

        auto* director = dynamic_cast< ClippingDirector* >(holder->getObj(SceneObj_ClippingDirector));
        if (director == nullptr) {
            aurora::throw_host_exception<std::logic_error>("The active scene has not created its ClippingDirector.");
        }
        return director;
    }

    void addToClippingTarget(LiveActor* pActor) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Clipping target registration requires a LiveActor.");
        }
        if (!pActor->mFlag.mIsInvalidClipping) {
            smgpc::compat::set_actor_clipping_target(pActor, true);
        }
    }

    void removeFromClippingTarget(LiveActor* pActor) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Clipping target removal requires a LiveActor.");
        }
        if (!pActor->mFlag.mIsInvalidClipping) {
            smgpc::compat::set_actor_clipping_target(pActor, false);
        }
    }

}  // namespace MR
