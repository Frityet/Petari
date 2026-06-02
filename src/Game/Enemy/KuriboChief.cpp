#include "Game/Enemy/KuriboChief.hpp"
#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/Enemy/ItemGenerator.hpp"
#include "Game/Enemy/WalkerStateBindStarPointer.hpp"
#include "Game/Enemy/WalkerStateChase.hpp"
#include "Game/Enemy/WalkerStateFindPlayer.hpp"
#include "Game/Enemy/WalkerStateFunction.hpp"
#include "Game/Enemy/WalkerStateParam.hpp"
#include "Game/Enemy/WalkerStateStagger.hpp"
#include "Game/Enemy/WalkerStateWander.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/MapObj/KeySwitch.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/Util.hpp"
#include "revolution/mtx.h"

namespace {
    class KuriboChiefParam {
    public:
        KuriboChiefParam();

        WalkerStateParam mStateParam;
        WalkerStateParam mStaggerStateParam;
        WalkerStateStaggerParam mStaggerParam;
        WalkerStateFindPlayerParam mFindPlayerParam;
        WalkerStateChaseParam mChaseParam;
        WalkerStateWanderParam mWanderParam;
        AnimScaleParam mAnimScaleParam;
    };

    KuriboChiefParam::KuriboChiefParam() {
        mStateParam.mGravityAccel = 1.5f;
        mStateParam.mAirFriction = 0.99f;
        mStateParam.mGroundFriction = 0.93f;
        mStateParam.mPlayerNearDistance = 1400.0f;
        mStateParam.mPlayerSightFanDegreeH = 80.0f;
        mStateParam.mPlayerSightFanDegreeV = 40.0f;

        mWanderParam.mSpeed = 0.1f;
        mWanderParam.mWaitTime = 120;
        mWanderParam.mWalkTime = 300;
        mWanderParam.mTurnMaxRateDegree = 1.0f;

        mChaseParam.mChaseSpeed = 0.2f;

        mStaggerStateParam.mGravityAccel = 1.5f;
        mStaggerStateParam.mAirFriction = 0.99f;
        mStaggerStateParam.mGroundFriction = 0.6f;

        mStaggerParam.mStaggerTime = 240;
        mStaggerParam.mRotateRateDegree = 0.0f;

        mFindPlayerParam.mJumpStartStep = 56;
        mFindPlayerParam.mJumpVelocity = 35.0f;
        mFindPlayerParam.mTurnMaxRateDegree = 2.0f;

        mAnimScaleParam._0 = 0.1f;
        mAnimScaleParam._10 = 30.0f;
        mAnimScaleParam._14 = 0.6f;
        mAnimScaleParam._18 = 0.06f;
        mAnimScaleParam._1C = 3.0f;
        mAnimScaleParam._24 = 0.1f;
    }

    KuriboChiefParam sParam;
};

namespace NrvKuriboChief {
    NEW_NERVE(KuriboChiefNrvWander, KuriboChief, Wander);
    NEW_NERVE(KuriboChiefNrvFindPlayer, KuriboChief, FindPlayer);
    NEW_NERVE(KuriboChiefNrvChase, KuriboChief, Chase);
    NEW_NERVE(KuriboChiefNrvAttackSuccess, KuriboChief, AttackSuccess);
    NEW_NERVE(KuriboChiefNrvStagger, KuriboChief, Stagger);
    NEW_NERVE(KuriboChiefNrvTrample, KuriboChief, Trample);
    NEW_NERVE_ONEND(KuriboChiefNrvBindStarPointer, KuriboChief, BindStarPointer, BindStarPointer);
    NEW_NERVE(KuriboChiefNrvBlowDown, KuriboChief, BlowDown);
    NEW_NERVE(KuriboChiefNrvBlowDownLand, KuriboChief, BlowDownLand);
};  // namespace NrvKuriboChief

KuriboChief::KuriboChief(const char* pName)
    : LiveActor(pName), mScaleController(nullptr), mStateWander(nullptr), mStateFindPlayer(nullptr), mStateChase(nullptr),
      mStateStagger(nullptr), mBindStarPointer(nullptr), mItemGenerator(nullptr), mKeySwitch(nullptr), mBaseQuat(0.0f, 1.0f),
      mFrontVec(0.0f, 0.0f, 1.0f) {
}

void KuriboChief::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    initModelManagerWithAnm("KuriboChief", nullptr, false);
    MR::connectToSceneEnemy(this);
    MR::initLightCtrl(this);
    MR::onCalcGravity(this);

    mItemGenerator = new ItemGenerator();

    initEffectKeeper(1, nullptr, false);
    MR::initShadowFromCSV(this, "Shadow");

    mScaleController = new AnimScaleController(&sParam.mAnimScaleParam);

    TVec3f starPointerOffset(350.0f, 0.0f, 0.0f);
    MR::initStarPointerTargetAtJoint(this, "Body", 350.0f, starPointerOffset);

    initSensor();
    initBinder(300.0f, 300.0f, 0);
    initNerve(&NrvKuriboChief::KuriboChiefNrvWander::sInstance);
    initSound(6, false);
    initState();
    initKeySwitch(rIter);
    MR::useStageSwitchWriteDead(this, rIter);
    MR::useStageSwitchSleep(this, rIter);
    MR::addBaseMatrixFollowTarget(this, rIter, nullptr, nullptr);
    MR::declareStarPiece(this, 6);

    s32 cameraID = -1;
    MR::getJMapInfoArg7WithInit(rIter, &cameraID);
    if (cameraID != -1) {
        MR::declareCameraRegisterVec(this, cameraID, &mPosition);
    }

    if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::syncStageSwitchAppear(this);
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

void KuriboChief::initAfterPlacement() {
    MR::trySetMoveLimitCollision(this);
}

void KuriboChief::initSensor() {
    initHitSensor(8);
    MR::addHitSensorAtJointEnemy(this, "Body", "Body", 8, 150.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::addHitSensorAtJointEnemy(this, "Head1", "Head", 8, 300.0f, TVec3f(120.0f, 0.0f, 0.0f));
    MR::addHitSensorAtJointEnemy(this, "Head2", "Head", 8, 150.0f, TVec3f(360.0f, 0.0f, 0.0f));
    MR::addHitSensorAtJointEnemy(this, "LegL", "LegL", 8, 130.0f, TVec3f(60.0f, -50.0f, 0.0f));
    MR::addHitSensorAtJointEnemy(this, "LegR", "LegR", 8, 130.0f, TVec3f(60.0f, -50.0f, 0.0f));
    MR::addHitSensorEnemy(this, "Punch", 8, 400.0f, TVec3f(0.0f, 150.0f, 0.0f));
}

void KuriboChief::initState() {
    mStateFindPlayer = new WalkerStateFindPlayer(this, &mFrontVec, &sParam.mStateParam, &sParam.mFindPlayerParam);
    mStateWander = new WalkerStateWander(this, &mFrontVec, &sParam.mStateParam, &sParam.mWanderParam);
    mStateChase = new WalkerStateChase(this, &mFrontVec, &sParam.mStateParam, &sParam.mChaseParam);
    mStateStagger = new WalkerStateStagger(this, &mFrontVec, &sParam.mStaggerStateParam, &sParam.mStaggerParam);
    mBindStarPointer = new WalkerStateBindStarPointer(this, mScaleController);
}

void KuriboChief::initKeySwitch(const JMapInfoIter& rIter) {
    if (MR::useStageSwitchWriteA(this, rIter)) {
        mKeySwitch = new KeySwitch("鍵スイッチ");
        mKeySwitch->initKeySwitchByOwner(rIter);
    }
}

void KuriboChief::makeArchiveList(NameObjArchiveListCollector* pArchiveList, const JMapInfoIter& rIter) {
    if (MR::isExistStageSwitchA(rIter)) {
        pArchiveList->addArchive("KeySwitch");
    }
}

void KuriboChief::kill() {
    if (MR::isValidSwitchDead(this)) {
        MR::onSwitchDead(this);
    }

    if (mKeySwitch != nullptr) {
        mKeySwitch->appearKeySwitch(mPosition);
    } else {
        mItemGenerator->generate(this);
    }

    LiveActor::kill();
}

void KuriboChief::control() {
    if (mScaleController != nullptr) {
        mScaleController->updateNerve();
    }

    MR::blendQuatFromGroundAndFront(&mBaseQuat, this, mFrontVec, 0.05f, 0.5f);
}

void KuriboChief::calcAndSetBaseMtx() {
    MR::setBaseTRMtx(this, mBaseQuat);

    TVec3f scale;
    JMathInlineVEC::PSVECMultiply(&mScaleController->_C, &mScale, &scale);
    MR::setBaseScale(this, scale);
}

void KuriboChief::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (pSender == getSensor("Punch")) {
        return;
    }

    if ((!isEnableAttack() && MR::isSensorPlayer(pReceiver)) || MR::isSensorEnemy(pReceiver)) {
        if (MR::sendMsgPushAndKillVelocityToTarget(this, pReceiver, pSender)) {
            return;
        }
    }

    if (isEnableAttack() && MR::isSensorPlayer(pReceiver)) {
        if (!MR::sendMsgEnemyAttack(pReceiver, pSender)) {
            MR::sendMsgPush(pReceiver, pSender);
        }
    }
}

bool KuriboChief::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (pReceiver != getSensor("Punch")) {
        if (MR::isMsgStarPieceReflect(msg)) {
            mScaleController->startHitReaction();
            return true;
        }

        if (MR::isMsgInvincibleAttack(msg)) {
            return requestBlowDown(pSender, pReceiver);
        }

        return false;
    }

    if (MR::isMsgPlayerHitAll(msg)) {
        if (isEnableKick()) {
            return requestBlowDown(pSender, pReceiver);
        }

        return requestStagger(pSender, pReceiver);
    }

    return false;
}

bool KuriboChief::receiveMsgEnemyAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (pReceiver == getSensor("Punch")) {
        return false;
    }

    if (MR::isMsgToEnemyAttackBlow(msg)) {
        return requestStagger(pSender, pReceiver);
    }

    if (MR::isMsgToEnemyAttackShockWave(msg)) {
        return requestStagger(pSender, pReceiver);
    }

    return false;
}

bool KuriboChief::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (pReceiver == getSensor("Punch")) {
        return false;
    }

    if (MR::isMsgInhaleBlackHole(msg)) {
        mItemGenerator->setTypeNone();
        kill();
        return true;
    }

    if (MR::isMsgPlayerKick(msg) && MR::isSensorPlayer(pSender) && isEnableKick() && requestBlowDown(pSender, pReceiver)) {
        mItemGenerator->setTypeStarPeace(6);
        return true;
    }

    return false;
}

bool KuriboChief::requestStagger(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    mStateStagger->setPunchDirection(pSender, pReceiver);
    setNerve(&NrvKuriboChief::KuriboChiefNrvStagger::sInstance);
    return true;
}

bool KuriboChief::requestBlowDown(HitSensor* pSender, HitSensor* pReceiver) {
    if (isDown()) {
        return false;
    }

    MR::setVelocityBlowAttack(this, pSender, pReceiver, 25.0f, 35.0f, 4);
    setNerve(&NrvKuriboChief::KuriboChiefNrvBlowDown::sInstance);
    return true;
}

bool KuriboChief::tryFind() {
    if (mStateFindPlayer->isInSightPlayer()) {
        setNerve(&NrvKuriboChief::KuriboChiefNrvFindPlayer::sInstance);
        return true;
    }

    return false;
}

bool KuriboChief::tryPointBind() {
    if (mBindStarPointer->tryStartPointBind()) {
        setNerve(&NrvKuriboChief::KuriboChiefNrvBindStarPointer::sInstance);
        return true;
    }

    return false;
}

void KuriboChief::exeWander() {
    MR::updateActorState(this, mStateWander);
    if (!tryFind()) {
        tryPointBind();
    }
}

void KuriboChief::exeFindPlayer() {
    if (!MR::updateActorStateAndNextNerve(this, mStateFindPlayer, &NrvKuriboChief::KuriboChiefNrvChase::sInstance)) {
        if (mStateFindPlayer->isFindJumpBegin()) {
            MR::startSound(this, "SE_EM_KURIBOCHIEF_FIND", -1, -1);
        }

        if (mStateFindPlayer->isLandStart()) {
            MR::startSound(this, "SE_EM_KURIBOCHIEF_LAND", -1, -1);
        }

        tryPointBind();
    }
}

void KuriboChief::exeChase() {
    if (MR::updateActorStateAndNextNerve(this, mStateChase, &NrvKuriboChief::KuriboChiefNrvWander::sInstance)) {
        mStateWander->setWanderCenter(mPosition);
    }

    tryPointBind();
}

void KuriboChief::exeStagger() {
    if (!MR::updateActorStateAndNextNerve(this, mStateStagger, &NrvKuriboChief::KuriboChiefNrvWander::sInstance)) {
        if (mStateStagger->isStaggerStart()) {
            MR::startSound(this, "SE_EM_KURIBOCHIEF_BLOW", -1, -1);
            MR::startBlowHitSound(this);
        }

        if (MR::isStep(this, 27)) {
            MR::startSound(this, "SE_EM_KURIBOCHIEF_BOUND", -1, -1);
        }

        if (mStateStagger->isSwooning(56)) {
            MR::startLevelSound(this, "SE_EM_LV_SWOON_S", -1, -1, -1);
        }

        if (mStateStagger->isSpinning(10, 105)) {
            MR::startLevelSound(this, "SE_EM_LV_KURIBOCHIEF_STAGGER", -1, -1, -1);
        }

        if (mStateStagger->isRecoverStart()) {
            MR::startSound(this, "SE_EM_KURIBOCHIEF_RECOVER", -1, -1);
        }
    }
}

void KuriboChief::exeTrample() {
    if (MR::isFirstStep(this)) {
        MR::startSound(this, "SE_EM_KURIBOCHIEF_TRAMPLE", -1, -1);
    }

    if (MR::isGreaterStep(this, 30)) {
        setNerve(&NrvKuriboChief::KuriboChiefNrvWander::sInstance);
    }
}

void KuriboChief::exeAttackSuccess() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Hit");
    }

    MR::turnDirectionToPlayerDegree(this, &mFrontVec, 5.0f);
    WalkerStateFunction::calcPassiveMovement(this, &sParam.mStateParam);

    if (MR::isGreaterStep(this, 60)) {
        setNerve(&NrvKuriboChief::KuriboChiefNrvWander::sInstance);
    }

    tryPointBind();
}

void KuriboChief::exeBlowDown() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "BlowDown");
        MR::startBlowHitSound(this);
        MR::startSound(this, "SE_EM_KURIBOCHIEF_BLOW", -1, -1);
        MR::startSystemSE("SE_SY_VS_BOSS_LAST_HIT", -1, -1);
    }

    WalkerStateFunction::calcPassiveMovement(this, &sParam.mStateParam);

    TVec3f invVelocity;
    JMathInlineVEC::PSVECNegate(&mVelocity, &invVelocity);
    MR::turnDirectionDegree(this, &mFrontVec, invVelocity, 30.0f);

    if (MR::isGreaterStep(this, 5) && MR::isBindedGround(this)) {
        setNerve(&NrvKuriboChief::KuriboChiefNrvBlowDownLand::sInstance);
        MR::zeroVelocity(this);
    }
}

void KuriboChief::exeBlowDownLand() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "BlowDownLand");
        MR::startSound(this, "SE_EM_KURIBOCHIEF_BOUND_LAST", -1, -1);
    }

    if (MR::isBckStopped(this)) {
        MR::emitEffect(this, "Death");
        MR::startSound(this, "SE_EM_KURIBOCHIEF_RUN", -1, -1);
        kill();
    }
}

bool KuriboChief::isEnableAttack() const {
    if (isNerve(&NrvKuriboChief::KuriboChiefNrvWander::sInstance) || isNerve(&NrvKuriboChief::KuriboChiefNrvFindPlayer::sInstance) ||
        isNerve(&NrvKuriboChief::KuriboChiefNrvChase::sInstance)) {
        return true;
    }

    return false;
}

bool KuriboChief::isEnableKick() const {
    if (isNerve(&NrvKuriboChief::KuriboChiefNrvStagger::sInstance)) {
        return mStateStagger->isEnableKick();
    }

    return false;
}

bool KuriboChief::isDown() const {
    if (isNerve(&NrvKuriboChief::KuriboChiefNrvBlowDown::sInstance) || isNerve(&NrvKuriboChief::KuriboChiefNrvBlowDownLand::sInstance)) {
        return true;
    }

    return false;
}

void KuriboChief::exeBindStarPointer() {
    MR::updateActorStateAndNextNerve(this, mBindStarPointer, &NrvKuriboChief::KuriboChiefNrvWander::sInstance);
}

void KuriboChief::endBindStarPointer() {
    mBindStarPointer->kill();
}

KuriboChief::~KuriboChief() {
}
