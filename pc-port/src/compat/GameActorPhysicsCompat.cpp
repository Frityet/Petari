#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "runtime/RuntimeContext.hpp"

#include <string>

namespace {
    s32 sCoinCount = 0;
    s32 sPurpleCoinCount = 0;
    bool sPurpleCoinCounterValid = false;

    void trace_item_count(const char* kind, s32 count) {
#ifndef NDEBUG
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->emit_semantic_trace_event("item", "count_changed", "kind=" + std::string(kind) + ";count=" + std::to_string(count));
        }
#else
        static_cast< void >(kind);
        static_cast< void >(count);
#endif
    }
}  // namespace

namespace MR {
    bool isInDeath(const LiveActor* pActor, const TVec3f& rOffset) {
        return pActor != nullptr && MR::isInAreaObj("DeathArea", pActor->mPosition + rOffset);
    }

    void calcActorAxisY(TVec3f* pOut, const LiveActor* pActor) {
        if (pOut == nullptr) {
            return;
        }
        if (pActor == nullptr) {
            pOut->set(0.0F, 1.0F, 0.0F);
            return;
        }
        const auto& matrix = pActor->getBaseMatrix().m;
        pOut->set(matrix[1], matrix[5], matrix[9]);
        if (pOut->normalize() == 0.0F) {
            pOut->set(0.0F, 1.0F, 0.0F);
        }
    }

    void attenuateVelocity(LiveActor* pActor, f32 scalar) {
        if (pActor != nullptr) {
            pActor->mVelocity.scale(scalar);
        }
    }

    void zeroVelocity(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mVelocity.zero();
        }
    }

    MirrorActor* tryCreateMirrorActor(LiveActor*, const char*) {
        return nullptr;
    }

    void setBinderExceptSensorType(LiveActor*, const TVec3f*, f32) {
    }

    void setClippingFar100m(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mClippingFarLevel = 6;
        }
    }

    bool isPressedRoofAndGround(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mBindedRoof && pActor->mBindedGround;
    }

    bool isOnGround(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mBindedGround;
    }

    bool isBindedGround(const LiveActor* pActor) {
        return isOnGround(pActor);
    }

    bool isBindedWall(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mBindedWall;
    }

    bool isBindedRoof(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mBindedRoof;
    }

    const TVec3f* getGroundNormal(const LiveActor* pActor) {
        return pActor != nullptr ? &pActor->mGroundNormal : nullptr;
    }

    const TVec3f* getWallNormal(const LiveActor* pActor) {
        return pActor != nullptr ? &pActor->mWallNormal : nullptr;
    }

    const TVec3f* getRoofNormal(const LiveActor* pActor) {
        return pActor != nullptr ? &pActor->mRoofNormal : nullptr;
    }

    bool isNoBind(const LiveActor* pActor) {
        return pActor == nullptr || pActor->mFlag.mIsNoBind;
    }

    void onBind(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsNoBind = false;
        }
    }

    void offBind(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsNoBind = true;
        }
    }

    void offCalcGravity(LiveActor* pActor) {
        if (pActor != nullptr) {
            pActor->mFlag.mIsCalcGravity = false;
        }
    }

    bool isBindedGroundDamageFire(const LiveActor* pActor) {
        return pActor != nullptr && pActor->mBindedGroundDamageFire;
    }

    void initShadowSurfaceCircle(LiveActor* pActor, f32) {
        if (pActor != nullptr) pActor->mShadowValid = true;
    }

    void initShadowVolumeSphere(LiveActor* pActor, f32) {
        if (pActor != nullptr) pActor->mShadowValid = true;
    }

    void initShadowVolumeCylinder(LiveActor* pActor, f32) {
        if (pActor != nullptr) pActor->mShadowValid = true;
    }

    void setShadowDropPositionPtr(LiveActor*, const char*, const TVec3f*) {
    }

    void setShadowDropLength(LiveActor*, const char*, f32) {
    }

    void onCalcShadow(LiveActor* pActor, const char*) {
        if (pActor != nullptr) pActor->mShadowCalcEnabled = true;
    }

    void offCalcShadow(LiveActor* pActor, const char*) {
        if (pActor != nullptr) pActor->mShadowCalcEnabled = false;
    }

    void onCalcShadowOneTime(LiveActor* pActor, const char*) {
        if (pActor != nullptr) pActor->mShadowCalcEnabled = true;
    }

    void onCalcShadowDropPrivateGravity(LiveActor* pActor, const char*) {
        if (pActor != nullptr) pActor->mShadowPrivateGravity = true;
    }

    void onCalcShadowDropPrivateGravityOneTime(LiveActor* pActor, const char*) {
        if (pActor != nullptr) pActor->mShadowPrivateGravity = true;
    }

    void invalidateShadow(LiveActor* pActor, const char*) {
        if (pActor != nullptr) pActor->mShadowValid = false;
    }

    void validateShadow(LiveActor* pActor, const char*) {
        if (pActor != nullptr) pActor->mShadowValid = true;
    }

    void setClippingRangeIncludeShadow(LiveActor* pActor, TVec3f* pCenter, f32) {
        if (pActor != nullptr && pCenter != nullptr) {
            pCenter->set(pActor->mPosition);
        }
    }

    bool isGalaxyDarkCometAppearInCurrentStage() {
        return false;
    }

    void incCoin(int amount) {
        sCoinCount += amount;
        trace_item_count("coin", sCoinCount);
    }

    void incPurpleCoin() {
        ++sPurpleCoinCount;
        trace_item_count("purple_coin", sPurpleCoinCount);
    }

    void declarePowerStarCoin100() {
    }

    void createPurpleCoinCounter() {
        sPurpleCoinCounterValid = false;
    }

    void validatePurpleCoinCounter() {
        sPurpleCoinCounterValid = true;
    }
}  // namespace MR
