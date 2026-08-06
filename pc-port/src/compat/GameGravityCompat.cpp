#include "compat/GameGravityCompat.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "scene/StageGravityService.hpp"

namespace {
    constexpr auto cNormalGravity = std::uint32_t{1U};
    constexpr auto cShadowGravity = std::uint32_t{2U};
    constexpr auto cMagnetGravity = std::uint32_t{4U};
    constexpr auto cMarioLauncherGravity = std::uint32_t{8U};

    bool query_gravity(const TVec3f& position, TVec3f* destination, std::uint32_t type_mask) {
        if (auto* service = smgpc::scene::StageGravityService::active()) {
            return service->query(position, destination, type_mask);
        }
        if (destination != nullptr) {
            destination->zero();
        }
        return false;
    }

    bool query_actor_gravity(const LiveActor* actor, TVec3f* destination, std::uint32_t type_mask) {
        if (actor == nullptr) {
            if (destination != nullptr) {
                destination->zero();
            }
            return false;
        }
        return query_gravity(actor->mPosition, destination, type_mask);
    }
}  // namespace

namespace MR {
    void registerGravity(PlanetGravity*) {
    }

    bool calcGravityVector(const LiveActor* actor, TVec3f* destination, GravityInfo*, u32) {
        return query_actor_gravity(actor, destination, cNormalGravity);
    }

    bool calcGravityVector(const NameObj*, const TVec3f& position, TVec3f* destination, GravityInfo*, u32) {
        return query_gravity(position, destination, cNormalGravity);
    }

    bool calcDropShadowVector(const LiveActor* actor, TVec3f* destination, GravityInfo*, u32) {
        return query_actor_gravity(actor, destination, cShadowGravity);
    }

    bool calcDropShadowVector(const NameObj*, const TVec3f& position, TVec3f* destination, GravityInfo*, u32) {
        return query_gravity(position, destination, cShadowGravity);
    }

    bool calcGravityAndDropShadowVector(const LiveActor* actor, TVec3f* destination, GravityInfo*, u32) {
        return query_actor_gravity(actor, destination, cNormalGravity | cShadowGravity);
    }

    bool calcGravityAndMagnetVector(const NameObj*, const TVec3f& position, TVec3f* destination,
                                    GravityInfo*, u32) {
        return query_gravity(position, destination, cNormalGravity | cMagnetGravity);
    }

    bool calcGravityVectorOrZero(const LiveActor* actor, TVec3f* destination, GravityInfo*, u32) {
        return query_actor_gravity(actor, destination, cNormalGravity);
    }

    bool calcGravityVectorOrZero(const NameObj*, const TVec3f& position, TVec3f* destination,
                                 GravityInfo*, u32) {
        return query_gravity(position, destination, cNormalGravity);
    }

    bool calcDropShadowVectorOrZero(const NameObj*, const TVec3f& position, TVec3f* destination,
                                    GravityInfo*, u32) {
        return query_gravity(position, destination, cShadowGravity);
    }

    bool calcGravityAndDropShadowVectorOrZero(const LiveActor* actor, TVec3f* destination,
                                              GravityInfo*, u32) {
        return query_actor_gravity(actor, destination, cNormalGravity | cShadowGravity);
    }

    bool calcAttractMarioLauncherOrZero(const LiveActor* actor, TVec3f* destination, GravityInfo*, u32) {
        return query_actor_gravity(actor, destination, cMarioLauncherGravity);
    }

    bool isZeroGravity(const LiveActor* actor) {
        return !query_actor_gravity(actor, nullptr, cNormalGravity);
    }

    bool isLightGravity(const GravityInfo&) {
        return false;
    }

    void settingGravityParamFromJMap(PlanetGravity*, const JMapInfoIter&) {
    }

    void getJMapInfoGravityType(const JMapInfoIter&, PlanetGravity*) {
    }

    void getJMapInfoGravityPower(const JMapInfoIter&, PlanetGravity*) {
    }

    void calcGravityOrZero(LiveActor* actor) {
        if (actor != nullptr) {
            calcGravityOrZero(actor, actor->mPosition);
        }
    }

    void calcGravityOrZero(LiveActor* actor, const TVec3f& position) {
        if (actor == nullptr) {
            return;
        }

        auto gravity = TVec3f{};
        (void)calcGravityVectorOrZero(static_cast<const NameObj*>(actor), position, &gravity, nullptr, 0U);
        if (!isNearZero(gravity, 0.001F)) {
            actor->mGravity.set(gravity);
            return;
        }

        if (actor->mBindedGround) {
            auto ground_normal = TVec3f{};
            if (!normalizeOrZero(actor->mGroundNormal, &ground_normal)) {
                actor->mGravity.set(-ground_normal);
            }
        }
    }
}  // namespace MR
