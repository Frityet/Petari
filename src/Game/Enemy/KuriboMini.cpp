#include "Game/Enemy/KuriboMini.hpp"
#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/Enemy/ItemGenerator.hpp"
#include "Game/Enemy/WalkerStateBindStarPointer.hpp"
#include "Game/Enemy/WalkerStateChase.hpp"
#include "Game/Enemy/WalkerStateFindPlayer.hpp"
#include "Game/Enemy/WalkerStateParam.hpp"
#include "Game/Enemy/WalkerStateStagger.hpp"
#include "Game/Enemy/WalkerStateWander.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/ActorStateUtil.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "revolution/mtx.h"

namespace {
    class KuriboMiniParam {
    public:
        KuriboMiniParam();

        WalkerStateParam mStateParam;                 // 0x00
        WalkerStateStaggerParam mStaggerParam;        // 0x18
        WalkerStateFindPlayerParam mFindPlayerParam;  // 0x48
        WalkerStateChaseParam mChaseParam;            // 0x54
        WalkerStateWanderParam mWanderParam;          // 0x68
    };

    KuriboMiniParam::KuriboMiniParam() {
        mStateParam.mGravityAccel = 1.5f;
        mStateParam.mAirFriction = 0.99f;
        mStateParam.mGroundFriction = 0.93f;
        mStateParam.mPlayerNearDistance = 1000.0f;
        mStateParam.mPlayerSightFanDegreeH = 70.0f;
        mStateParam.mPlayerSightFanDegreeV = 30.0f;
        mWanderParam.mSpeed = 0.1f;
        mWanderParam.mWaitTime = 120;
        mWanderParam.mWalkTime = 120;
        mWanderParam.mTurnMaxRateDegree = 3.0f;
        mChaseParam.mChaseSpeed = 0.2f;
        mFindPlayerParam.mTurnMaxRateDegree = 5.0f;
        mFindPlayerParam.mJumpVelocity = 20.0f;
    }

    static KuriboMiniParam sParam;
};  // namespace

namespace NrvKuriboMini {
    NEW_NERVE(KuriboMiniNrvWander, KuriboMini, Wander);
    NEW_NERVE(KuriboMiniNrvFindPlayer, KuriboMini, FindPlayer);
    NEW_NERVE(KuriboMiniNrvChase, KuriboMini, Chase);
    NEW_NERVE(KuriboMiniNrvStagger, KuriboMini, Stagger);

    class KuriboMiniNrvBindStarPointer : public Nerve {
    public:
        virtual void execute(Spine* pSpine) const {
            KuriboMini* actor = reinterpret_cast< KuriboMini* >(pSpine->mExecutor);
            if (!MR::updateActorStateAndNextNerve(actor, actor->mBindStarPointer, &KuriboMiniNrvWander::sInstance)) {
                actor->tryDeadMap();
            }
        }

        virtual void executeOnEnd(Spine* pSpine) const {
            KuriboMini* actor = reinterpret_cast< KuriboMini* >(pSpine->mExecutor);
            actor->mBindStarPointer->kill();
        }

        static KuriboMiniNrvBindStarPointer sInstance;
    };

    KuriboMiniNrvBindStarPointer KuriboMiniNrvBindStarPointer::sInstance;

    NEW_NERVE(KuriboMiniNrvAttackSuccess, KuriboMini, AttackSuccess);
    NEW_NERVE(KuriboMiniNrvHipDropDown, KuriboMini, HipDropDown);
    NEW_NERVE(KuriboMiniNrvPressDown, KuriboMini, PressDown);
    NEW_NERVE(KuriboMiniNrvFlatDown, KuriboMini, FlatDown);
    NEW_NERVE(KuriboMiniNrvBlowDown, KuriboMini, BlowDown);
};  // namespace NrvKuriboMini

KuriboMini::KuriboMini(const char* pName)
    : LiveActor(pName), mScaleController(nullptr), mItemGenerator(nullptr), mStateWander(nullptr), mStateFindPlayer(nullptr),
      mStateChase(nullptr), mStateStagger(nullptr), mBindStarPointer(nullptr), _A8(0.0f, 0.0f, 0.0f, 1.0f), _B8(0.0f, 0.0f, 1.0f) {
}

void KuriboMini::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    initModelManagerWithAnm("KuriboMini", nullptr, false);
    MR::connectToSceneEnemy(this);
    MR::makeQuatAndFrontFromRotate(&_A8, &_B8, this);
    MR::onCalcGravity(this);
    MR::initLightCtrl(this);
    MR::declareStarPiece(this, 3);
    MR::declareCoin(this, 1);
    mItemGenerator = new ItemGenerator();
    mScaleController = new AnimScaleController(nullptr);
    initSound(4, false);
    initEffectKeeper(1, nullptr, false);
    MR::initStarPointerTarget(this, 40.0f, TVec3f(0.0f, 60.0f, 0.0f));
    MR::initShadowVolumeSphere(this, 40.0f);
    initHitSensor(2);
    MR::addHitSensorEnemy(this, "body", 8, 60.0f, TVec3f(0.0f, 60.0f, 0.0f));
    MR::addHitSensorEnemyAttack(this, "attack", 8, 40.0f, TVec3f(0.0f, 60.0f, 0.0f));
    initBinder(60.0f, 60.0f, 0);
    initNerve(&NrvKuriboMini::KuriboMiniNrvWander::sInstance);
    initState();
    if (MR::isValidInfo(rIter)) {
        MR::setGroupClipping(this, rIter, 32);
    }

    MR::useStageSwitchWriteDead(this, rIter);
    MR::useStageSwitchSleep(this, rIter);
    if (MR::useStageSwitchReadB(this, rIter)) {
        MR::FunctorV0M< KuriboMini*, void (KuriboMini::*)() > deadFunc =
            MR::Functor_Inline(this, static_cast< void (KuriboMini::*)() >(&KuriboMini::makeActorDead));
        MR::listenStageSwitchOnB(this, deadFunc);
    }

    if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::syncStageSwitchAppear(this);
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

void KuriboMini::initAfterPlacement() {
    MR::trySetMoveLimitCollision(this);
}

void KuriboMini::initState() {
    mStateFindPlayer = new WalkerStateFindPlayer(this, &_B8, &sParam.mStateParam, &sParam.mFindPlayerParam);
    mStateWander = new WalkerStateWander(this, &_B8, &sParam.mStateParam, &sParam.mWanderParam);
    mStateChase = new WalkerStateChase(this, &_B8, &sParam.mStateParam, &sParam.mChaseParam);
    mStateStagger = new WalkerStateStagger(this, &_B8, &sParam.mStateParam, &sParam.mStaggerParam);
    mBindStarPointer = new WalkerStateBindStarPointer(this, mScaleController);
}

void KuriboMini::makeActorAppeared() {
    LiveActor::makeActorAppeared();
    mItemGenerator->setTypeStarPeace(3);
}

void KuriboMini::kill() {
    if (MR::isValidSwitchDead(this)) {
        MR::onSwitchDead(this);
    }

    MR::emitEffect(this, "Death");
    MR::startSound(this, "SE_EM_EXPLODE_S", -1, -1);
    mItemGenerator->generate(this);
    LiveActor::kill();
}

void KuriboMini::control() {
    MR::blendQuatFromGroundAndFront(&_A8, this, _B8, 0.05f, 0.5f);
    mScaleController->updateNerve();
}

void KuriboMini::calcAndSetBaseMtx() {
    MR::setBaseTRMtx(this, _A8);
    TVec3f scale;
    JMathInlineVEC::PSVECMultiply(&mScaleController->_C, &mScale, &scale);
    MR::setBaseScale(this, scale);
}

void KuriboMini::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return;
    }

    if (!MR::isSensorEnemyAttack(pSender)) {
        if ((!isEnableAttack() && MR::isSensorPlayer(pReceiver)) || MR::isSensorEnemy(pReceiver)) {
            if (MR::sendMsgPushAndKillVelocityToTarget(this, pReceiver, pSender)) {
                return;
            }
        }
    }

    if (isEnableAttack() && MR::isSensorPlayer(pReceiver) && MR::isSensorEnemyAttack(pSender)) {
        if (!MR::isPlayerHipDropFalling() && MR::sendMsgEnemyAttack(pReceiver, pSender)) {
            requestAttackSuccess();
        } else {
            MR::sendMsgPush(pReceiver, pSender);
        }
    }
}

bool KuriboMini::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    if (MR::isMsgLockOnStarPieceShoot(msg)) {
        return true;
    }

    if (MR::isMsgStarPieceAttack(msg)) {
        return requestStagger(pSender, pReceiver);
    }

    if (MR::isMsgPlayerTrample(msg)) {
        if (requestFlatDown(pSender, pReceiver)) {
            mItemGenerator->setTypeCoin(1);
            return true;
        }
    }

    if (MR::isMsgInvincibleAttack(msg)) {
        if (requestBlowDown(pSender, pReceiver)) {
            mItemGenerator->setTypeCoin(1);
            return true;
        }
    }

    if (MR::isMsgPlayerHipDrop(msg)) {
        if (requestHipDropDown(pSender, pReceiver)) {
            mItemGenerator->setTypeCoin(1);
            return true;
        }
    }

    if (MR::isMsgPlayerHitAll(msg)) {
        if (requestBlowDown(pSender, pReceiver)) {
            mItemGenerator->setTypeCoin(1);
            return true;
        }
    }

    return false;
}

bool KuriboMini::receiveMsgEnemyAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    if (MR::isMsgToEnemyAttackTrample(msg)) {
        if (requestPressDown()) {
            mItemGenerator->setTypeStarPeace(3);
            return true;
        }
    }

    if (MR::isMsgToEnemyAttackShockWave(msg)) {
        return requestStagger(pSender, pReceiver);
    }

    if (MR::isMsgExplosionAttack(msg) || MR::isMsgToEnemyAttackBlow(msg)) {
        if (requestBlowDown(pSender, pReceiver)) {
            mItemGenerator->setTypeStarPeace(3);
            return true;
        }
    }

    return false;
}

bool KuriboMini::receiveMsgPush(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    if (MR::isSensorEnemyAttack(pReceiver)) {
        return false;
    }

    if (MR::isSensorEnemy(pSender) || MR::isSensorRide(pSender) || (!isEnableAttack() && MR::isSensorPlayer(pSender))) {
        if (!isDown()) {
            MR::addVelocityFromPush(this, 1.5f, pSender, pReceiver);
            return true;
        }
    }

    return false;
}

bool KuriboMini::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    if (MR::isMsgInhaleBlackHole(msg)) {
        mItemGenerator->setTypeNone();
        kill();
        return true;
    }

    if (MR::isMsgPlayerKick(msg) && isEnableKick()) {
        if (requestBlowDown(pSender, pReceiver)) {
            mItemGenerator->setTypeStarPeace(3);
            return true;
        }
    }

    return false;
}

bool KuriboMini::requestHipDropDown(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    if (MR::isSensorEnemyAttack(pReceiver)) {
        return false;
    }

    MR::startSound(this, "SE_EM_STOMPED_S", -1, -1);
    MR::startAction(this, "FlatDown");
    setNerve(&NrvKuriboMini::KuriboMiniNrvHipDropDown::sInstance);
    MR::offBind(this);
    return true;
}

bool KuriboMini::requestFlatDown(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    MR::startSound(this, "SE_EM_STOMPED_S", -1, -1);
    MR::startAction(this, "FlatDown");
    setNerve(&NrvKuriboMini::KuriboMiniNrvFlatDown::sInstance);
    MR::offBind(this);
    return true;
}

bool KuriboMini::requestPressDown() {
    if (isDown()) {
        return false;
    }

    MR::startSound(this, "SE_EM_STOMPED_S", -1, -1);
    MR::startAction(this, "FlatDown");
    setNerve(&NrvKuriboMini::KuriboMiniNrvPressDown::sInstance);
    MR::offBind(this);
    return true;
}

bool KuriboMini::requestBlowDown(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    if (!isDown()) {
        MR::setVelocityBlowAttack(this, pSender, pReceiver, 22.0f, 25.0f, 4);
        setNerve(&NrvKuriboMini::KuriboMiniNrvBlowDown::sInstance);
        return true;
    }

    return false;
}

bool KuriboMini::requestStagger(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    if (!isDown()) {
        mStateStagger->setPunchDirection(pSender, pReceiver);
        setNerve(&NrvKuriboMini::KuriboMiniNrvStagger::sInstance);
        return true;
    }

    return false;
}

bool KuriboMini::requestAttackSuccess() {
    if (isDown()) {
        return false;
    }

    if (isEnableAttack()) {
        setNerve(&NrvKuriboMini::KuriboMiniNrvAttackSuccess::sInstance);
        return true;
    }

    return false;
}

bool KuriboMini::tryFind() {
    if (mStateFindPlayer->isInSightPlayer()) {
        setNerve(&NrvKuriboMini::KuriboMiniNrvFindPlayer::sInstance);
        return true;
    }

    return false;
}

bool KuriboMini::tryPointBind() {
    if (mBindStarPointer->tryStartPointBind()) {
        setNerve(&NrvKuriboMini::KuriboMiniNrvBindStarPointer::sInstance);
        return true;
    }

    return false;
}

bool KuriboMini::tryDeadMap() {
    if (MR::isInDeath(this, TVec3f(0.0f, 0.0f, 0.0f)) || MR::isBindedGroundDamageFire(this) || MR::isInWater(mPosition)) {
        mItemGenerator->setTypeNone();
        kill();
        return true;
    }

    return false;
}

void KuriboMini::exeWander() {
    MR::updateActorState(this, mStateWander);
    if (!tryFind() && !tryDeadMap()) {
        tryPointBind();
    }
}

void KuriboMini::exeFindPlayer() {
    if (!MR::updateActorStateAndNextNerve(this, mStateFindPlayer, &NrvKuriboMini::KuriboMiniNrvChase::sInstance)) {
        if (mStateFindPlayer->isFindJumpBegin()) {
            MR::startSound(this, "SE_EM_KURIBOMINI_FIND", -1, -1);
        }

        if (!tryDeadMap()) {
            tryPointBind();
        }
    }
}

void KuriboMini::exeChase() {
    if (MR::updateActorStateAndNextNerve(this, mStateChase, &NrvKuriboMini::KuriboMiniNrvWander::sInstance)) {
        mStateWander->setWanderCenter(mPosition);
    }

    if (mStateChase->isRunning()) {
        MR::startLevelSound(this, "SE_EM_LV_KURIBOMINI_DASH", -1, -1, -1);
    }

    if (!tryDeadMap()) {
        tryPointBind();
    }
}

void KuriboMini::exeStagger() {
    if (!MR::updateActorStateAndNextNerve(this, mStateStagger, &NrvKuriboMini::KuriboMiniNrvWander::sInstance)) {
        if (mStateStagger->isStaggerStart()) {
            MR::startSound(this, "SE_EM_CRASH_S", -1, -1);
            MR::startBlowHitSound(this);
        }

        if (mStateStagger->isSwooning(15)) {
            MR::startLevelSound(this, "SE_EM_LV_SWOON_S", -1, -1, -1);
        }

        if (mStateStagger->isRecoverStart()) {
            MR::startSound(this, "SE_EM_KURIBO_SWOON_RECOVER", -1, -1);
        }

        tryDeadMap();
    }
}

void KuriboMini::exeAttackSuccess() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Hit");
    }

    MR::turnDirectionToPlayerDegree(this, &_B8, 5.0f);
    calcPassiveMovement();
    if (tryDeadMap()) {
        return;
    }

    if (tryPointBind()) {
        return;
    }

    if (MR::isGreaterStep(this, 60)) {
        setNerve(&NrvKuriboMini::KuriboMiniNrvWander::sInstance);
    }
}

void KuriboMini::exeHipDropDown() {
    if (MR::isFirstStep(this)) {
        MR::startSound(this, "SE_EM_CRASH_S", -1, -1);
        MR::zeroVelocity(this);
    }

    if (MR::isGreaterStep(this, 40)) {
        kill();
    }
}

void KuriboMini::exeFlatDown() {
    if (MR::isFirstStep(this)) {
        MR::startSound(this, "SE_EM_CRASH_S", -1, -1);
        MR::zeroVelocity(this);
    }

    if (MR::isGreaterStep(this, 20)) {
        kill();
    }
}

void KuriboMini::exePressDown() {
    if (MR::isFirstStep(this)) {
        MR::startSound(this, "SE_EM_CRASH_S", -1, -1);
        MR::zeroVelocity(this);
    }

    if (MR::isGreaterStep(this, 180)) {
        kill();
    }
}

void KuriboMini::exeBlowDown() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "BlowDown");
        MR::startSound(this, "SE_EM_CRASH_S", -1, -1);
        MR::startBlowHitSound(this);
    }

    calcPassiveMovement();
    TVec3f invVelocity;
    JMathInlineVEC::PSVECNegate(&mVelocity, &invVelocity);
    MR::turnDirectionDegree(this, &_B8, invVelocity, 30.0f);
    if (MR::isGreaterStep(this, 30)) {
        kill();
    }
}

void KuriboMini::calcPassiveMovement() {
    if (!MR::isOnGround(this)) {
        MR::addVelocityToGravity(this, 1.5f);
    }

    f32 friction;
    if (MR::isOnGround(this)) {
        friction = 0.93f;
    } else {
        friction = 0.99f;
    }

    MR::attenuateVelocity(this, friction);
    if (MR::isBindedWall(this)) {
        MR::calcReboundVelocity(&mVelocity, *MR::getWallNormal(this), 0.2f, 0.7f);
    }
}

bool KuriboMini::isEnableAttack() const {
    if (isNerve(&NrvKuriboMini::KuriboMiniNrvWander::sInstance) || isNerve(&NrvKuriboMini::KuriboMiniNrvFindPlayer::sInstance) ||
        isNerve(&NrvKuriboMini::KuriboMiniNrvChase::sInstance)) {
        return true;
    }

    return false;
}

bool KuriboMini::isEnableKick() const {
    return isNerve(&NrvKuriboMini::KuriboMiniNrvStagger::sInstance);
}

bool KuriboMini::isDown() const {
    if (isNerve(&NrvKuriboMini::KuriboMiniNrvFlatDown::sInstance) || isNerve(&NrvKuriboMini::KuriboMiniNrvHipDropDown::sInstance) ||
        isNerve(&NrvKuriboMini::KuriboMiniNrvPressDown::sInstance) || isNerve(&NrvKuriboMini::KuriboMiniNrvBlowDown::sInstance)) {
        return true;
    }

    return false;
}

KuriboMini::~KuriboMini() {
}
