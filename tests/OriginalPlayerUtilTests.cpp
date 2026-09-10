#include "OriginalPlayerUtilTests.hpp"

#include "Game/Animation/XanimePlayer.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/StageCollisionService.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace smgpc::tests {
    namespace {
        void require(bool condition, std::string_view message) {
            if (!condition) {
                throw std::runtime_error(std::string(message));
            }
        }

        bool near(const TVec3f& actual, const TVec3f& expected) {
            return std::fabs(actual.x - expected.x) < 0.0001F &&
                   std::fabs(actual.y - expected.y) < 0.0001F &&
                   std::fabs(actual.z - expected.z) < 0.0001F;
        }

        template<class F>
        struct Restore final {
            F action;
            ~Restore() { action(); }
        };

        void verify_live_vectors(MarioActor& actor) {
            auto& mario = *actor.mMario;
            const auto saved_velocity = mario.mVelocity;
            const auto saved_actor_velocity = actor.mVelocity;
            const auto saved_actor_gravity = actor.mGravity;
            const auto saved_last_move = actor.mLastMove;
            const auto saved_rush = actor._934;
            const auto restore = Restore{[&] {
                mario.mVelocity = saved_velocity;
                actor.mVelocity = saved_actor_velocity;
                actor.mGravity = saved_actor_gravity;
                actor.mLastMove = saved_last_move;
                actor._934 = saved_rush;
            }};

            require(MR::getPlayerPos() == &actor.mPosition &&
                        MR::getPlayerCenterPos() == &actor._2A0,
                    "original position getters must borrow the actual MarioActor fields");
            actor._934 = false;
            mario.mVelocity.set(11.0F, 22.0F, 33.0F);
            actor.mVelocity.set(-11.0F, -22.0F, -33.0F);
            require(MR::getPlayerVelocity() == &mario.mVelocity &&
                        near(*MR::getPlayerVelocity(), mario.mVelocity),
                    "unbound player velocity must borrow internal Mario velocity, independently of LiveActor velocity");
            MR::getPlayerVelocity()->x = 44.0F;
            require(mario.mVelocity.x == 44.0F && actor.mVelocity.x == -11.0F,
                    "writes through player velocity must reach its original owner");

            const auto* gravity = &actor.getGravityVec();
            const auto selected_gravity = *gravity;
            actor.mGravity.set(7.0F, 8.0F, 9.0F);
            require(MR::getPlayerGravity() == gravity && gravity != &actor.mGravity &&
                        near(*MR::getPlayerGravity(), selected_gravity),
                    "player gravity must borrow Mario's selected gravity vector rather than the LiveActor cache");

            actor._934 = true;
            actor.mLastMove.set(3.0F, 4.0F, 5.0F);
            require(MR::getPlayerVelocity() == &actor.getLastMove() &&
                        near(*MR::getPlayerVelocity(), actor.mLastMove),
                    "rush velocity must borrow the actor's actual last displacement");
        }

        void verify_translation(MarioActor& actor) {
            auto& mario = *actor.mMario;
            const auto position = actor.mPosition;
            const auto internal_position = mario.mPosition;
            const auto camera_position = actor.mCameraTrans;
            const auto conditional_position = mario._688;
            const auto condition = mario.mMovementStates._37;
            const auto transform_enabled = actor._1C0;
            const auto center = actor._2A0;
            const auto restore = Restore{[&] {
                mario.mMovementStates._37 = false;
                MR::setPlayerPos(position);
                mario.mPosition = internal_position;
                actor.mCameraTrans = camera_position;
                mario._688 = conditional_position;
                mario.mMovementStates._37 = condition;
                actor._1C0 = transform_enabled;
            }};
            auto* dummy = actor.getSensor("dummy");
            require(dummy != nullptr, "initialized Mario must retain its original translation-based dummy sensor");

            mario.mMovementStates._37 = false;
            mario._688.set(17.0F, 23.0F, 31.0F);
            const auto untouched = mario._688;
            actor._1C0 = false;
            const auto first = position + TVec3f(10.0F, -20.0F, 30.0F);
            MR::setPlayerPos(first);
            require(near(actor.mPosition, first) && near(mario.mPosition, first) &&
                        near(actor.mCameraTrans, first) && near(dummy->mPosition, first) && actor._1C0,
                    "setPlayerPos must immediately update actor/internal/camera position, sensors and matrix readiness");
            require(near(mario._688, untouched) && near(actor._2A0, center),
                    "setPlayerPos must preserve the conditional anchor when _37 is clear and leave the separate center field alone");

            mario.mMovementStates._37 = true;
            actor._1C0 = false;
            const auto second = position + TVec3f(-30.0F, 20.0F, -10.0F);
            MR::setPlayerPos(second);
            require(near(actor.mPosition, second) && near(mario.mPosition, second) &&
                        near(mario._688, second) && near(actor.mCameraTrans, second) &&
                        near(dummy->mPosition, second) && actor._1C0,
                    "setPlayerPos must also update Mario's conditional anchor when _37 is set");
        }

        void verify_ground_owner(MarioActor& actor) {
            auto& mario = *actor.mMario;
            const auto grounded = mario.mMovementStates._1;
            const auto rush = actor._934;
            auto* rush_sensor = actor._924;
            const auto restore = Restore{[&] {
                mario.mMovementStates._1 = grounded;
                actor._934 = rush;
                actor._924 = rush_sensor;
            }};
            actor._934 = false;
            const auto cached_ground = MR::isOnGround(&actor);
            mario.mMovementStates._1 = !cached_ground;
            require(MR::isOnGroundPlayer() == !cached_ground && MR::isOnGround(&actor) == cached_ground,
                    "unbound player grounding must follow the live original movement bit even when cached Binder grounding disagrees");
            mario.mMovementStates._1 = cached_ground;
            require(MR::isOnGroundPlayer() == cached_ground,
                    "player grounding must observe movement-bit changes without another scheduler tick");
            mario.mMovementStates._1 = grounded;

            auto* collision = scene::StageCollisionService::active();
            require(collision != nullptr, "rush grounding requires the still-live authored Gateway collision owner");
            auto down = actor.getGravityVec();
            const auto length = std::sqrt(down.dot(down));
            require(std::isfinite(length) && length > 0.0F, "rush grounding requires finite original Mario gravity");
            down.scale(1.0F / length);
            auto hit = scene::StageCollisionHit{};
            require(collision->line_cast(actor.mPosition - down * 250.0F, down * 1000.0F, &hit),
                    "rush grounding must find a real authored KCL surface beneath Mario");

            // This is an ordinary LiveActor/Binder on the actual map. No
            // synthetic contact record supplies the rush-host ground result.
            auto host = LiveActor("Original player utility rush host");
            host.mPosition = hit.position + hit.normal * 11.0F;
            host.mGravity = -hit.normal;
            host.mVelocity = -hit.normal * 4.0F;
            host.mFlag.mIsDead = false;
            host.initBinder(10.0F, 0.0F, 0U);
            host.updateBinder();
            require(host.mBinder->isBindedGround() && MR::isOnGround(&host),
                    "the real rush host Binder must publish contact with the authored KCL surface");
            auto sensor = HitSensor(0U, 0U, 10.0F, &host);
            actor._924 = &sensor;
            actor._934 = true;
            mario.mMovementStates._1 = false;
            require(MR::isOnGroundPlayer(),
                    "a bound player must inherit its grounded rush host even when Mario's movement bit is clear");
            host.mVelocity = hit.normal * 4.0F;
            mario.mMovementStates._1 = true;
            require(host.mBinder->isBindedGround() && !MR::isOnGround(&host) && !MR::isOnGroundPlayer(),
                    "rush grounding must evaluate host departure velocity despite retained contact and Mario's set movement bit");
            actor._934 = rush;
            actor._924 = rush_sensor;
        }

#ifndef NDEBUG
        void verify_process_trace_ownership() {
            auto& runtime = runtime::RuntimeContext::instance();
            require(compat::current_jkr_allocation_domain() != nullptr,
                    "trace ownership regression must emit from an actual Game allocation scope");
            constexpr auto category = std::string_view{"original-player-utility-scene-heap-ownership-regression"};
            constexpr auto name = std::string_view{"long-semantic-event-name-retained-after-scene-retirement"};
            constexpr auto detail = std::string_view{"long-event-detail-must-use-process-storage-despite-the-active-original-Game-heap"};
            runtime.emit_semantic_trace_event(category, name, detail);
            const auto events = runtime.semantic_trace_events();
            require(!events.empty(), "semantic trace must retain the emitted event");
            const auto& event = events.back();
            require(event.category == category && event.name == name && event.detail == detail &&
                        event.stage_name == runtime.current_stage_name(),
                    "semantic trace must preserve complete long strings and the current stage identity");
            require(JKRHeap::findFromRoot(const_cast<void*>(static_cast<const void*>(events.data()))) == nullptr &&
                        JKRHeap::findFromRoot(const_cast<char*>(event.category.data())) == nullptr &&
                        JKRHeap::findFromRoot(const_cast<char*>(event.name.data())) == nullptr &&
                        JKRHeap::findFromRoot(const_cast<char*>(event.detail.data())) == nullptr &&
                        JKRHeap::findFromRoot(const_cast<char*>(event.stage_name.data())) == nullptr,
                    "process trace vector and retained event strings must not belong to the emitting scene's Game heap");
        }
#endif

        void verify_control_reset(MarioActor& actor) {
            auto& mario = *actor.mMario;
            require(mario.mSwim != nullptr && mario.getAnimator()->mXanimePlayer != nullptr,
                    "control reset requires the original initialized Swim and animation owners");
            const auto previous_control = actor._3C0;
            const auto restore_control = Restore{[&] { actor._3C0 = previous_control; }};
            auto& animation = *mario.getAnimator()->mXanimePlayer;
            mario._13C.set(1.0F, 2.0F, 3.0F);
            mario._148.set(4.0F, 5.0F, 6.0F);
            mario._154.set(7.0F, 8.0F, 9.0F);
            mario.mVerticalSpeed = 13.0F;
            mario.mSwim->mColdWaterDamageInterval = 17;
            mario._1C._3 = true;
            mario.mMovementStates._1F = true;
            animation._7E = false;

            MR::offPlayerControl();
            require(actor._3C0 && MR::isOffPlayerControl(),
                    "offPlayerControl must synchronously set the original actor control flag");
            MR::onPlayerControl(false);
            require(!actor._3C0 && !MR::isOffPlayerControl() &&
                        mario.mVerticalSpeed == 13.0F && mario.mSwim->mColdWaterDamageInterval == 17 &&
                        mario._1C._3 && !animation._7E && near(mario._13C, TVec3f(1.0F, 2.0F, 3.0F)),
                    "onPlayerControl(false) must enable control without performing resetCondition");
            MR::offPlayerControl();
            // Last gameplay operation in this fixture: the real reset may
            // close states and change animation. Do not synthesize restoration
            // of those original state transitions or schedule more movement.
            MR::onPlayerControl(true);
            require(!actor._3C0 && !MR::isOffPlayerControl() && animation._7E &&
                        mario.mSwim->mColdWaterDamageInterval == 0 && !mario._1C._3 &&
                        mario.mVerticalSpeed == 0.0F && near(mario._13C, TVec3f(0.0F, 0.0F, 0.0F)) &&
                        near(mario._148, TVec3f(0.0F, 0.0F, 0.0F)) && near(mario._154, TVec3f(0.0F, 0.0F, 0.0F)),
                    "onPlayerControl(true) must execute original resetCondition immediately, including animation, swim and movement reset");
            require(mario.mMovementStates._1F,
                    "original resetCondition must preserve movement bit _1F while resetting the other movement flags");
        }
    }

    void verify_original_player_util(MarioActor& actor) {
        require(actor.mMario != nullptr && MR::getMarioHolder()->getMarioActor() == &actor,
                "player utility checks require the initialized actual MarioHolder owner");
        const auto domain = compat::JkrAllocationScope(
            runtime::RuntimeContext::instance().scheduler().allocation_domain());
#ifndef NDEBUG
        verify_process_trace_ownership();
#endif
        verify_live_vectors(actor);
        verify_translation(actor);
        verify_ground_owner(actor);
        verify_control_reset(actor);
        std::cout << "[proof] original PlayerUtil actual owners, translation, KCL rush grounding and synchronous control reset passed\n";
    }
}
