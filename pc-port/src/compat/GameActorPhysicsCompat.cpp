#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "compat/ActorMotionCompat.hpp"
#include "compat/ActorPhysicsRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeServices.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace {
    [[noreturn]] void throw_scene_playing_result_unavailable() {
        throw std::logic_error("ScenePlayingResult is unavailable in the active scene.");
    }

    [[noreturn]] void throw_game_scene_layout_unavailable() {
        throw std::logic_error("GameSceneLayoutHolder is unavailable in the active scene.");
    }

    LiveActor &require_actor(LiveActor *actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor utility requires a LiveActor.");
        }
        return *actor;
    }

    const LiveActor &require_actor(const LiveActor *actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor utility requires a LiveActor.");
        }
        return *actor;
    }

    smgpc::compat::ActorShadowControllerRuntimeState &require_shadow_controller(
        LiveActor *actor, const char *name) {
        (void)require_actor(actor);
        auto *controller = smgpc::compat::actor_shadow_controller_runtime_state(actor, name);
        if (controller == nullptr) {
            throw std::logic_error("Actor has no matching shadow controller.");
        }
        return *controller;
    }

    template < typename Operation >
    void for_each_shadow_controller(LiveActor *actor, const char *name, Operation &&operation) {
        (void)require_actor(actor);
        if (name != nullptr) {
            operation(require_shadow_controller(actor, name));
            return;
        }

        auto *shadow = smgpc::compat::actor_shadow_runtime_state(actor);
        if (shadow == nullptr) {
            throw std::logic_error("Actor has no initialized shadow controller list.");
        }
        if (shadow->controllers.empty()) {
            throw std::logic_error("Actor has no shadow controllers.");
        }
        for (auto &controller : shadow->controllers) {
            operation(controller);
        }
    }

    smgpc::compat::ActorShadowRuntimeState &require_shadow_state(LiveActor *actor) {
        (void)require_actor(actor);
        auto *shadow = smgpc::compat::actor_shadow_runtime_state(actor);
        if (shadow == nullptr) {
            throw std::logic_error("Actor has no initialized shadow controller list.");
        }
        return *shadow;
    }

    TVec3f &require_vector(TVec3f *vector) {
        if (vector == nullptr) {
            throw std::invalid_argument("Actor utility requires an output vector.");
        }
        return *vector;
    }

    const TVec3f &require_player_position() {
        const auto *position = MR::getPlayerPos();
        if (position == nullptr) {
            throw std::logic_error("Player position is unavailable.");
        }
        return *position;
    }

    [[noreturn]] void throw_shadow_projection_unavailable() {
        throw std::logic_error("Shadow-aware clipping is unavailable without real projection and draw behavior.");
    }
}  // namespace

namespace MR {
    void resetPosition(LiveActor *pActor) {
        auto &actor = require_actor(pActor);
        auto sensors = std::vector<HitSensor *>{};
        smgpc::compat::collect_actor_hit_sensors(&actor, sensors);
        for (auto *sensor : sensors) {
            if (sensor != nullptr) {
                sensor->mSensorCount = 0U;
            }
        }
        if (smgpc::compat::has_actor_binder(&actor)) {
            smgpc::compat::clear_actor_binder_contacts(&actor);
        }
        if (actor.mFlag.mIsCalcGravity) {
            smgpc::compat::update_live_actor_gravity(actor);
        }
        const auto was_no_calc_anim = actor.mFlag.mIsNoCalcAnim;
        actor.mFlag.mIsNoCalcAnim = false;
        actor.calcAnim();
        actor.mFlag.mIsNoCalcAnim = was_no_calc_anim;
    }

    void resetPosition(LiveActor *pActor, const TVec3f &rPosition) {
        require_actor(pActor).mPosition.set(rPosition);
        resetPosition(pActor);
    }

    void resetPosition(LiveActor *, const char *) {
        throw std::logic_error("Named-position reset is unavailable without NamePosHolder.");
    }

    bool isInDeath(const LiveActor *pActor, const TVec3f &rOffset) {
        (void)require_actor(pActor);
        (void)rOffset;
        throw std::logic_error("DeathArea queries are unavailable without parsed AreaObj ownership.");
    }

    void calcActorAxisY(TVec3f *pOut, const LiveActor *pActor) {
        auto &output = require_vector(pOut);
        const auto &actor = require_actor(pActor);
        Mtx matrix{};
        MR::makeMtxTR(matrix, &actor);
        output.set(matrix[0][1], matrix[1][1], matrix[2][1]);
    }

    bool isNearPlayer(const LiveActor *pActor, f32 distance) {
        const auto &actor = require_actor(pActor);
        auto *player = smgpc::compat::active_player_system_for_player_util();
        if (player == nullptr || player->attached_actor() == nullptr) {
            throw std::logic_error("Player state is unavailable.");
        }
        return !player->is_player_hidden() &&
               actor.mPosition.squareDistance(require_player_position()) < (distance * distance);
    }

    void calcVecToPlayerH(TVec3f *pOut, const LiveActor *pActor, const TVec3f *pUp) {
        auto &output = require_vector(pOut);
        const auto &actor = require_actor(pActor);
        output.set(require_player_position() - actor.mPosition);
        MR::vecKillElement(output, pUp != nullptr ? *pUp : actor.mGravity, &output);
        MR::normalizeOrZero(&output);
    }

    void attenuateVelocity(LiveActor *pActor, f32 scalar) {
        require_actor(pActor).mVelocity.scale(scalar);
    }

    void addVelocityMoveToDirection(LiveActor *pActor, const TVec3f &rDirection, f32 speed) {
        auto &actor = require_actor(pActor);
        auto direction = rDirection;
        MR::vecKillElement(direction, actor.mGravity, &direction);
        if (!MR::normalizeOrZero(&direction)) {
            direction.scale(speed);
            if (MR::isOnGround(&actor)) {
                MR::vecKillElement(direction, *MR::getGroundNormal(&actor), &direction);
            }
            actor.mVelocity.add(direction);
        }
    }

    void addVelocityJump(LiveActor *pActor, f32 speed) {
        auto &actor = require_actor(pActor);
        actor.mVelocity.add(actor.mGravity * -speed);
    }

    void addVelocityToGravity(LiveActor *pActor, f32 acceleration) {
        auto &actor = require_actor(pActor);
        actor.mVelocity.add(actor.mGravity * acceleration);
    }

    void addVelocityToGravityOrGround(LiveActor *pActor, f32 acceleration) {
        auto &actor = require_actor(pActor);
        if (MR::isBindedGround(&actor)) {
            actor.mVelocity.add(*MR::getGroundNormal(&actor) * -acceleration);
        } else {
            addVelocityToGravity(&actor, acceleration);
        }
    }

    bool reboundVelocityFromCollision(LiveActor *pActor, f32 restitution, f32 threshold, f32 tangentScale) {
        auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        if (contacts == nullptr || (!contacts->ground && !contacts->wall && !contacts->roof)) {
            return false;
        }
        auto unit_normal = contacts->fix_reaction;
        if (MR::normalizeOrZero(&unit_normal)) {
            return false;
        }
        const auto hit_speed = unit_normal.dot(actor.mVelocity);
        if (hit_speed >= 0.0F) {
            return false;
        }
        actor.mVelocity.sub(unit_normal * hit_speed);
        if (hit_speed < -threshold) {
            actor.mVelocity.scale(tangentScale);
            actor.mVelocity.sub(unit_normal * hit_speed * restitution);
            return true;
        }
        return false;
    }

    void turnDirectionDegree(const LiveActor *pActor, TVec3f *pDirection, const TVec3f &rTargetDirection, f32 degree) {
        const auto &actor = require_actor(pActor);
        auto &direction = require_vector(pDirection);
        auto current = direction;
        auto target = rTargetDirection;
        MR::vecKillElement(current, actor.mGravity, &current);
        MR::vecKillElement(target, actor.mGravity, &target);
        if (MR::normalizeOrZero(&current) || MR::normalizeOrZero(&target)) {
            return;
        }
        const auto cosine = MR::clamp(current.dot(target), -1.0F, 1.0F);
        const auto angle = std::acos(cosine) * (180.0F / 3.14159265358979323846F);
        if (angle <= degree) {
            direction.set(target);
            return;
        }
        const auto cross = current.cross(target);
        const auto signed_degree = cross.dot(actor.mGravity) > 0.0F ? degree : -degree;
        MR::rotateVecDegree(&direction, current, actor.mGravity, signed_degree);
        MR::normalizeOrZero(&direction);
    }

    void turnDirectionToPlayerDegree(const LiveActor *pActor, TVec3f *pDirection, f32 degree) {
        const auto &actor = require_actor(pActor);
        turnDirectionDegree(&actor, pDirection, require_player_position() - actor.mPosition, degree);
    }

    f32 calcNerveValue(const LiveActor *pActor, s32 stepMax, f32 valueStart, f32 valueEnd) {
        const auto &actor = require_actor(pActor);
        const auto rate = stepMax <= 0 ? 1.0F : MR::clamp(static_cast<f32>(actor.getNerveStep()) / static_cast<f32>(stepMax), 0.0F, 1.0F);
        return valueStart + ((valueEnd - valueStart) * rate);
    }

    f32 calcHitPowerToWall(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        if (contacts == nullptr || !contacts->wall) {
            return 0.0F;
        }
        const auto speed = actor.mVelocity.dot(contacts->wall_normal);
        return speed < 0.0F ? -speed : 0.0F;
    }

    void zeroVelocity(LiveActor *pActor) {
        require_actor(pActor).mVelocity.zero();
    }

    MirrorActor *tryCreateMirrorActor(LiveActor *pActor, const char *) {
        (void)require_actor(pActor);
        throw std::logic_error("MirrorActor creation is unavailable without parsed MirrorArea ownership and mirror rendering.");
    }

    void setBinderExceptSensorType(LiveActor *pActor, const TVec3f *pCenter, f32) {
        (void)require_actor(pActor);
        if (pCenter == nullptr) {
            throw std::invalid_argument("Clip-area binder filtering requires a center position.");
        }
        throw std::logic_error(
            "Clip-area binder filtering is unavailable without CollisionParts sensor ownership and ClipAreaHolder.");
    }

    void setClippingFar100m(LiveActor *pActor) {
        smgpc::compat::configure_actor_clipping_far_level(pActor, 6);
    }

    bool isPressedRoofAndGround(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        if (contacts == nullptr || !contacts->roof || !contacts->ground) {
            return false;
        }
        throw std::logic_error(
            "Pressed roof/ground resolution is unavailable without bound sensors and moving CollisionParts force data.");
    }

    bool isOnGround(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        return contacts != nullptr && contacts->ground && actor.mVelocity.dot(contacts->ground_normal) <= 0.0F;
    }

    bool isBindedGround(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        return contacts != nullptr && contacts->ground;
    }

    bool isBindedWall(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        return contacts != nullptr && contacts->wall;
    }

    bool isBindedRoof(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        return contacts != nullptr && contacts->roof;
    }

    const TVec3f *getGroundNormal(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        if (contacts == nullptr || !contacts->ground) {
            throw std::logic_error("Ground normal is unavailable without a real Binder ground contact.");
        }
        return &contacts->ground_normal;
    }

    const TVec3f *getWallNormal(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        if (contacts == nullptr || !contacts->wall) {
            throw std::logic_error("Wall normal is unavailable without a real Binder wall contact.");
        }
        return &contacts->wall_normal;
    }

    const TVec3f *getRoofNormal(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        const auto *contacts = smgpc::compat::actor_binder_contacts(&actor);
        if (contacts == nullptr || !contacts->roof) {
            throw std::logic_error("Roof normal is unavailable without a real Binder roof contact.");
        }
        return &contacts->roof_normal;
    }

    bool isNoBind(const LiveActor *pActor) {
        return require_actor(pActor).mFlag.mIsNoBind;
    }

    void onBind(LiveActor *pActor) {
        require_actor(pActor).mFlag.mIsNoBind = false;
    }

    void offBind(LiveActor *pActor) {
        auto &actor = require_actor(pActor);
        actor.mFlag.mIsNoBind = true;
        if (smgpc::compat::has_actor_binder(&actor)) {
            smgpc::compat::clear_actor_binder_contacts(&actor);
        }
    }

    void offCalcGravity(LiveActor *pActor) {
        require_actor(pActor).mFlag.mIsCalcGravity = false;
    }

    void onCalcGravity(LiveActor *pActor) {
        auto &actor = require_actor(pActor);
        actor.mFlag.mIsCalcGravity = true;
        if (!actor.mFlag.mIsDead) {
            smgpc::compat::update_live_actor_gravity(actor);
        }
    }

    bool isBindedGroundDamageFire(const LiveActor *pActor) {
        const auto &actor = require_actor(pActor);
        if (actor.mBinder == nullptr || !actor.mBinder->isBindedGround()) {
            return false;
        }
        return MR::isGroundCodeDamageFire(&actor.mBinder->mGroundInfo.mParentTriangle);
    }

    void initShadowSurfaceCircle(LiveActor *actor, f32 radius) {
        actor = &require_actor(actor);
        actor->initShadowControllerList(1U);
        (void)smgpc::compat::add_actor_shadow_controller(
            actor, "水面丸影", smgpc::compat::ActorShadowControllerKind::SurfaceCircle, radius);
    }

    void initShadowVolumeSphere(LiveActor *actor, f32 radius) {
        actor = &require_actor(actor);
        actor->initShadowControllerList(1U);
        (void)smgpc::compat::add_actor_shadow_controller(
            actor, "ボリューム影(球)", smgpc::compat::ActorShadowControllerKind::VolumeSphere, radius);
    }

    void initShadowVolumeCylinder(LiveActor *actor, f32 radius) {
        actor = &require_actor(actor);
        actor->initShadowControllerList(1U);
        (void)smgpc::compat::add_actor_shadow_controller(
            actor, "ボリューム影(円柱)", smgpc::compat::ActorShadowControllerKind::VolumeCylinder, radius);
    }

    void setShadowDropPositionPtr(LiveActor *actor, const char *name, const TVec3f *position) {
        require_shadow_controller(actor, name).drop_position = position;
    }

    void setShadowDropLength(LiveActor *actor, const char *name, f32 length) {
        if (!std::isfinite(length) || length < 0.0F) {
            throw std::invalid_argument("Actor shadow drop length must be finite and non-negative.");
        }
        require_shadow_controller(actor, name).drop_length = length;
    }

    void onCalcShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.calculation_mode = smgpc::compat::ActorShadowCalculationMode::Continuous;
        });
        require_shadow_state(actor).calculation_enabled = true;
    }

    void offCalcShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.calculation_mode = smgpc::compat::ActorShadowCalculationMode::Disabled;
        });
        require_shadow_state(actor).calculation_enabled = false;
    }

    void onCalcShadowOneTime(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.calculation_mode = smgpc::compat::ActorShadowCalculationMode::OneTime;
        });
        require_shadow_state(actor).calculation_enabled = true;
    }

    void onCalcShadowDropPrivateGravity(LiveActor *actor, const char *name) {
        require_shadow_controller(actor, name).gravity_mode =
            smgpc::compat::ActorShadowGravityMode::PrivateContinuous;
        require_shadow_state(actor).private_gravity = true;
    }

    void onCalcShadowDropPrivateGravityOneTime(LiveActor *actor, const char *name) {
        require_shadow_controller(actor, name).gravity_mode =
            smgpc::compat::ActorShadowGravityMode::PrivateOneTime;
        require_shadow_state(actor).private_gravity = true;
    }

    void offCalcShadowDropPrivateGravity(LiveActor *actor, const char *name) {
        require_shadow_controller(actor, name).gravity_mode =
            smgpc::compat::ActorShadowGravityMode::PrivateDisabled;
        require_shadow_state(actor).private_gravity = false;
    }

    bool isExistShadow(const LiveActor *actor, const char *name) {
        (void)require_actor(actor);
        return smgpc::compat::actor_shadow_controller_runtime_state(actor, name) != nullptr;
    }

    void invalidateShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.valid = false;
        });
        require_shadow_state(actor).valid = false;
    }

    void validateShadow(LiveActor *actor, const char *name) {
        for_each_shadow_controller(actor, name, [](auto &controller) {
            controller.valid = true;
        });
        require_shadow_state(actor).valid = true;
    }

    void setClippingRangeIncludeShadow(LiveActor *, TVec3f *, f32) {
        throw_shadow_projection_unavailable();
    }

    void incCoin(int) {
        throw_scene_playing_result_unavailable();
    }

    void incPurpleCoin() {
        throw_scene_playing_result_unavailable();
    }

    void declarePowerStarCoin100() {
        throw std::logic_error("EventPowerStar declaration is unavailable.");
    }

    void createPurpleCoinCounter() {
        throw_game_scene_layout_unavailable();
    }

    void validatePurpleCoinCounter() {
        throw_game_scene_layout_unavailable();
    }
}  // namespace MR
