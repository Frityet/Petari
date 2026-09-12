#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/ActorShadowLocalUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"

// Original query and clipping bodies from ActorShadowLocalUtil/ActorShadowUtil.
// The native owner supplies the actual original controller list and projection.
namespace ActorShadow {
    u32 getShadowControllerCount(const LiveActor* pActor) {
        return pActor->mShadowControllerList->getControllerCount();
    }

    ShadowController* getShadowController(const LiveActor* pActor, u32 index) {
        return pActor->mShadowControllerList->getController(index);
    }

    ShadowController* getShadowController(const LiveActor* pActor, const char* pName) {
        return pActor->mShadowControllerList->getController(pName);
    }

    bool isExistShadowController(const LiveActor* pActor, const char* pName) {
        if (pActor->mShadowControllerList == nullptr) {
            return false;
        }

        return getShadowController(pActor, pName) != nullptr;
    }
}

namespace MR {
    bool calcClippingRangeIncludeShadow(TVec3f* pVecOutput, f32* pF32Output, const LiveActor* pActor, f32 a4) {
        TVec3f projectionPos;

        if (ActorShadow::getShadowController(pActor, static_cast< const char* >(nullptr))->isProjected()) {
            getShadowProjectionPos(pActor, static_cast< const char* >(nullptr), &projectionPos);
            pVecOutput->set((pActor->mPosition + projectionPos) * 0.5f);
            *pF32Output = projectionPos.distance(pActor->mPosition) * 0.5f + a4;
            return true;
        } else {
            pVecOutput->set(pActor->mPosition);
            *pF32Output = a4;
            return false;
        }
    }

    void setClippingRangeIncludeShadow(LiveActor* pActor, TVec3f* a2, f32 a3) {
        f32 stack_8 = a3;

        if (calcClippingRangeIncludeShadow(a2, &stack_8, pActor, a3)) {
            setClippingTypeSphere(pActor, stack_8, a2);
        } else {
            setClippingTypeSphere(pActor, a3);
        }
    }

    bool isShadowProjected(const LiveActor* pActor, const char* pName) {
        return ActorShadow::getShadowController(pActor, pName)->isProjected();
    }

    bool isShadowProjectedAny(const LiveActor* pActor) {
        u32 count = ActorShadow::getShadowControllerCount(pActor);

        for (u32 i = 0; i < count; i++) {
            if (ActorShadow::getShadowController(pActor, i)->isProjected()) {
                return true;
            }
        }

        return false;
    }

    void getShadowProjectionPos(const LiveActor* pActor, const char* pName, TVec3f* pResult) {
        ActorShadow::getShadowController(pActor, pName)->getProjectionPos(pResult);
    }

    void getShadowProjectionNormal(const LiveActor* pActor, const char* pName, TVec3f* pResult) {
        ActorShadow::getShadowController(pActor, pName)->getProjectionNormal(pResult);
    }

    f32 getShadowProjectionLength(const LiveActor* pActor, const char* pName) {
        ShadowController* pController = ActorShadow::getShadowController(pActor, pName);

        if (pController->isProjected()) {
            return pController->getProjectionLength();
        }

        return FLOAT_MAX;
    }
}
