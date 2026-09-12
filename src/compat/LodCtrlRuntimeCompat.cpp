#include <aurora/exception.hpp>
#include "Game/Util/ModelUtil.hpp"
#include "Game/LiveActor/LodCtrl.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
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
            aurora::throw_host_exception<std::invalid_argument>("A LOD object name requires an actor and submodel name.");
        }
        // Actor and submodel names already carry Game's CP932 identity bytes.
        // Keep the original fullwidth parentheses in that same encoding.
        constexpr char brackets[] = "\x81\x69\x81\x6a";
        constexpr char format[] = "%s\x81\x69%s\x81\x6a";
        const auto length = std::strlen(pActor->getName()) + std::strlen(pSubName) + std::strlen(brackets) + 1U;
        auto* name = new char[length];
        std::snprintf(name, length, format, pActor->getName(), pSubName);
        return name;
    }

    void setShadowVisibleSyncHostAll(LiveActor* actor, bool visible) {
        if (!actor || !actor->mShadowControllerList) {
            aurora::throw_host_exception<std::logic_error>("LOD shadow visibility requires original shadow controller ownership");
        }
        for (u32 i = 0; i < actor->mShadowControllerList->getControllerCount(); ++i) {
            auto* controller = actor->mShadowControllerList->getController(i);
            if (visible) controller->onVisibleSyncHost();
            else controller->offVisibleSyncHost();
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
            aurora::throw_host_exception<std::invalid_argument>("Transform copying requires source and destination actors.");
        }
        pDestination->mPosition.set(pSource->mPosition);
        pDestination->mRotation.set(pSource->mRotation);
        pDestination->mScale.set(pSource->mScale);
    }



    void hideModelAndOnCalcAnim(LiveActor* pActor) {
        if (pActor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Model hiding requires a LiveActor.");
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
            aurora::throw_host_exception<std::invalid_argument>("NPC LodCtrl requires a LiveActor.");
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
