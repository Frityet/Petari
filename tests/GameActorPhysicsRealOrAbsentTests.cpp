#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/MapObj/ClipAreaHolder.hpp"
#include "Game/LiveActor/ShadowController.hpp"
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
#include "compat/StageSessionState.hpp"
#include "runtime/RuntimeContext.hpp"
#include "SceneExecutionFixture.hpp"
#include <aurora/dvd.h>
#include <cstdlib>

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

    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    require(disc && aurora_dvd_open(disc), "actor physics proof requires SMGPC_REAL_DISC");
    struct Disc { ~Disc() { aurora_dvd_close(); } } disc_guard;
    DVDInit();
    auto logger = smgpc::logging::create_default_logger();
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original actor physics utilities"});
    smgpc::render::AuroraRenderer renderer(window);
    smgpc::resource::GameResourceRuntime resources;
    smgpc::runtime::RuntimeContext runtime(*logger, window, resources);
    smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
    smgpc::compat::StageSessionState session("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0));
    smgpc::compat::StageSessionBinding session_binding(session);
    auto domain = smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 16U << 20);
    smgpc::test::SceneExecutionFixture scene(runtime.scheduler(), domain);
    const smgpc::compat::JkrAllocationScope game(domain);

    {
        ProbeActor actor;
        actor.makeActorAppeared();
        actor.initHitSensor(2);
        auto* first = smgpc::compat::add_actor_hit_sensor(&actor, "first", 1U, 1U, 10.0F, {});
        auto* second = smgpc::compat::add_actor_hit_sensor(&actor, "second", 1U, 1U, 10.0F, {});
        first->addHitSensor(second);
        actor.initBinder(50.0F, 0.0F, 8U);
        actor.mBinder->_158 = 1.0F;
        actor.mBinder->mWallInfo.mParentTriangle.mNormals[0].set(1.0F, 0.0F, 0.0F);
        require(MR::isBindedWall(&actor), "the actor begins with an actual original Binder wall contact");
        require(MR::getWallNormal(&actor) == &actor.mBinder->mWallInfo.mParentTriangle.mNormals[0],
                "wall normal queries return the original Binder triangle storage");
        actor.mFlag.mIsNoCalcAnim = true;
        actor.calc_anim_count = 0;

        MR::resetPosition(&actor, TVec3f{10.0F, 20.0F, 30.0F});
        require(first->mSensorCount == 0U, "resetPosition must clear real HitSensor contacts");
        require(!MR::isBindedWall(&actor),
                "resetPosition must clear original Binder contact immediately without another integration tick");
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
        actor.mBinder->_C8 = 0.0F;
        actor.mBinder->mGroundInfo.mParentTriangle.mNormals[0].set(0.0F, 1.0F, 0.0F);
        actor.mBinder->mFixReactionVector.set(0.0F, 1.0F, 0.0F);
        actor.mRotation.zero();
        actor.mGravity.set(0.0F, -1.0F, 0.0F);
        actor.mVelocity.zero();
        MR::addVelocityMoveToDirection(&actor, TVec3f{1.0F, 0.0F, 0.0F}, 3.0F);
        require(actor.mVelocity.epsilonEquals(TVec3f{3.0F, 0.0F, 0.0F}, 0.0001F),
                "directional acceleration must use the exact gravity-plane then ground-plane calculation");

        actor.mBinder->clear();
        actor.mBinder->_158 = 0.0F;
        actor.mBinder->mWallInfo.mParentTriangle.mNormals[0].set(1.0F, 0.0F, 0.0F);
        actor.mBinder->mFixReactionVector.set(1.0F, 0.0F, 0.0F);
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
        require(clipping != nullptr && clipping->sphere_configured && clipping->far_level == 6,
                "sphere configuration must preserve the original ClippingActorInfo 100m default");

        auto camera = smgpc::camera::CameraPose{};
        camera.eye = {0.0F, 0.0F, 0.0F};
        camera.watch = {0.0F, 0.0F, 1.0F};
        camera.near_clip = 1.0F;
        camera.far_clip = 30000.0F;
        smgpc::compat::update_actor_clipping(actor, camera);
        require(actor.mFlag.mIsClipped,
                "a registered actor begins with the original 100m clipping distance");
        MR::setClippingFarMax(&actor);
        smgpc::compat::update_actor_clipping(actor, camera);
        require(!actor.mFlag.mIsClipped,
                "an explicit maximum clipping distance must use the current camera far plane");
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
            require(actor.mShadowControllerList && actor.mShadowControllerList->getControllerCount() == 1U,
                    "actor must retain its actual original ShadowControllerList");
            auto* original = actor.mShadowControllerList->getController(0U);
            require(MR::isExistShadow(&actor, nullptr) && MR::isExistShadow(&actor, "any-single-controller-name"),
                    "the exact single-controller lookup must succeed without requiring a name match");

            MR::onCalcShadowOneTime(&actor, nullptr);
            require(original->_60 == 2 && original->_65 == 0,
                    "one-time shadow calculation must update the owned controller");
            MR::onCalcShadow(&actor, nullptr);
            require(original->_60 == 1,
                    "continuous shadow calculation must replace one-time mode");
            MR::offCalcShadow(&actor, nullptr);
            require(original->_60 == 0,
                    "offCalcShadow must disable the owned controller");

            MR::onCalcShadowDropPrivateGravity(&actor, nullptr);
            require(original->_61 == 4,
                    "private-gravity calculation must be tracked per controller");
            MR::onCalcShadowDropPrivateGravityOneTime(&actor, nullptr);
            require(original->_61 == 5 && original->_66 == 0,
                    "one-time private gravity must replace continuous mode");
            MR::offCalcShadowDropPrivateGravity(&actor, nullptr);
            require(original->_61 == 3,
                    "private-gravity disable must remain controller-local");

            auto drop_position = TVec3f{4.0F, 5.0F, 6.0F};
            MR::setShadowDropPositionPtr(&actor, nullptr, &drop_position);
            MR::setShadowDropLength(&actor, nullptr, 250.0F);
            require(original->mDropPos == &drop_position && std::abs(original->mDropLength - 250.0F) < 0.0001F,
                    "shadow position and length setters must update the named controller state");
            MR::invalidateShadow(&actor, nullptr);
            require(!original->_71, "shadow invalidation must update the owned controller");
            MR::validateShadow(&actor, nullptr);
            require(original->_71, "shadow validation must update the owned controller");
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
            require(!MR::isExistShadow(&actor, "missing"),
                    "multi-controller lookup must reject a missing authored controller name");
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
        MR::setBinderExceptSensorType(&actor, &center, 10.0F);
        auto* clip_filter = dynamic_cast<ClipAreaCollisionFilter*>(actor.mBinder->mCollisionPartsFilter);
        require(clip_filter && clip_filter->_04 == &center && clip_filter->_08 == 10.0F,
                "Binder filtering must retain the original ClipArea filter and the caller's live center");
        MR::setBinderCollisionPartsFilter(&actor, nullptr);
        delete clip_filter;
        require_unavailable([&] { (void)MR::isInDeath(&actor, {}); },
                            "DeathArea membership must not become false while AreaObj ownership is absent");
        require_unavailable([&] { MR::onCalcShadow(&actor, nullptr); },
                            "shadow calculation must reject an actor without a controller list");
        MR::initShadowVolumeSphere(&actor, 10.0F);
        MR::setClippingRangeIncludeShadow(&actor, &center, 100.0F);
        const auto* clipping = smgpc::compat::actor_clipping_runtime_state(&actor);
        require(clipping && clipping->sphere_radius == 100.0F && center.epsilonEquals(actor.mPosition, 0.0F),
                "an unprojected original shadow retains the actor's ordinary clipping sphere");

        require(!MR::isBindedGroundDamageFire(&actor),
                "an original Binder without ground contact cannot report a DamageFire floor");
        require(!MR::isPressedRoofAndGround(&actor),
                "an original Binder without opposing contacts cannot report crushing pressure");
        ++passed;
    }

    std::cout << "Game actor physics real-or-absent tests passed: " << passed << "/8\n";
    return 0;
}
