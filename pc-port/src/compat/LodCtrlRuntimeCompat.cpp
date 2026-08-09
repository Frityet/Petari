#include "Game/LiveActor/LodCtrl.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/live_actor/LiveActorModel.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
    const smgpc::render::live_actor::LiveActorModel& requireModel(const LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("A model operation requires a LiveActor.");
        }
        const auto* model = smgpc::compat::actor_model(pActor);
        if (model == nullptr || model->model_arc_name().empty()) {
            throw std::logic_error("The LiveActor has no real model resource.");
        }
        return *model;
    }

    smgpc::render::live_actor::LiveActorModel& requireModel(LiveActor* pActor) {
        return const_cast< smgpc::render::live_actor::LiveActorModel& >(requireModel(const_cast< const LiveActor* >(pActor)));
    }

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

    const char* getModelResName(const LiveActor* pActor) {
        return requireModel(pActor).model_arc_name().data();
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

    void calcAnimDirect(LiveActor* pActor) {
        if (pActor == nullptr) {
            throw std::invalid_argument("Direct animation calculation requires a LiveActor.");
        }
        const auto wasDisabled = pActor->mFlag.mIsNoCalcAnim;
        pActor->mFlag.mIsNoCalcAnim = false;
        pActor->calcAnim();
        pActor->mFlag.mIsNoCalcAnim = wasDisabled;
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

    void syncJointAnimation(LiveActor* pDestination, const LiveActor* pSource) {
        requireModel(pDestination).syncJointAnimationFrom(requireModel(pSource));
    }

    void syncMaterialAnimation(LiveActor* pDestination, const LiveActor* pSource) {
        requireModel(pDestination).syncMaterialAnimationFrom(requireModel(pSource));
    }

    void setClippingTypeSphereContainsModelBoundingBox(LiveActor* pActor, f32 margin) {
        if (!std::isfinite(margin)) {
            throw std::invalid_argument("A clipping margin must be finite.");
        }
        const auto radius = requireModel(pActor).model_bounding_radius();
        if (!radius.has_value()) {
            throw std::logic_error("Model clipping bounds are unavailable without a parsed real BDL/BMD model.");
        }
        smgpc::compat::configure_actor_clipping_sphere(pActor, *radius + margin, nullptr);
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
