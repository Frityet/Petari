#include "Game/LiveActor/LiveActor.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "compat/ActorMotionCompat.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <stdexcept>
#include <string_view>

namespace {
    void refresh_actor_joint_matrices(LiveActor* actor) {
        auto* model = smgpc::compat::actor_model(actor);
        if (model == nullptr) {
            return;
        }
        if (auto* controller = smgpc::compat::actor_bck_ctrl(actor); controller != nullptr) {
            model->syncBckFrameController(
                controller->mFrame, controller->mRate, controller->mState);
        }
        model->refresh_resolved_joint_matrices(
            smgpc::compat::actor_base_matrix(actor));
    }
}  // namespace

LiveActor::LiveActor(const char* pName)
    : NameObj(pName), mPosition(0.0F, 0.0F, 0.0F), mRotation(0.0F, 0.0F, 0.0F),
      mScale(1.0F, 1.0F, 1.0F), mVelocity(0.0F, 0.0F, 0.0F), mGravity(0.0F, -1.0F, 0.0F),
      mModelManager(nullptr), mAnimKeeper(nullptr), mSpine(nullptr), mSensorKeeper(nullptr), mBinder(nullptr),
      mRailRider(nullptr), mEffectKeeper(nullptr), mSoundObject(nullptr), mFlag(), mShadowControllerList(nullptr),
      mCollisionParts(nullptr), mStageSwitchCtrl(nullptr), mStarPointerTarget(nullptr), mActorLightCtrl(nullptr),
      mCameraCtrl(nullptr) {
    smgpc::compat::register_actor_runtime_state(this);
}

LiveActor::~LiveActor() {
    smgpc::compat::release_actor_runtime_state(this);
}

void LiveActor::init(const JMapInfoIter&) {
}

void LiveActor::movement() {
    if (smgpc::compat::actor_model(this) != nullptr && !mFlag.mIsStoppedAnim) {
        smgpc::compat::advance_actor_animation(this);
    }
    if (mFlag.mIsDead) {
        return;
    }

    // Keep physics ownership at the retail virtual-call boundary. Derived
    // actors such as MarioActor call LiveActor::movement() and immediately
    // inspect the displacement and contact planes after it returns, so the
    // scheduler cannot legally run either phase outside this call.
    smgpc::compat::update_live_actor_gravity(*this);
    smgpc::compat::update_actor_hit_sensors(this);
    smgpc::compat::update_actor_nerve(this);

    if (mFlag.mIsDead) {
        return;
    }

    control();

    if (!mFlag.mIsDead) {
        updateBinder();
        smgpc::compat::update_actor_hit_sensors(this);
        if (mActorLightCtrl != nullptr) {
            MR::updateLightCtrl(this);
        }
    }
}

void LiveActor::calcAnim() {
    if (mFlag.mIsNoCalcAnim) {
        return;
    }
    calcAnmMtx();
}

void LiveActor::calcAnmMtx() {
    if (smgpc::compat::actor_model(this) != nullptr) {
        MR::setBaseScale(this, mScale);
        calcAndSetBaseMtx();
        refresh_actor_joint_matrices(this);
    }
}

void LiveActor::calcViewAndEntry() {
    // Model view calculation and entry are performed by the native renderer.
    // Retail does not recalculate the base or animation matrices in this pass.
}

void LiveActor::appear() {
    makeActorAppeared();
}

void LiveActor::kill() {
    makeActorDead();
}

void LiveActor::makeActorAppeared() {
    if (mFlag.mIsClipped) {
        endClipped();
    }
    mFlag.mIsDead = false;
    smgpc::compat::validate_actor_hit_sensors(this);
    smgpc::compat::update_actor_hit_sensors(this);
}

void LiveActor::makeActorDead() {
    mVelocity.zero();
    mFlag.mIsDead = true;
    smgpc::compat::invalidate_actor_hit_sensors(this);
    smgpc::compat::clear_actor_binder_contacts(this);
}

bool LiveActor::receiveMessage(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (msg == ACTMES_PUSH) {
        return receiveMsgPush(pSender, pReceiver);
    }
    if (msg > ACTMES_PLAYER_ATTACK_START && msg < ACTMES_PLAYER_ATTACK_END) {
        return receiveMsgPlayerAttack(msg, pSender, pReceiver);
    }
    if (msg > ACTMES_ENEMY_ATTACK_START && msg < ACTMES_ENEMY_ATTACK_END) {
        return receiveMsgEnemyAttack(msg, pSender, pReceiver);
    }
    if (msg == ACTMES_TAKE) {
        return receiveMsgTake(pSender, pReceiver);
    }
    if (msg == ACTMES_TAKEN) {
        return receiveMsgTaken(pSender, pReceiver);
    }
    if (msg == ACTMES_THROW) {
        return receiveMsgThrow(pSender, pReceiver);
    }
    if (msg == ACTMES_APART) {
        return receiveMsgApart(pSender, pReceiver);
    }
    return receiveOtherMsg(msg, pSender, pReceiver);
}

MtxPtr LiveActor::getBaseMtx() const {
    if (smgpc::compat::actor_model(this) == nullptr) {
        return nullptr;
    }
    const auto& matrix = smgpc::compat::actor_base_matrix(this);
    return reinterpret_cast<MtxPtr>(const_cast<f32*>(matrix.m.data()));
}

MtxPtr LiveActor::getTakingMtx() const {
    return getBaseMtx();
}

void LiveActor::attackSensor(HitSensor*, HitSensor*) {
}

bool LiveActor::receiveMsgApart(HitSensor* pSender, HitSensor* pReceiver) {
    MR::setHitSensorApart(pSender, pReceiver);
    return true;
}

void LiveActor::startClipped() {
    mFlag.mIsClipped = true;
    smgpc::compat::invalidate_actor_hit_sensors(this);
}

void LiveActor::endClipped() {
    mFlag.mIsClipped = false;
    if (!mFlag.mIsDead) {
        smgpc::compat::validate_actor_hit_sensors(this);
        smgpc::compat::update_actor_hit_sensors(this);
    }
}

void LiveActor::calcAndSetBaseMtx() {
    if (MR::getTaken(this)) {
        MR::setBaseTRMtx(this, MR::getTaken(this)->mHost->getTakingMtx());
    } else {
        TPos3f mtx;

        if (mRotation.x == 0.0f && mRotation.z == 0.0f) {
            MR::makeMtxTransRotateY(mtx, this);
        } else {
            MR::makeMtxTR(mtx, this);
        }

        MR::setBaseTRMtx(this, mtx);
    }
}

void LiveActor::initNerve(const Nerve* pNerve) {
    smgpc::compat::replace_actor_spine(this, pNerve);
}

void LiveActor::setNerve(const Nerve* pNerve) {
    if (mSpine != nullptr) {
        mSpine->setNerve(pNerve);
    }
}

bool LiveActor::isNerve(const Nerve* pNerve) const {
    return mSpine != nullptr && mSpine->getCurrentNerve() == pNerve;
}

s32 LiveActor::getNerveStep() const {
    return mSpine != nullptr ? mSpine->mStep : 0;
}

void LiveActor::initSound(int, bool) {
    // Audio is intentionally absent from the current native compatibility
    // boundary. Keep the exact retail slot null.
}

void LiveActor::initModelManagerWithAnm(const char* pModelArcName, const char* pAnimArcName, bool) {
    smgpc::compat::initialize_actor_model(this, pModelArcName, pAnimArcName);
    calcAndSetBaseMtx();
}

void LiveActor::initEffectKeeper(int effectNum, const char* pEffectName, bool sort) {
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        const auto* model = smgpc::compat::actor_model(this);
        const auto groupName = pEffectName != nullptr ? std::string_view(pEffectName) :
                                                       (model != nullptr ? model->model_arc_name() : std::string_view{});
        runtime->register_effect_keeper(smgpc::runtime::EffectKeeperHostKind::LiveActor, getName(), effectNum,
                                        groupName, sort, this);
    }
}

void LiveActor::initActorLightCtrl() {
    smgpc::compat::replace_actor_light_ctrl(this);
}

void LiveActor::initHitSensor(int sensorCount) {
    smgpc::compat::initialize_actor_hit_sensors(this, sensorCount);
}

void LiveActor::initBinder(f32 radius, f32 offset, u32 type) {
    smgpc::compat::configure_actor_binder(this, radius, offset, type);
    MR::onBind(this);
}

void LiveActor::initRailRider(const JMapInfoIter& rIter) {
    smgpc::compat::replace_actor_rail_rider(this, rIter);
}

void LiveActor::initShadowControllerList(u32 controllerCount) {
    // The native runtime owns the controller records without expanding the
    // retail LiveActor layout or writing a host object into its Wii pointer.
    smgpc::compat::initialize_actor_shadow_controller_list(this, controllerCount);
}

void LiveActor::initActorCollisionParts(const char* resourceName, HitSensor* sensor,
                                        ResourceHolder* resourceHolder, MtxPtr matrix, bool, bool) {
    if (resourceHolder == nullptr) {
        throw std::logic_error("Model-owned CollisionParts are unavailable without an exact ModelManager resource provider.");
    }
    MR::initCollisionPartsFromResourceHolder(this, resourceName, sensor, resourceHolder, matrix);
}

void LiveActor::initStageSwitch(const JMapInfoIter& rIter) {
    smgpc::compat::adopt_actor_stage_switch(this, MR::createStageSwitchCtrl(this, rIter));
}

void LiveActor::initActorStarPointerTarget(f32 radius, const TVec3f* position, MtxPtr matrix, TVec3f offset) {
    if (position != nullptr || matrix != nullptr) {
        throw std::logic_error("Pointer- or matrix-bound StarPointerTarget requires the exact retail target provider.");
    }
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->star_pointer().register_target(
            *this, radius, smgpc::camera::CameraParamVec3{.x = offset.x, .y = offset.y, .z = offset.z});
    }
}

HitSensor* LiveActor::getSensor(const char* pSensorName) const {
    return smgpc::compat::actor_hit_sensor(this, pSensorName);
}

void LiveActor::addToSoundObjHolder() {
    // Audio is intentionally absent.
}

void LiveActor::updateBinder() {
    smgpc::compat::integrate_live_actor_velocity(*this);
}
