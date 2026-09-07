#include "compat/GameGravityCompat.hpp"

#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace {
    [[nodiscard]] PlanetGravityManager& gravity_manager() {
        auto* holder = MR::getSceneObjHolder();
        if (holder == nullptr) {
            throw std::logic_error("PlanetGravity is unavailable without a scene-owned SceneObjHolder");
        }

        auto* manager = static_cast<PlanetGravityManager*>(
            holder->getObj(SceneObj_PlanetGravityManager));
        if (manager == nullptr) {
            throw std::logic_error("PlanetGravity is unavailable without the scene-owned PlanetGravityManager");
        }
        return *manager;
    }

    [[nodiscard]] u32 host_id(const void* object, u32 host) {
        if (host != 0U) {
            return host;
        }
        return static_cast<u32>(reinterpret_cast<std::uintptr_t>(object));
    }

    bool query_gravity(const NameObj* object, const TVec3f& position, TVec3f* destination,
                       GravityInfo* info, u32 host, u32 type_mask) {
        if (object == nullptr) {
            throw std::invalid_argument("gravity queries require a real requesting NameObj");
        }
        return gravity_manager().calcTotalGravityVector(
            destination, info, position, type_mask, host_id(object, host));
    }

    bool query_actor_gravity(const LiveActor* actor, TVec3f* destination, GravityInfo* info,
                             u32 host, u32 type_mask) {
        if (actor == nullptr) {
            throw std::invalid_argument("gravity queries require a real LiveActor");
        }
        return query_gravity(actor, actor->mPosition, destination, info, host, type_mask);
    }

    void get_jmap_arg_plus(const JMapInfoIter& iter, const char* field_name, f32* destination) {
        auto result = f32{};
        if (iter.getValue(field_name, &result) && result >= 0.0F) {
            *destination = result;
        }
    }

    void get_jmap_arg_plus(const JMapInfoIter& iter, const char* field_name, s32* destination) {
        auto result = s32{};
        if (iter.getValue(field_name, &result) && result >= 0) {
            *destination = result;
        }
    }
}  // namespace

namespace MR {
    void registerGravity(PlanetGravity* gravity) {
        if (gravity == nullptr) {
            throw std::invalid_argument("cannot register a null PlanetGravity");
        }
        auto& manager = gravity_manager();
        if (gravity->mIsRegistered) {
            throw std::logic_error("PlanetGravity is already registered");
        }
        manager.registerGravity(gravity);
    }

    bool calcGravityVector(const LiveActor* actor, TVec3f* destination, GravityInfo* info, u32 host) {
        return query_actor_gravity(actor, destination, info, host, GRAVITY_TYPE_NORMAL);
    }

    bool calcGravityVector(const NameObj* object, const TVec3f& position, TVec3f* destination,
                           GravityInfo* info, u32 host) {
        return query_gravity(object, position, destination, info, host, GRAVITY_TYPE_NORMAL);
    }

    bool calcDropShadowVector(const LiveActor* actor, TVec3f* destination, GravityInfo* info, u32 host) {
        return query_actor_gravity(actor, destination, info, host, GRAVITY_TYPE_SHADOW);
    }

    bool calcDropShadowVector(const NameObj* object, const TVec3f& position, TVec3f* destination,
                              GravityInfo* info, u32 host) {
        return query_gravity(object, position, destination, info, host, GRAVITY_TYPE_SHADOW);
    }

    bool calcGravityAndDropShadowVector(const LiveActor* actor, TVec3f* destination,
                                        GravityInfo* info, u32 host) {
        return query_actor_gravity(actor, destination, info, host,
                                   GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_SHADOW);
    }

    bool calcGravityAndMagnetVector(const NameObj* object, const TVec3f& position,
                                    TVec3f* destination, GravityInfo* info, u32 host) {
        return query_gravity(object, position, destination, info, host,
                             GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_MAGNET);
    }

    bool calcGravityVectorOrZero(const LiveActor* actor, TVec3f* destination,
                                 GravityInfo* info, u32 host) {
        return query_actor_gravity(actor, destination, info, host, GRAVITY_TYPE_NORMAL);
    }

    bool calcGravityVectorOrZero(const NameObj* object, const TVec3f& position,
                                 TVec3f* destination, GravityInfo* info, u32 host) {
        return query_gravity(object, position, destination, info, host, GRAVITY_TYPE_NORMAL);
    }

    bool calcDropShadowVectorOrZero(const NameObj* object, const TVec3f& position,
                                    TVec3f* destination, GravityInfo* info, u32 host) {
        return query_gravity(object, position, destination, info, host, GRAVITY_TYPE_SHADOW);
    }

    bool calcGravityAndDropShadowVectorOrZero(const LiveActor* actor, TVec3f* destination,
                                              GravityInfo* info, u32 host) {
        return query_actor_gravity(actor, destination, info, host,
                                   GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_SHADOW);
    }

    bool calcAttractMarioLauncherOrZero(const LiveActor* actor, TVec3f* destination,
                                        GravityInfo* info, u32 host) {
        return query_actor_gravity(actor, destination, info, host, GRAVITY_TYPE_MARIO_LAUNCHER);
    }

    bool isZeroGravity(const LiveActor* actor) {
        auto gravity = TVec3f{};
        return !query_actor_gravity(actor, &gravity, nullptr, 0U, GRAVITY_TYPE_NORMAL);
    }

    bool isLightGravity(const GravityInfo& info) {
        return info.mGravityInstance != nullptr &&
               info.mGravityInstance->mGravityPower == GRAVITY_POWER_LIGHT;
    }

    void settingGravityParamFromJMap(PlanetGravity* gravity, const JMapInfoIter& iter) {
        if (gravity == nullptr) {
            throw std::invalid_argument("gravity JMap parameters require a real PlanetGravity");
        }

        auto range = gravity->mRange;
        get_jmap_arg_plus(iter, "Range", &range);
        gravity->mRange = range;

        auto distant = gravity->mDistant;
        get_jmap_arg_plus(iter, "Distant", &distant);
        gravity->mDistant = distant;

        auto priority = gravity->mPriority;
        get_jmap_arg_plus(iter, "Priority", &priority);
        gravity->setPriority(priority);

        auto id = gravity->mGravityId;
        get_jmap_arg_plus(iter, "Gravity_id", &id);
        gravity->mGravityId = id;

        getJMapInfoGravityType(iter, gravity);
        getJMapInfoGravityPower(iter, gravity);

        auto inverse = static_cast<s32>(gravity->mIsInverse != false);
        get_jmap_arg_plus(iter, "Inverse", &inverse);
        gravity->mIsInverse = inverse != 0;
    }

    void getJMapInfoGravityType(const JMapInfoIter& iter, PlanetGravity* gravity) {
        if (gravity == nullptr) {
            throw std::invalid_argument("gravity type parsing requires a real PlanetGravity");
        }

        const char* type = nullptr;
        if (iter.getValue("Gravity_type", &type) && type != nullptr) {
            if (std::strcmp(type, "Normal") == 0) {
                gravity->mGravityType = GRAVITY_TYPE_NORMAL;
            } else if (std::strcmp(type, "Shadow") == 0) {
                gravity->mGravityType = GRAVITY_TYPE_SHADOW;
            } else if (std::strcmp(type, "Magnet") == 0) {
                gravity->mGravityType = GRAVITY_TYPE_MAGNET;
            }
        }
    }

    void getJMapInfoGravityPower(const JMapInfoIter& iter, PlanetGravity* gravity) {
        if (gravity == nullptr) {
            throw std::invalid_argument("gravity power parsing requires a real PlanetGravity");
        }

        const char* power = nullptr;
        if (iter.getValue("Power", &power) && power != nullptr) {
            if (std::strcmp(power, "Light") == 0) {
                gravity->mGravityPower = GRAVITY_POWER_LIGHT;
            } else if (std::strcmp(power, "Normal") == 0) {
                gravity->mGravityPower = GRAVITY_POWER_NORMAL;
            } else if (std::strcmp(power, "Heavy") == 0) {
                gravity->mGravityPower = GRAVITY_POWER_HEAVY;
            }
        }
    }

    void calcGravityOrZero(LiveActor* actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("calcGravityOrZero requires a real LiveActor");
        }
        calcGravityOrZero(actor, actor->mPosition);
    }

    void calcGravityOrZero(LiveActor* actor, const TVec3f& position) {
        if (actor == nullptr) {
            throw std::invalid_argument("calcGravityOrZero requires a real LiveActor");
        }

        auto gravity = TVec3f{};
        (void)calcGravityVectorOrZero(static_cast<const NameObj*>(actor), position, &gravity, nullptr, 0U);
        if (!isNearZero(gravity, 0.001F)) {
            actor->mGravity.set(gravity);
            return;
        }

        const auto* contacts = smgpc::compat::actor_binder_contacts(actor);
        if (contacts != nullptr && contacts->ground) {
            auto ground_normal = TVec3f{};
            if (!normalizeOrZero(contacts->ground_normal, &ground_normal)) {
                actor->mGravity.set(-ground_normal);
            }
        }
    }
}  // namespace MR
