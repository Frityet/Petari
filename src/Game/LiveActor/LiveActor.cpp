#include "Game/LiveActor/LiveActor.hpp"
#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "Game/LiveActor/ActorAnimKeeper.hpp"
#include "Game/LiveActor/ActorPadAndCameraCtrl.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingGroupHolder.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/StarPointerTarget.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/HitSensorKeeper.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>

#include <stdexcept>
#include <string_view>
#include <utility>

LiveActor::LiveActor(const char* pName)
    : NameObj(pName), mPosition(0.0F, 0.0F, 0.0F), mRotation(0.0F, 0.0F, 0.0F), mScale(1.0F, 1.0F, 1.0F), mVelocity(0.0F, 0.0F, 0.0F),
      mGravity(0.0F, -1.0F, 0.0F), mModelManager(nullptr), mAnimKeeper(nullptr), mSpine(nullptr), mSensorKeeper(nullptr), mBinder(nullptr),
      mRailRider(nullptr), mEffectKeeper(nullptr), mSoundObject(nullptr), mFlag(), mShadowControllerList(nullptr), mCollisionParts(nullptr),
      mStageSwitchCtrl(nullptr), mStarPointerTarget(nullptr), mActorLightCtrl(nullptr), mCameraCtrl(nullptr) {
    if (MR::getSceneObjHolder() != nullptr) {
        MR::getAllLiveActorGroup()->registerActor(this);
        auto* director = MR::getClippingDirector();
        director->registerActor(this);
        mNativeClippingHolder = director->mActorHolder;
        mNativeClippingGroups = director->mGroupHolder;
    }
}

LiveActor::~LiveActor() {
    delete std::exchange(mShadowControllerList, nullptr);
    releaseNativeResources();
    delete std::exchange(mSensorKeeper, nullptr);
}

void LiveActor::requireNativeResources() const {
    if (mNativeResourcesReleased) {
        aurora::throw_host_exception< std::logic_error >("LiveActor resources have already retired");
    }
}

std::shared_ptr< ModelManager > LiveActor::retainNativeModel() const {
    return mNativeModel;
}

void LiveActor::adoptNativeLodCtrl(std::unique_ptr< LodCtrl > lod) {
    requireNativeResources();
    if (!lod) {
        aurora::throw_host_exception< std::invalid_argument >("LiveActor LOD ownership requires a real LodCtrl");
    }
    if (mNativeLodCtrl) {
        aurora::throw_host_exception< std::logic_error >("LiveActor already owns a LodCtrl");
    }
    mNativeLodCtrl = std::move(lod);
}

void LiveActor::releaseNativeReference(const NameObj* object) noexcept {
    if (object == mNativeClippingHolder) {
        mNativeClippingHolder = nullptr;
        mNativeClippingGroups = nullptr;
    } else if (object == mNativeClippingGroups) {
        mNativeClippingGroups = nullptr;
    }
}

void LiveActor::releaseNativeSensorReference(const HitSensor* sensor) noexcept {
    if (mSensorKeeper != nullptr) {
        mSensorKeeper->releaseNativeReference(sensor);
    }
}

void LiveActor::releaseNativeResources() noexcept {
    if (std::exchange(mNativeResourcesReleased, true)) {
        return;
    }

    auto* clipping = std::exchange(mNativeClippingHolder, nullptr);
    auto* groups = std::exchange(mNativeClippingGroups, nullptr);
    if (clipping != nullptr) {
        clipping->unregisterNativeActor(this, groups);
    }

    // Publish retirement before child destruction can reenter their host.
    auto* effects = std::exchange(mEffectKeeper, nullptr);
    auto* target = std::exchange(mStarPointerTarget, nullptr);
    auto lod = std::move(mNativeLodCtrl);
    auto* light = std::exchange(mActorLightCtrl, nullptr);
    auto* stageSwitch = std::exchange(mStageSwitchCtrl, nullptr);
    auto* rail = std::exchange(mRailRider, nullptr);
    auto* spine = std::exchange(mSpine, nullptr);
    auto* binder = std::exchange(mBinder, nullptr);
    auto* sound = std::exchange(mSoundObject, nullptr);
    auto soundHeap = std::move(mNativeSoundHeap);
    auto* camera = std::exchange(mCameraCtrl, nullptr);
    auto* animation = std::exchange(mAnimKeeper, nullptr);
    auto model = std::move(mNativeModel);

    delete effects;
    releaseNativeCollisionParts();
    auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime != nullptr) {
        runtime->star_pointer().unregister_target(*this);
    }
    if (auto* scheduler = smgpc::runtime::try_active_scene_scheduler()) {
        scheduler->disconnect_name_obj(*this);
    } else if (runtime != nullptr) {
        runtime->unregister_live_actor_model(*this);
    }
    // Draw-buffer removal still reads the original model pointer above.
    mModelManager = nullptr;
    delete target;

    lod.reset();
    delete light;
    delete stageSwitch;
    delete rail;
    delete spine;
    delete binder;
    delete sound;
    soundHeap.reset();
    delete camera;
    delete animation;
    model.reset();
}

CollisionParts* LiveActor::adoptCollisionParts(std::unique_ptr< CollisionParts > parts) {
    const aurora::allocation::HostAllocationScope host;
    auto* result = parts.get();
    mNativeCollisionParts.push_back(std::move(parts));
    return result;
}

void LiveActor::releaseNativeCollisionParts() noexcept {
    mCollisionParts = nullptr;
    std::vector< std::unique_ptr< CollisionParts > > children;
    children.swap(mNativeCollisionParts);
}

void LiveActor::init(const JMapInfoIter&) {
}

void LiveActor::movement() {
    if (mModelManager != nullptr && !mFlag.mIsStoppedAnim) {
        mModelManager->update();

        if (mAnimKeeper != nullptr) {
            mAnimKeeper->update();
        }
    }
    if (MR::isCalcGravity(this)) {
        MR::calcGravity(this);
    }
    if (mSensorKeeper)
        mSensorKeeper->doObjCol();
    if (mFlag.mIsDead)
        return;
    if (mSpine != nullptr) {
        mSpine->update();
    }

    if (mFlag.mIsDead) {
        return;
    }

    control();

    if (!mFlag.mIsDead) {
        updateBinder();
        if (mEffectKeeper != nullptr) {
            mEffectKeeper->update();
        }
        if (mCameraCtrl != nullptr) {
            mCameraCtrl->update();
        }
        if (mActorLightCtrl != nullptr) {
            MR::updateLightCtrl(this);
        }
        MR::tryUpdateHitSensorsAll(this);
        MR::actorSoundMovement(this);
        MR::requestCalcActorShadow(this);
    }
}

void LiveActor::calcAnim() {
    if (mFlag.mIsNoCalcAnim) {
        return;
    }
    calcAnmMtx();
    if (mCollisionParts != nullptr) {
        MR::setCollisionMtx(this);
    }
}

void LiveActor::calcAnmMtx() {
    if (mModelManager == nullptr) {
        return;
    }

    MR::getJ3DModel(this)->setBaseScale(mScale);
    calcAndSetBaseMtx();
    mModelManager->calcAnim();
}

void LiveActor::calcViewAndEntry() {
    if (mFlag.mIsNoCalcView) {
        return;
    }

    if (mModelManager == nullptr) {
        return;
    }

    if (mFlag.mIsNoCalcView) {
        return;
    }

    mModelManager->calcView();
}

void LiveActor::appear() {
    makeActorAppeared();
}

void LiveActor::kill() {
    makeActorDead();
}

void LiveActor::makeActorAppeared() {
    if (mSensorKeeper != nullptr) {
        mSensorKeeper->validateBySystem();
    }

    if (MR::isClipped(this)) {
        endClipped();
    }

    mFlag.mIsDead = false;

    if (mCollisionParts != nullptr) {
        MR::validateCollisionParts(this);
    }

    MR::resetPosition(this);

    if (mActorLightCtrl != nullptr) {
        mActorLightCtrl->reset();
    }

    MR::tryUpdateHitSensorsAll(this);
    MR::addToClippingTarget(this);
    MR::connectToSceneTemporarily(this);

    if (!MR::isNoEntryDrawBuffer(this)) {
        MR::connectToDrawTemporarily(this);
    }
}

void LiveActor::makeActorDead() {
    mVelocity.zero();

    MR::clearHitSensors(this);

    if (mSensorKeeper != nullptr) {
        mSensorKeeper->invalidateBySystem();
    }

    if (getBinder() != nullptr) {
        mBinder->clear();
    }

    if (mEffectKeeper != nullptr) {
        mEffectKeeper->clear();
    }

    if (mCollisionParts != nullptr) {
        MR::invalidateCollisionParts(this);
    }

    mFlag.mIsDead = true;

    MR::removeFromClippingTarget(this);
    MR::disconnectToSceneTemporarily(this);
    MR::disconnectToDrawTemporarily(this);
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
    if (MR::getJ3DModel(this) != nullptr) {
        return MR::getJ3DModel(this)->mBaseTransformMtx;
    }

    return nullptr;
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

    if (getSensorKeeper() != nullptr) {
        mSensorKeeper->invalidateBySystem();
    }

    if (mEffectKeeper != nullptr) {
        mEffectKeeper->stopEmitterOnClipped();
    }

    MR::disconnectToSceneTemporarily(this);

    if (MR::isNoEntryDrawBuffer(this)) {
        return;
    }

    MR::disconnectToDrawTemporarily(this);
}

void LiveActor::endClipped() {
    mFlag.mIsClipped = false;

    if (getSensorKeeper() != nullptr) {
        mSensorKeeper->validateBySystem();
        MR::updateHitSensorsAll(this);
    }

    if (mEffectKeeper != nullptr) {
        mEffectKeeper->playEmitterOffClipped();
    }

    MR::connectToSceneTemporarily(this);

    if (MR::isNoEntryDrawBuffer(this)) {
        return;
    }

    MR::connectToDrawTemporarily(this);
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
    requireNativeResources();
    auto replacement = std::make_unique< Spine >(this, pNerve);
    delete std::exchange(mSpine, replacement.release());
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

void LiveActor::initSound(int param1, bool is2D) {
    requireNativeResources();
    auto heapOwner = mNativeModel ? mNativeModel->nativeAllocationHeap() : JKRHeap::retainCurrentNativeLifetime();
    if (!heapOwner) {
        aurora::throw_host_exception< std::logic_error >("Actor sound construction requires the original caller's Game heap");
    }
    std::unique_ptr< AudAnmSoundObject > replacement;
    {
        const JKRHeap::CurrentHeapScope heap(*heapOwner);
        const aurora::allocation::ClientAllocationScope allocations({true, true});
        replacement = std::make_unique< AudAnmSoundObject >(is2D ? nullptr : &mPosition, param1, heapOwner.get());
    }
    // The previous allocation heap must outlive the old object's final delete.
    auto previousHeap = std::move(mNativeSoundHeap);
    mNativeSoundHeap = std::move(heapOwner);
    delete std::exchange(mSoundObject, replacement.release());
}

void LiveActor::initModelManagerWithAnm(const char* pModelName, const char* pAnimName, bool a3) {
    requireNativeResources();
    if (mNativeModel) {
        aurora::throw_host_exception< std::logic_error >("Actor model replacement requires scene draw retirement first");
    }
    // Resource loading may wait for main-thread work; never hold the current
    // heap mutex across ModelManager::createNative.
    const aurora::allocation::HostAllocationScope host;
    auto owner = ModelManager::createNative(JKRHeap::retainCurrentNativeLifetime(), pModelName, pAnimName, a3);
    mModelManager = owner.get();
    mNativeModel = owner;
    try {
        const auto heapOwner = owner->nativeAllocationHeap();
        const JKRHeap::CurrentHeapScope heap(*heapOwner);
        const aurora::allocation::ClientAllocationScope allocations({true, true});
        J3DSys::CommandScope commands;

        MR::getJ3DModel(this)->setBaseScale(mScale);
        LiveActor::calcAndSetBaseMtx();
        MR::calcJ3DModel(this);

        auto animation = std::unique_ptr< ActorAnimKeeper >(ActorAnimKeeper::tryCreate(this));
        auto camera = std::unique_ptr< ActorPadAndCameraCtrl >(ActorPadAndCameraCtrl::tryCreate(mModelManager, &mPosition));
        delete std::exchange(mAnimKeeper, animation.release());
        delete std::exchange(mCameraCtrl, camera.release());
    } catch (...) {
        mModelManager = nullptr;
        mNativeModel.reset();
        throw;
    }
}

void LiveActor::initEffectKeeper(int effectNum, const char* pEffectName, bool sort) {
    requireNativeResources();
    auto keeper = std::make_unique< EffectKeeper >(getName(), MR::getModelResourceHolder(this), effectNum, pEffectName);
    auto* previous = std::exchange(mEffectKeeper, keeper.get());
    try {
        if (sort) {
            keeper->enableSort();
        }
        keeper->init(this);
        if (mBinder != nullptr) {
            keeper->setBinder(mBinder);
        }
    } catch (...) {
        mEffectKeeper = previous;
        throw;
    }
    keeper.release();
    delete previous;
}

void LiveActor::initActorLightCtrl() {
    requireNativeResources();
    auto replacement = std::make_unique< ActorLightCtrl >(this);
    delete std::exchange(mActorLightCtrl, replacement.release());
}

void LiveActor::initHitSensor(int sensorCount) {
    requireNativeResources();
    auto keeper = std::make_unique< HitSensorKeeper >(sensorCount);
    delete std::exchange(mSensorKeeper, keeper.release());
}

void LiveActor::initBinder(f32 radius, f32 offset, u32 type) {
    requireNativeResources();
    auto replacement = std::make_unique< Binder >(getBaseMtx(), &mPosition, &mGravity, radius, offset, type);
    auto* previous = std::exchange(mBinder, replacement.release());
    if (mEffectKeeper != nullptr) {
        mEffectKeeper->setBinder(mBinder);
    }
    delete previous;
    MR::onBind(this);
}

void LiveActor::initRailRider(const JMapInfoIter& rIter) {
    requireNativeResources();
    auto replacement = std::make_unique< RailRider >(rIter);
    delete std::exchange(mRailRider, replacement.release());
}

void LiveActor::initShadowControllerList(u32 controllerCount) {
    requireNativeResources();
    auto list = std::make_unique< ShadowControllerList >(this, controllerCount);
    delete std::exchange(mShadowControllerList, list.release());
}

void LiveActor::initActorCollisionParts(const char* pParam1, HitSensor* pParam2, ResourceHolder* pParam3, MtxPtr pParam4, bool param5, bool param6) {
    MR::CollisionScaleType scaleType;

    if (param6) {
        scaleType = MR::CollisionScaleType_NotUsingScale;
    } else {
        scaleType = MR::CollisionScaleType_Unk2;

        if (param5) {
            scaleType = MR::CollisionScaleType_AutoEqualScale;
        }
    }

    if (pParam3 != nullptr) {
        TPos3f mtx;

        if (pParam4 != nullptr) {
            mtx.set(pParam4);
        } else {
            MR::makeMtxTRS(mtx.toMtxPtr(), this);
        }

        mCollisionParts = MR::createCollisionPartsFromResourceHolder(pParam3, pParam1, pParam2, mtx, scaleType);
    } else if (pParam4 == nullptr) {
        mCollisionParts = MR::createCollisionPartsFromLiveActor(this, pParam1, pParam2, scaleType);
    } else {
        mCollisionParts = MR::createCollisionPartsFromLiveActor(this, pParam1, pParam2, pParam4, scaleType);
    }

    MR::invalidateCollisionParts(this);
}

void LiveActor::initStageSwitch(const JMapInfoIter& rIter) {
    requireNativeResources();
    auto replacement = std::unique_ptr< StageSwitchCtrl >(MR::createStageSwitchCtrl(this, rIter));
    delete std::exchange(mStageSwitchCtrl, replacement.release());
}

void LiveActor::initActorStarPointerTarget(f32 radius, const TVec3f* pTrans, MtxPtr pMtx, TVec3f offset) {
    requireNativeResources();
    auto target = std::make_unique< StarPointerTarget >(radius, pTrans, pMtx, offset);
    delete std::exchange(mStarPointerTarget, target.release());
}

HitSensor* LiveActor::getSensor(const char* pSensorName) const {
    if (mSensorKeeper != nullptr) {
        return mSensorKeeper->getSensor(pSensorName);
    }

    return nullptr;
}

void LiveActor::addToSoundObjHolder() {
    mSoundObject->addToSoundObjHolder();
}

void LiveActor::updateBinder() {
    if (mBinder == nullptr) {
        mPosition += mVelocity;
    } else if (mFlag.mIsNoBind) {
        mPosition += mVelocity;
        mBinder->clear();
    } else {
        mPosition += getBinder()->bind(mVelocity);
    }
}
