#include "Game/Util/ModelUtil.hpp"
#include "Game/LiveActor/LodCtrl.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"


#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {




    const char* createSubModelObjName(const LiveActor* pActor, const char* pSubName) {
        if (pActor == nullptr || pSubName == nullptr) {
            throw std::invalid_argument("A LOD object name requires an actor and submodel name.");
        }
        const auto length = std::strlen(pActor->getName()) + std::strlen(pSubName) + std::strlen("（）") + 1U;
        auto* name = new char[length];
        std::snprintf(name, length, "%s（%s）", pActor->getName(), pSubName);
        return name;
    }

    smgpc::compat::ActorShadowRuntimeState& requireShadowControllers(LiveActor* pActor) {
        const auto* existing = smgpc::compat::actor_shadow_runtime_state(
            static_cast<const LiveActor*>(pActor));
        if (existing == nullptr || existing->controllers.empty()) {
            throw std::logic_error(
                "LOD shadow visibility synchronization is unavailable without real ShadowController ownership.");
        }
        return *smgpc::compat::actor_shadow_runtime_state(pActor);
    }

    void setShadowVisibleSyncHostAll(LiveActor* pActor, bool visibleSyncHost) {
        auto& shadow = requireShadowControllers(pActor);
        for (auto& controller : shadow.controllers) {
            controller.visible_sync_host = visibleSyncHost;
        }
    }
}  // namespace

namespace MR {

    const char* createLowModelObjName(const LiveActor* pActor) {
        return createSubModelObjName(pActor, "Low");
    }

    const char* createMiddleModelObjName(const LiveActor* pActor) {
        return createSubModelObjName(pActor, "Middle");
    }



    void copyTransRotateScale(const LiveActor* pSource, LiveActor* pDestination) {
        if (pSource == nullptr || pDestination == nullptr) {
            throw std::invalid_argument("Transform copying requires source and destination actors.");
        }
        pDestination->mPosition.set(pSource->mPosition);
        pDestination->mRotation.set(pSource->mRotation);
        pDestination->mScale.set(pSource->mScale);
    }

    f32 calcDistanceToPlayer(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("Player distance requires a LiveActor.");
        }
        const auto* player_position = MR::getPlayerPos();
        if (player_position == nullptr) {
            return std::numeric_limits<f32>::max();
        }
        return pActor->mPosition.distance(*player_position);
    }



    void hideModelAndOnCalcAnim(LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("Model hiding requires a LiveActor.");
        }
        pActor->mFlag.mIsNoCalcAnim = true;
        pActor->mFlag.mIsNoCalcView = true;
        pActor->mFlag.mIsHiddenModel = true;
        pActor->mFlag.mIsNoCalcAnim = false;
    }





    void setClippingTypeSphereContainsModelBoundingBox(LiveActor* pActor, f32 radiusOffset) {
        f32 modelBoundingRadius = 0.0f;
        MR::calcModelBoundingRadius(&modelBoundingRadius, pActor);

        setClippingTypeSphere(pActor, modelBoundingRadius + radiusOffset, nullptr);
    }

    void offShadowVisibleSyncHostAll(LiveActor* pActor) {
        setShadowVisibleSyncHostAll(pActor, false);
    }

    void onShadowVisibleSyncHostAll(LiveActor* pActor) {
        setShadowVisibleSyncHostAll(pActor, true);
    }

    LodCtrl* createLodCtrlNPC(LiveActor* pActor, const JMapInfoIter& rIter) {
        if (pActor == nullptr) {
            throw std::invalid_argument("NPC LodCtrl requires a LiveActor.");
        }

        auto lod = std::make_unique<LodCtrl>(pActor, rIter);
        lod->createLodModel(MR::DrawBufferType_NPC, MR::MovementType_NPC, -1);
        lod->syncMaterialAnimation();
        lod->syncJointAnimation();
        lod->initLightCtrl();
        lod->offSyncShadowHost();
        lod->_1B = true;

        auto* result = lod.get();
        smgpc::compat::adopt_actor_lod_ctrl(pActor, result);
        (void)lod.release();
        return result;
    }

}  // namespace MR
