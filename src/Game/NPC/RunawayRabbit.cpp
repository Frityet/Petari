#include "Game/NPC/RunawayRabbit.hpp"
#include "Game/Enemy/WalkerStateBlowDamage.hpp"
#include "Game/Enemy/WalkerStateRunaway.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/LiveActor/SpotMarkLight.hpp"
#include "Game/NPC/RunawayRabbitCollect.hpp"
#include "Game/NPC/TrickRabbitUtil.hpp"
#include "Game/Util.hpp"

template < typename T >
class BaseMatrixFollowValidateDelegator : public BaseMatrixFollowValidater {
public:
    BaseMatrixFollowValidateDelegator(T* pObj, bool (T::*pFunc)(s32) const) : mObj(pObj), mFunc(pFunc) {}

    virtual bool isValid(s32 id) const {
        return (mObj->*mFunc)(id);
    }

    T* mObj;
    bool (T::*mFunc)(s32) const;
};

namespace {
    class RunawayRabbitParam : public WalkerStateRunawayParam {
    public:
        RunawayRabbitParam();
    };

    RunawayRabbitParam sParam;

    RunawayRabbitParam::RunawayRabbitParam() : WalkerStateRunawayParam() {}
};  // namespace

namespace NrvRunawayRabbit {
    NERVE_DECL_NULL(RunawayRabbitNrvNoActive);
    NEW_NERVE(RunawayRabbitNrvHide, RunawayRabbit, Hide);
    NEW_NERVE(RunawayRabbitNrvAppear, RunawayRabbit, Appear);
    NEW_NERVE(RunawayRabbitNrvRunaway, RunawayRabbit, Runaway);
    NERVE_DECL_NULL(RunawayRabbitNrvTryCaughtDemo);
    NEW_NERVE(RunawayRabbitNrvCaught, RunawayRabbit, Caught);
    NEW_NERVE(RunawayRabbitNrvCaughtTalk, RunawayRabbit, CaughtTalk);
    NEW_NERVE(RunawayRabbitNrvCaughtEnd, RunawayRabbit, CaughtEnd);
    NEW_NERVE(RunawayRabbitNrvStop, RunawayRabbit, Stop);

    class RunawayRabbitNrvBlowDamage : public Nerve {
    public:
        virtual void execute(Spine*) const;
        static RunawayRabbitNrvBlowDamage sInstance;
    };

    RunawayRabbitNrvNoActive RunawayRabbitNrvNoActive::sInstance;
    RunawayRabbitNrvTryCaughtDemo RunawayRabbitNrvTryCaughtDemo::sInstance;
    RunawayRabbitNrvBlowDamage RunawayRabbitNrvBlowDamage::sInstance;

    void RunawayRabbitNrvBlowDamage::execute(Spine* pSpine) const {
        RunawayRabbit* actor = reinterpret_cast< RunawayRabbit* >(pSpine->mExecutor);
        MR::updateActorStateAndNextNerve(actor, actor->mStateBlowDamage, &RunawayRabbitNrvRunaway::sInstance);
    }
};  // namespace NrvRunawayRabbit

RunawayRabbit::RunawayRabbit(const char* pName, RunawayRabbitCollect* pCollector)
    : LiveActor(pName), mStateRunaway(nullptr), mStateBlowDamage(nullptr), mCollect(pCollector), mFootPrint(nullptr), mSpotMarkLight(nullptr),
      mMsgCtrl(nullptr), _A4(0, 0, 0, 1), _B4(0, 0, 1), _C0(0, 0, 0, 1), _D0(0, 0, 1), mObjArg0(-1), mRunawayLevel(0),
      mObjArg1(-1), _EC(0), mNotCaughtableTimer(0), _F4(true), _F5(false), mObjArg3(-1.0f) {}

void RunawayRabbit::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);

    s32 modelType = -1;
    MR::getJMapInfoArg2WithInit(rIter, &modelType);

    const char* modelName = modelType == 0 ? "TrickRabbitBaby" : "TrickRabbit";
    initModelManagerWithAnm(modelName, nullptr, false);
    MR::connectToSceneNpc(this);
    MR::initLightCtrl(this);
    MR::makeQuatAndFrontFromRotate(&_A4, &_B4, this);
    MR::onCalcGravity(this);
    MR::addBaseMatrixFollowTarget(this, rIter, nullptr, new BaseMatrixFollowValidateDelegator< RunawayRabbit >(this, &RunawayRabbit::isValidFollow));

    mStateRunaway = new WalkerStateRunaway(this, &_B4, &sParam);
    mStateBlowDamage = new WalkerStateBlowDamage(this, &_B4, nullptr);

    MR::getJMapInfoArg0WithInit(rIter, &mObjArg0);
    MR::getJMapInfoArg1WithInit(rIter, &mObjArg1);
    MR::getJMapInfoArg3WithInit(rIter, &mObjArg3);

    if (mObjArg3 <= 0.0f) {
        mObjArg3 = 600.0f;
    }

    s32 cameraRegisterId = -1;
    MR::getJMapInfoArg7WithInit(rIter, &cameraRegisterId);

    if (cameraRegisterId != -1) {
        MR::declareCameraRegisterVec(this, cameraRegisterId, &mPosition);
    }

    mSpotMarkLight = new SpotMarkLight(this, 100.0f, 1500.0f, nullptr);
    mSpotMarkLight->initWithoutIter();
    initSensor();
    initBinder(60.0f, 60.0f, 0);
    MR::onCalcGravity(this);
    MR::initShadowVolumeSphere(this, 45.0f);

    TVec3f shadowOffset(0.0f, 0.0f, 0.0f);
    MR::setShadowDropPositionAtJoint(this, nullptr, "Spine", shadowOffset);

    mFootPrint = TrickRabbitUtil::createRabbitFootPrint(this);
    initSound(6, false);
    initEffectKeeper(0, nullptr, false);
    MR::hideModel(this);
    MR::invalidateHitSensors(this);
    initNerve(&NrvRunawayRabbit::RunawayRabbitNrvNoActive::sInstance);

    if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::listenStageSwitchOnAppear(this, MR::Functor(this, &RunawayRabbit::startRunnaway));
    }

    makeActorAppeared();
}

void RunawayRabbit::initAfterPlacement() {
    MR::trySetMoveLimitCollision(this);
}

void RunawayRabbit::initSensor() {
    initHitSensor(2);
    MR::addHitSensorAtJointEnemy(this, "Body", "Spine", 8, 70.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::addHitSensorAtJointEnemy(this, "Catch", "Spine", 8, 30.0f, TVec3f(0.0f, 0.0f, 0.0f));
    MR::initStarPointerTargetAtJoint(this, "Spine", 70.0f, TVec3f(0, 0, 0));
}

void RunawayRabbit::appear() {
    LiveActor::appear();
    MR::emitEffect(this, "AppearSmoke");
}

void RunawayRabbit::control() {
    updatePose();

    if (mNotCaughtableTimer > 0) {
        mNotCaughtableTimer--;
    }

    if (MR::isBindedGroundWater(this)) {
        MR::setSeVersion(this, 1);
    }
    else {
        MR::setSeVersion(this, 0);
    }
}

void RunawayRabbit::calcAndSetBaseMtx() {
    MR::setBaseTRMtx(this, _A4);
}

void RunawayRabbit::updatePose() {
    TVec3f up = -mGravity;
    MR::blendQuatUpFront(&_A4, up, _B4, 0.1f, 0.2f);
}

void RunawayRabbit::updateBindActorMatrix() {
    TPos3f baseMtx;
    baseMtx.setQuat(_C0);
    baseMtx.setTrans(_D0);
    MR::setPlayerBaseMtx(baseMtx.toMtxPtr());
}

void RunawayRabbit::activate() {
    if (isNerve(&NrvRunawayRabbit::RunawayRabbitNrvNoActive::sInstance)) {
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvHide::sInstance);
    }
}

void RunawayRabbit::startRunnaway() {
    if (_F4) {
        MR::showModel(this);
        MR::invalidateClipping(this);
        MR::validateHitSensors(this);
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvAppear::sInstance);
        mCollect->noticeAppearRabbit(this);
    }
}

void RunawayRabbit::incrementRunawayLevel() {
    if (mRunawayLevel < 2) {
        mRunawayLevel++;
    }
}

void RunawayRabbit::setLastMessage() {
    MR::forwardNodeCurrentBranchLeft(mMsgCtrl);
}

void RunawayRabbit::setMessage() {
    MR::forwardNodeCurrentBranchRight(mMsgCtrl);
}

void RunawayRabbit::setNotCaughtable() {
    mNotCaughtableTimer = 5;
}

void RunawayRabbit::startJumpSound() {
    if (MR::isBindedGroundWater(this)) {
        MR::startSound(this, "SE_SM_RABBIT_JUMP_WATER", -1, -1);
    }
    else {
        MR::startSound(this, "SE_SM_RABBIT_JUMP", -1, -1);
    }
}

void RunawayRabbit::setMsgCtrl(TalkMessageCtrl* pMsgCtrl) {
    mMsgCtrl = pMsgCtrl;
}

void RunawayRabbit::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isSensorPlayer(pReceiver)) {
        if (pSender == getSensor("Catch") && isCaughtable()) {
            mCollect->noticeCaughtRabbit(this);
            MR::requestStartDemoMarioPuppetable(this, "捕まり", &NrvRunawayRabbit::RunawayRabbitNrvCaught::sInstance,
                                                &NrvRunawayRabbit::RunawayRabbitNrvTryCaughtDemo::sInstance);
        }
        else if (isCaught()) {
            MR::sendMsgPush(pReceiver, pSender);
        }
    }
    else {
        MR::sendMsgPushAndKillVelocityToTarget(this, pReceiver, pSender);
    }
}

bool RunawayRabbit::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isMsgPlayerTrample(msg) && isCaught()) {
        return true;
    }

    if (MR::isMsgLockOnStarPieceShoot(msg)) {
        return true;
    }

    if (MR::isMsgStarPieceAttack(msg) && isEnableBlow()) {
        MR::setVelocitySeparateHV(this, pSender, pReceiver, 20.0f, 20.0f);
        MR::limitedStarPieceHitSound();
        MR::startSound(this, "SE_SM_RABBIT_STAR_PIECE_HIT", -1, -1);
        MR::startSound(this, "SE_SV_RABBIT_STAR_PIECE_HIT", -1, -1);
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvBlowDamage::sInstance);
        return true;
    }

    return false;
}

bool RunawayRabbit::receiveMsgEnemyAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isMsgToEnemyAttackBlow(msg) && isEnableBlow()) {
        MR::setVelocitySeparateHV(this, pSender, pReceiver, 25.0f, 30.0f);
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvBlowDamage::sInstance);
        return true;
    }

    return false;
}

bool RunawayRabbit::receiveMsgPush(HitSensor* pSender, HitSensor* pReceiver) {
    if (!MR::isSensorPlayer(pSender) && isEnableBlow()) {
        MR::addVelocityFromPush(this, 0.5f, pSender, pReceiver);
        return true;
    }

    return false;
}

bool RunawayRabbit::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    return MR::isMsgTouchPlantItem(msg);
}

void RunawayRabbit::exeHide() {
    if (!_F4) {
        return;
    }

    f32 distance = MR::calcDistanceToPlayer(this);

    if (distance <= mObjArg3) {
        s32 volume = 100;
        s32 pitch = mObjArg1 == 0 ? 0 : 8;

        if (distance >= mObjArg3 - 250.0f) {
            volume = static_cast< s32 >(MR::getLinerValueFromMinMax(distance, mObjArg3 - 250.0f, mObjArg3, 100.0f, 35.0f));
        }

        MR::startLevelSound(this, "SE_SV_LV_RABBIT_NEAR2", volume, pitch, -1);

        if (mObjArg1 == 0) {
            MR::startLevelSound(this, "SE_SM_LV_RABBIT_RUS_HOLE", volume, pitch, -1);
        }
        else {
            MR::startLevelSound(this, "SE_SM_LV_RABBIT_RUS_LEAVES", volume, pitch, -1);
        }
    }
}

void RunawayRabbit::exeAppear() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Jump");

        if (mObjArg1 == 0) {
            MR::setVelocitySeparateHV(this, _B4, 19.0f, 25.0f);
        }
        else {
            TVec3f away = mPosition - *MR::getPlayerPos();
            MR::setVelocitySeparateHV(this, away, 16.0f, 25.0f);
        }

        MR::startSound(this, "SE_SM_RABBIT_APPEAR", -1, -1);
        MR::startSystemSE("SE_SM_RUNAWAY_RABBIT_APP_ME", -1, -1);
    }

    MR::turnDirectionDegree(this, &_B4, mVelocity, 20.0f);
    MR::addVelocityToGravity(this, 1.0f);
    MR::attenuateVelocity(this, 0.99f);

    if (MR::isGreaterStep(this, 5) && MR::isBindedGround(this)) {
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvRunaway::sInstance);
        _EC = 0;
    }
}

void RunawayRabbit::exeRunaway() {
    MR::updateActorState(this, mStateRunaway);

    if (!mStateRunaway->isRunning()) {
        return;
    }

    if (MR::checkPassBckFrame(this, 3.0f)) {
        startJumpSound();
    }

    f32 bckRate = 0.6f;

    if (MR::isNearPlayerHorizontal(this, 400.0f)) {
        if (_EC < 2000) {
            _EC++;
        }

        s32 runawayTime = _EC;
        s32 rampTime;

        switch (mRunawayLevel) {
        case 0:
            rampTime = 240;
            break;
        case 1:
            rampTime = 500;
            break;
        case 2:
            rampTime = 800;
            break;
        default:
            rampTime = 240;
            break;
        }

        f32 rate = MR::clamp(static_cast< f32 >(runawayTime - 200) / static_cast< f32 >(rampTime), 0.0f, 1.0f);
        bckRate = MR::getLinerValue(rate, 1.8f, 0.5f, 1.0f);

        if (bckRate < 1.3f) {
            MR::startAction(this, "RunTired");
        }
        else {
            MR::startAction(this, "Run");
        }
    }

    if (MR::isBindedGroundWater(this)) {
        bckRate *= 0.3f;
    }

    mStateRunaway->mRunawaySpeed = bckRate;
    MR::setBckRate(this, MR::clamp(bckRate, 0.9f, 1.4f));
}

void RunawayRabbit::exeCaught() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "TossStart");
        MR::startBckPlayer("TossStart", static_cast< const char* >(nullptr));
        MR::startSound(this, "SE_SM_RABBIT_CAUGHT", -1, -1);
        MR::startSoundPlayer("SE_PV_CATCH", -1);
        mSpotMarkLight->kill();
        MR::makeQuatRotateDegree(&_C0, *MR::getPlayerRotate());
        _D0.set(*MR::getPlayerPos());
    }

    f32 rate = MR::calcNerveRate(this, 5);
    _C0.slerp(_A4, rate);
    MR::vecBlend(_D0, mPosition, &_D0, rate);

    if (!MR::isBindedGround(this)) {
        MR::addVelocityToGravity(this, 2.0f);
        MR::attenuateVelocity(this, 0.99f);
    }
    else {
        MR::zeroVelocity(this);
    }

    updateBindActorMatrix();

    if (MR::isGreaterEqualStep(this, 7) && MR::isBindedGround(this)) {
        MR::zeroVelocity(this);
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvCaughtTalk::sInstance);
    }
}

void RunawayRabbit::exeCaughtTalk() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "TossWait");
        MR::startBckPlayer("TossWait", static_cast< const char* >(nullptr));
    }

    _D0.set(mPosition);
    MR::startLevelSound(this, "SE_SM_LV_RABBIT_STRUGGLE", -1, -1, -1);
    MR::zeroVelocity(this);
    updateBindActorMatrix();

    if (mMsgCtrl == nullptr || MR::tryTalkForceWithoutDemoMarioPuppetableAtEnd(mMsgCtrl)) {
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvCaughtEnd::sInstance);
    }
}

void RunawayRabbit::exeCaughtEnd() {
    if (MR::isFirstStep(this)) {
        MR::zeroVelocity(this);
        MR::startAction(this, "Toss");
        MR::startBckPlayer("Toss", static_cast< const char* >(nullptr));
    }

    if (MR::isStep(this, 10)) {
        MR::startSoundPlayer("SE_PV_THROW", -1);
    }

    updateBindActorMatrix();

    if (MR::isBckStopped(this)) {
        MR::endDemo(this, "捕まり");
        setNerve(&NrvRunawayRabbit::RunawayRabbitNrvStop::sInstance);
    }
}

void RunawayRabbit::exeStop() {
    if (MR::isFirstStep(this)) {
        kill();
        return;
    }

    MR::turnDirectionToPlayerDegree(this, &_B4, 3.0f);

    if (MR::isBindedGround(this)) {
        MR::zeroVelocity(this);
    }
    else {
        MR::attenuateVelocity(this, 0.99f);
        MR::addVelocityToGravity(this, 1.0f);
    }
}

bool RunawayRabbit::isCaught() const {
    return isNerve(&NrvRunawayRabbit::RunawayRabbitNrvStop::sInstance);
}

bool RunawayRabbit::isCaughtable() const {
    if (isRunnaway() && mNotCaughtableTimer == 0) {
        return true;
    }

    return false;
}

bool RunawayRabbit::isRunnaway() const {
    return !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvAppear::sInstance) &&
           !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvTryCaughtDemo::sInstance) &&
           !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvCaught::sInstance) &&
           !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvCaughtTalk::sInstance) &&
           !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvCaughtEnd::sInstance) &&
           !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvStop::sInstance);
}

bool RunawayRabbit::isChasing() const {
    return isNerve(&NrvRunawayRabbit::RunawayRabbitNrvAppear::sInstance) ||
           isNerve(&NrvRunawayRabbit::RunawayRabbitNrvRunaway::sInstance) ||
           isNerve(&NrvRunawayRabbit::RunawayRabbitNrvBlowDamage::sInstance);
}

bool RunawayRabbit::isEnableBlow() const {
    return isNerve(&NrvRunawayRabbit::RunawayRabbitNrvRunaway::sInstance);
}

bool RunawayRabbit::isValidFollow(s32) const {
    return !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvNoActive::sInstance) && !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvHide::sInstance) &&
           !isNerve(&NrvRunawayRabbit::RunawayRabbitNrvStop::sInstance);
}

RunawayRabbit::~RunawayRabbit() {}
