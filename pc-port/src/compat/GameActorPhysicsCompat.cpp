#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>
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
    void resetPosition(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }
        pActor->updateHitSensors();
        pActor->calcAndSetBaseMtx();
    }

    void resetPosition(LiveActor* pActor, const TVec3f& rPosition) {
        if (pActor == nullptr) {
            return;
        }
        pActor->mPosition.set(rPosition);
        resetPosition(pActor);
    }

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

    bool isNearPlayer(const LiveActor* pActor, f32 distance) {
        return pActor != nullptr && pActor->mPosition.squareDistance(*MR::getPlayerPos()) < (distance * distance);
    }

    void calcVecToPlayerH(TVec3f* pOut, const LiveActor* pActor, const TVec3f* pUp) {
        if (pOut == nullptr || pActor == nullptr) {
            return;
        }
        pOut->set(*MR::getPlayerPos() - pActor->mPosition);
        MR::vecKillElement(*pOut, pUp != nullptr ? *pUp : pActor->mGravity, pOut);
        MR::normalizeOrZero(pOut);
    }

    void attenuateVelocity(LiveActor* pActor, f32 scalar) {
        if (pActor != nullptr) {
            pActor->mVelocity.scale(scalar);
        }
    }

    void addVelocityMoveToDirection(LiveActor* pActor, const TVec3f& rDirection, f32 speed) {
        if (pActor == nullptr) {
            return;
        }
        auto direction = rDirection;
        const auto& plane_normal = pActor->mBindedGround ? pActor->mGroundNormal : pActor->mGravity;
        MR::vecKillElement(direction, plane_normal, &direction);
        if (!MR::normalizeOrZero(&direction)) {
            pActor->mVelocity.add(direction * speed);
        }
    }

    void addVelocityJump(LiveActor* pActor, f32 speed) {
        if (pActor != nullptr) {
            pActor->mVelocity.add(pActor->mGravity * -speed);
        }
    }

    void addVelocityToGravity(LiveActor* pActor, f32 acceleration) {
        if (pActor != nullptr) {
            pActor->mVelocity.add(pActor->mGravity * acceleration);
        }
    }

    void addVelocityToGravityOrGround(LiveActor* pActor, f32 acceleration) {
        if (pActor == nullptr) {
            return;
        }
        if (pActor->mBindedGround) {
            pActor->mVelocity.add(pActor->mGroundNormal * -acceleration);
        } else {
            addVelocityToGravity(pActor, acceleration);
        }
    }

    bool reboundVelocityFromCollision(LiveActor* pActor, f32 restitution, f32 threshold, f32 tangentScale) {
        if (pActor == nullptr) {
            return false;
        }
        const TVec3f* normal = nullptr;
        if (pActor->mBindedWall) {
            normal = &pActor->mWallNormal;
        } else if (pActor->mBindedGround) {
            normal = &pActor->mGroundNormal;
        } else if (pActor->mBindedRoof) {
            normal = &pActor->mRoofNormal;
        }
        if (normal == nullptr) {
            return false;
        }

        auto unit_normal = *normal;
        if (MR::normalizeOrZero(&unit_normal)) {
            return false;
        }
        const auto hit_speed = unit_normal.dot(pActor->mVelocity);
        if (hit_speed >= 0.0F) {
            return false;
        }
        pActor->mVelocity.sub(unit_normal * hit_speed);
        if (hit_speed < -threshold) {
            pActor->mVelocity.scale(tangentScale);
            pActor->mVelocity.sub(unit_normal * hit_speed * restitution);
            return true;
        }
        return false;
    }

    void turnDirectionDegree(const LiveActor* pActor, TVec3f* pDirection, const TVec3f& rTargetDirection, f32 degree) {
        if (pActor == nullptr || pDirection == nullptr) {
            return;
        }
        auto current = *pDirection;
        auto target = rTargetDirection;
        MR::vecKillElement(current, pActor->mGravity, &current);
        MR::vecKillElement(target, pActor->mGravity, &target);
        if (MR::normalizeOrZero(&current) || MR::normalizeOrZero(&target)) {
            return;
        }
        const auto cosine = MR::clamp(current.dot(target), -1.0F, 1.0F);
        const auto angle = std::acos(cosine) * (180.0F / 3.14159265358979323846F);
        if (angle <= degree) {
            pDirection->set(target);
            return;
        }
        const auto cross = current.cross(target);
        const auto signed_degree = cross.dot(pActor->mGravity) > 0.0F ? degree : -degree;
        MR::rotateVecDegree(pDirection, current, pActor->mGravity, signed_degree);
        MR::normalizeOrZero(pDirection);
    }

    void turnDirectionToPlayerDegree(const LiveActor* pActor, TVec3f* pDirection, f32 degree) {
        if (pActor != nullptr) {
            turnDirectionDegree(pActor, pDirection, *MR::getPlayerPos() - pActor->mPosition, degree);
        }
    }

    bool checkPassBckFrame(const LiveActor* pActor, f32 frame) {
        if (pActor == nullptr) {
            return false;
        }
        const auto current = static_cast<f32>(pActor->getNerveStep() % 30);
        const auto previous = static_cast<f32>((pActor->getNerveStep() + 29) % 30);
        return previous < frame && current >= frame;
    }

    f32 calcNerveValue(const LiveActor* pActor, s32 stepMax, f32 valueStart, f32 valueEnd) {
        if (pActor == nullptr || stepMax <= 0) {
            return valueEnd;
        }
        const auto rate = MR::clamp(static_cast<f32>(pActor->getNerveStep()) / static_cast<f32>(stepMax), 0.0F, 1.0F);
        return valueStart + ((valueEnd - valueStart) * rate);
    }

    f32 calcHitPowerToWall(const LiveActor* pActor) {
        if (pActor == nullptr || !pActor->mBindedWall) {
            return 0.0F;
        }
        const auto speed = pActor->mVelocity.dot(pActor->mWallNormal);
        return speed < 0.0F ? -speed : 0.0F;
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

    void onCalcGravity(LiveActor* pActor) {
        if (pActor == nullptr) {
            return;
        }
        if (!MR::normalizeOrZero(&pActor->mGravity)) {
            pActor->mFlag.mIsCalcGravity = true;
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
