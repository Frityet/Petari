#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "camera/CameraPose.hpp"
#include "compat/ActorPhysicsRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    class ProbeActor final : public LiveActor {
    public:
        ProbeActor() : LiveActor("ActorPhysicsProbe") {
        }

        void calcAnim() override {
            ++calc_anim_count;
            LiveActor::calcAnim();
        }

        int calc_anim_count = 0;
    };

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_unavailable(const std::function< void() >& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }

    void require_near(float actual, float expected, std::string_view message) {
        require(std::abs(actual - expected) < 0.0001F, message);
    }
}  // namespace

int main() {
    auto passed = 0;

    require_unavailable([] { MR::incCoin(1); },
                        "coin collection must not update a process-global substitute for ScenePlayingResult");
    require_unavailable([] { MR::incPurpleCoin(); },
                        "Purple Coin collection must not update a process-global substitute for ScenePlayingResult");
    ++passed;

    require_unavailable([] { MR::declarePowerStarCoin100(); },
                        "the 100-Coin Power Star must not report a declaration without EventPowerStar machinery");
    require_unavailable([] { (void)MR::isGalaxyDarkCometAppearInCurrentStage(); },
                        "an absent current-stage comet state must not become false");
    ++passed;

    require_unavailable([] { MR::createPurpleCoinCounter(); },
                        "Purple Coin counter creation must not succeed without GameSceneLayoutHolder");
    require_unavailable([] { MR::validatePurpleCoinCounter(); },
                        "Purple Coin counter validation must not succeed without the real counter layout");
    ++passed;

    {
        ProbeActor actor;
        actor.makeActorAppeared();
        actor.initHitSensor(2);
        auto* first = smgpc::compat::add_actor_hit_sensor(&actor, "first", 1U, 1U, 10.0F, {});
        auto* second = smgpc::compat::add_actor_hit_sensor(&actor, "second", 1U, 1U, 10.0F, {});
        first->addHitSensor(second);
        actor.initBinder(50.0F, 0.0F, 8U);
        auto contacts = smgpc::compat::ActorBinderContactState{};
        contacts.wall = true;
        contacts.wall_normal.set(1.0F, 0.0F, 0.0F);
        contacts.fix_reaction.set(1.0F, 0.0F, 0.0F);
        smgpc::compat::record_actor_binder_contacts(&actor, contacts);
        actor.mFlag.mIsNoCalcAnim = true;

        MR::resetPosition(&actor, TVec3f{10.0F, 20.0F, 30.0F});
        require(first->mSensorCount == 0U, "resetPosition must clear real HitSensor contacts");
        require(!MR::isBindedWall(&actor), "resetPosition must clear the registered Binder contact state");
        require(actor.calc_anim_count == 1 && actor.mFlag.mIsNoCalcAnim,
                "resetPosition must perform calcAnimDirect semantics and restore the no-calc flag");
        require(actor.mPosition.epsilonEquals(TVec3f{10.0F, 20.0F, 30.0F}, 0.0001F),
                "the position overload must set the requested position before reset");
        ++passed;
    }

    {
        ProbeActor actor;
        actor.mRotation.set(90.0F, 0.0F, 0.0F);
        actor.mScale.zero();
        auto axis = TVec3f{};
        MR::calcActorAxisY(&axis, &actor);
        require(axis.epsilonEquals(TVec3f{0.0F, 0.0F, 1.0F}, 0.0001F),
                "calcActorAxisY must use the actor's rotation matrix rather than scaled render axes");

        actor.initBinder(50.0F, 0.0F, 8U);
        auto ground = smgpc::compat::ActorBinderContactState{};
        ground.ground = true;
        ground.ground_normal.set(0.0F, 1.0F, 0.0F);
        ground.fix_reaction.set(0.0F, 1.0F, 0.0F);
        smgpc::compat::record_actor_binder_contacts(&actor, ground);
        actor.mRotation.zero();
        actor.mGravity.set(0.0F, -1.0F, 0.0F);
        actor.mVelocity.zero();
        MR::addVelocityMoveToDirection(&actor, TVec3f{1.0F, 0.0F, 0.0F}, 3.0F);
        require(actor.mVelocity.epsilonEquals(TVec3f{3.0F, 0.0F, 0.0F}, 0.0001F),
                "directional acceleration must use the exact gravity-plane then ground-plane calculation");

        auto wall = smgpc::compat::ActorBinderContactState{};
        wall.wall = true;
        wall.wall_normal.set(1.0F, 0.0F, 0.0F);
        wall.fix_reaction.set(1.0F, 0.0F, 0.0F);
        smgpc::compat::record_actor_binder_contacts(&actor, wall);
        actor.mVelocity.set(-4.0F, 2.0F, 0.0F);
        require_near(MR::calcHitPowerToWall(&actor), 4.0F,
                     "wall hit power must use the real recorded wall normal");
        require(MR::reboundVelocityFromCollision(&actor, 0.5F, 1.0F, 0.25F),
                "a collision above threshold must rebound from Binder's fix-reaction vector");
        require(actor.mVelocity.epsilonEquals(TVec3f{2.0F, 0.5F, 0.0F}, 0.0001F),
                "rebound velocity must match the decompiled normal/tangent calculation");
        ++passed;
    }

    {
        ProbeActor actor;
        actor.makeActorAppeared();
        actor.mPosition.set(0.0F, 0.0F, 20000.0F);
        MR::setClippingTypeSphere(&actor, 100.0F);
        const auto* clipping = smgpc::compat::actor_clipping_runtime_state(&actor);
        require(clipping != nullptr && clipping->sphere_configured && !clipping->far_level.has_value(),
                "sphere clipping must be actor-owned and must not invent a far level");

        auto camera = smgpc::camera::CameraPose{};
        camera.eye = {0.0F, 0.0F, 0.0F};
        camera.watch = {0.0F, 0.0F, 1.0F};
        camera.near_clip = 1.0F;
        camera.far_clip = 30000.0F;
        smgpc::compat::update_actor_clipping(actor, camera);
        require(!actor.mFlag.mIsClipped,
                "an unset clipping far level must use the real camera far plane, not a fabricated 100m level");
        MR::setClippingFar100m(&actor);
        smgpc::compat::update_actor_clipping(actor, camera);
        require(actor.mFlag.mIsClipped, "the scheduler clipping evaluator must consume the configured 100m level");
        actor.mPosition.z = 5000.0F;
        smgpc::compat::update_actor_clipping(actor, camera);
        require(!actor.mFlag.mIsClipped, "a configured actor must be restored when its sphere re-enters the frustum");
        ++passed;
    }

    {
        const auto shadow_baseline = smgpc::compat::actor_shadow_runtime_state_count();
        {
            ProbeActor actor;
            require(!MR::isExistShadow(&actor, nullptr),
                    "an actor without a controller list must not report a fabricated shadow");
            MR::initShadowVolumeSphere(&actor, 50.0F);

            const auto* shadow = smgpc::compat::actor_shadow_runtime_state(&actor);
            require(shadow != nullptr && shadow->capacity == 1U && shadow->controllers.size() == 1U,
                    "sphere initialization must create one actor-owned controller in a one-slot list");
            const auto* controller = smgpc::compat::actor_shadow_controller_runtime_state(&actor, nullptr);
            require(controller != nullptr && controller->name == "ボリューム影(球)" &&
                        controller->kind == smgpc::compat::ActorShadowControllerKind::VolumeSphere,
                    "sphere initialization must preserve the source controller name and shape kind");
            require_near(controller->radius, 50.0F,
                         "sphere initialization must retain the authored shadow radius");
            require(controller->drop_position == &actor.mPosition && controller->drop_direction == &actor.mGravity &&
                        controller->valid,
                    "a new volume controller must follow the host transform and begin valid");
            require(actor.mShadowControllerList == nullptr,
                    "native controller ownership must not place a host pointer in the retail Wii field");
            require(MR::isExistShadow(&actor, nullptr) && MR::isExistShadow(&actor, "any-single-controller-name"),
                    "the exact single-controller lookup must succeed without requiring a name match");

            MR::onCalcShadowOneTime(&actor, nullptr);
            require(controller->calculation_mode == smgpc::compat::ActorShadowCalculationMode::OneTime,
                    "one-time shadow calculation must update the owned controller");
            MR::onCalcShadow(&actor, nullptr);
            require(controller->calculation_mode == smgpc::compat::ActorShadowCalculationMode::Continuous,
                    "continuous shadow calculation must replace one-time mode");
            MR::offCalcShadow(&actor, nullptr);
            require(controller->calculation_mode == smgpc::compat::ActorShadowCalculationMode::Disabled,
                    "offCalcShadow must disable the owned controller");

            MR::onCalcShadowDropPrivateGravity(&actor, nullptr);
            require(controller->gravity_mode == smgpc::compat::ActorShadowGravityMode::PrivateContinuous,
                    "private-gravity calculation must be tracked per controller");
            MR::onCalcShadowDropPrivateGravityOneTime(&actor, nullptr);
            require(controller->gravity_mode == smgpc::compat::ActorShadowGravityMode::PrivateOneTime,
                    "one-time private gravity must replace continuous mode");
            MR::offCalcShadowDropPrivateGravity(&actor, nullptr);
            require(controller->gravity_mode == smgpc::compat::ActorShadowGravityMode::PrivateDisabled,
                    "private-gravity disable must remain controller-local");

            auto drop_position = TVec3f{4.0F, 5.0F, 6.0F};
            MR::setShadowDropPositionPtr(&actor, nullptr, &drop_position);
            MR::setShadowDropLength(&actor, nullptr, 250.0F);
            require(controller->drop_position == &drop_position && std::abs(controller->drop_length - 250.0F) < 0.0001F,
                    "shadow position and length setters must update the named controller state");
            MR::invalidateShadow(&actor, nullptr);
            require(!controller->valid, "shadow invalidation must update the owned controller");
            MR::validateShadow(&actor, nullptr);
            require(controller->valid, "shadow validation must update the owned controller");
        }
        {
            ProbeActor actor;
            actor.initShadowControllerList(2U);
            auto& first = smgpc::compat::add_actor_shadow_controller(
                &actor, "first", smgpc::compat::ActorShadowControllerKind::SurfaceCircle, 20.0F);
            auto& second = smgpc::compat::add_actor_shadow_controller(
                &actor, "second", smgpc::compat::ActorShadowControllerKind::VolumeCylinder, 30.0F);
            require(smgpc::compat::actor_shadow_controller_runtime_state(&actor, "first") == &first &&
                        smgpc::compat::actor_shadow_controller_runtime_state(&actor, "second") == &second,
                    "multi-controller lookup must select the exact authored name");
            require(!MR::isExistShadow(&actor, nullptr) && !MR::isExistShadow(&actor, "missing"),
                    "multi-controller lookup must not turn an absent or unnamed controller into success");
        }
        require(smgpc::compat::actor_shadow_runtime_state_count() == shadow_baseline,
                "LiveActor destruction must release its complete shadow-controller state");
        ++passed;
    }

    {
        ProbeActor actor;
        actor.initBinder(50.0F, 0.0F, 8U);
        auto center = TVec3f{};
        require_unavailable([&] { (void)MR::tryCreateMirrorActor(&actor, "Coin"); },
                            "MirrorActor creation must not silently report that no MirrorArea exists");
        require_unavailable([&] { MR::setBinderExceptSensorType(&actor, &center, 10.0F); },
                            "ClipArea Binder filtering must not pretend to apply without CollisionParts sensor ownership");
        require_unavailable([&] { (void)MR::isInDeath(&actor, {}); },
                            "DeathArea membership must not become false while AreaObj ownership is absent");
        require_unavailable([&] { MR::onCalcShadow(&actor, nullptr); },
                            "shadow calculation must reject an actor without a controller list");
        require_unavailable([&] { MR::setClippingRangeIncludeShadow(&actor, &center, 100.0F); },
                            "shadow-aware clipping must not fabricate an unprojected center");

        auto floor = smgpc::compat::ActorBinderContactState{};
        floor.ground = true;
        floor.ground_normal.set(0.0F, 1.0F, 0.0F);
        floor.ground_attribute = 7U;
        smgpc::compat::record_actor_binder_contacts(&actor, floor);
        require(!MR::isBindedGroundDamageFire(&actor),
                "a raw host snapshot attribute must not be fabricated into a DamageFire floor code");
        floor.roof = true;
        floor.roof_normal.set(0.0F, -1.0F, 0.0F);
        smgpc::compat::record_actor_binder_contacts(&actor, floor);
        require_unavailable([&] { (void)MR::isPressedRoofAndGround(&actor); },
                            "roof/ground pressure must not ignore moving CollisionParts and press sensors");
        ++passed;
    }

    std::cout << "Game actor physics real-or-absent tests passed: " << passed << "/8\n";
    return 0;
}
