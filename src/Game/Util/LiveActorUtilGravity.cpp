#include "Game/Util/LiveActorUtil.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/MathUtil.hpp"

namespace MR {
    void calcGravity(LiveActor* pActor) {
        TVec3f gravity;
        calcGravityVector(pActor, pActor->mPosition, &gravity, nullptr, 0);

        if (!isNearZero(gravity)) {
            pActor->mGravity.set(gravity);
        }
    }

    void calcGravity(LiveActor* pActor, const TVec3f& rPos) {
        TVec3f gravity;
        calcGravityVector(pActor, rPos, &gravity, nullptr, 0);

        if (!isNearZero(gravity)) {
            pActor->mGravity.set(gravity);
        }
    }

    void calcGravityOrZero(LiveActor* pActor) {
        calcGravityOrZero(pActor, pActor->mPosition);
    }

    void calcGravityOrZero(LiveActor* pActor, const TVec3f& rPos) {
        TVec3f gravity;
        calcGravityVectorOrZero(pActor, rPos, &gravity, nullptr, 0);

        if (!isNearZero(gravity, 0.001f)) {
            pActor->mGravity.set(gravity);
            return;
        }

        if (isBindedGround(pActor)) {
            const TVec3f* normal = pActor->mBinder->mGroundInfo.mParentTriangle.getNormal(0);
            pActor->mGravity.set(-*normal);
        }
    }

};  // namespace MR
