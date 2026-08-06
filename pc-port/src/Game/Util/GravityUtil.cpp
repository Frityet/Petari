#include "Game/Util/GravityUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/MathUtil.hpp"

namespace {
    bool copy_actor_gravity(const NameObj* pObj, TVec3f* pDest) {
        if (pDest == nullptr) {
            return false;
        }

        pDest->zero();
        const auto* actor = dynamic_cast< const LiveActor* >(pObj);
        if (actor == nullptr) {
            return false;
        }

        return !MR::normalizeOrZero(actor->mGravity, pDest);
    }
}  // namespace

namespace MR {
    bool calcGravityVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo*, u32) {
        return copy_actor_gravity(pActor, pDest);
    }

    bool calcGravityVector(const NameObj* pObj, const TVec3f&, TVec3f* pDest, GravityInfo*, u32) {
        // A positional query cannot be answered faithfully until the host has
        // a PlanetGravityManager-equivalent registry. Do not invent a global
        // down vector: report no field and leave a deterministic zero result.
        return copy_actor_gravity(pObj, pDest);
    }

    bool calcGravityVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, u32 host) {
        return calcGravityVector(pActor, pDest, pInfo, host);
    }

    bool calcGravityVectorOrZero(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, u32 host) {
        return calcGravityVector(pObj, rPosition, pDest, pInfo, host);
    }

    void calcGravityOrZero(LiveActor* pActor) {
        if (pActor != nullptr) {
            calcGravityOrZero(pActor, pActor->mPosition);
        }
    }

    void calcGravityOrZero(LiveActor* pActor, const TVec3f& rPosition) {
        if (pActor == nullptr) {
            return;
        }

        TVec3f gravity;
        if (calcGravityVectorOrZero(static_cast< const NameObj* >(pActor), rPosition, &gravity, nullptr, 0U)) {
            pActor->mGravity.set(gravity);
            return;
        }

        if (pActor->mBindedGround) {
            TVec3f groundNormal;
            if (!normalizeOrZero(pActor->mGroundNormal, &groundNormal)) {
                pActor->mGravity.set(-groundNormal);
            }
        }
    }
}  // namespace MR
