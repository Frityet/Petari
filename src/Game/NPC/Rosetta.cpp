#include "Game/NPC/Rosetta.hpp"
#include "Game/Demo/AstroDemoFunction.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NPC/RosettaDemoAstroDome.hpp"
#include "Game/NPC/RosettaDemoEpilogue.hpp"
#include "Game/NPC/TalkMessageFunc.hpp"
#include "Game/System/NerveExecutor.hpp"
#include "Game/Util.hpp"

class RosettaDemoHeavensDoor1 : public NerveExecutor {
public:
    RosettaDemoHeavensDoor1(Rosetta*, const JMapInfoIter&);

    static void makeArchiveList(NameObjArchiveListCollector*, const JMapInfoIter&);

private:
    u8 _8[0xC];
};

class RosettaDemoHeavensDoor2 : public NerveExecutor {
public:
    RosettaDemoHeavensDoor2(Rosetta*, const JMapInfoIter&);

    static void makeArchiveList(NameObjArchiveListCollector*, const JMapInfoIter&);

private:
    u8 _8[0x10];
};

class TurnJointCtrl {
public:
    enum AXIS {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2
    };

    TurnJointCtrl(LiveActor*);

    void init(f32, f32, f32);
    void addFace(const char*, f32, AXIS, AXIS, AXIS);
    void addWaist(const char*, f32, AXIS, AXIS, AXIS);
    void startCtrl(s32);
    void validate();
    void invalidate();
    void setStarePos(const TVec3f&);
    void update();
    void setCallBackFunction();

private:
    u8 _0[0x6C];
};

namespace MR {
    bool calcPlayerFaceStarePos(TVec3f*, MtxPtr, MtxPtr);
    void setNextStageBGM(const char*);
};  // namespace MR

namespace NrvRosetta {
    NEW_NERVE(RosettaNrvDemo, Rosetta, Demo);
    NEW_NERVE(RosettaNrvReaction, Rosetta, Reaction);
};  // namespace NrvRosetta

Rosetta::Rosetta(const char* pName) : NPCActor(pName), _15C(this, -1), _170(nullptr), _174(nullptr), _17C(-1) {}

void Rosetta::makeArchiveList(NameObjArchiveListCollector* pArchiveList, const JMapInfoIter& rIter) {
    pArchiveList->addArchive("Rosetta");
    pArchiveList->addArchive("RosettaMiddle");
    pArchiveList->addArchive("RosettaLow");

    s32 type;
    MR::getJMapInfoArg0WithInit(rIter, &type);

    switch (type) {
    case 0:
        RosettaDemoHeavensDoor1::makeArchiveList(pArchiveList, rIter);
        break;
    case 1:
        RosettaDemoHeavensDoor2::makeArchiveList(pArchiveList, rIter);
        break;
    case 2:
        RosettaDemoAstroDomeExplain::makeArchiveList(pArchiveList, rIter);
        break;
    default:
        break;
    }
}

void Rosetta::init(const JMapInfoIter& rIter) {
    NPCActorCaps caps("Rosetta");
    caps.setDefault();
    caps.setIndirect();
    caps._5D = true;
    caps.mSensor = false;
    caps.mMakeActor = false;
    caps.mBinder = false;
    caps.mMessageOffset.x = 0.0f;
    caps.mMessageOffset.y = 0.0f;
    caps.mMessageOffset.z = 0.0f;
    caps.mTalkJointName = "Chin";
    caps.mReactionNerve = &NrvRosetta::RosettaNrvReaction::sInstance;
    initialize(rIter, caps);

    initHitSensor(2);
    MR::addHitSensorNpc(this, "Head", 8, 70.0f, TVec3f(0.0f, 160.0f, 0.0f));
    MR::addHitSensorNpc(this, "Body", 8, 80.0f, TVec3f(0.0f, 50.0f, 0.0f));

    if (mMsgCtrl != nullptr) {
        TalkMessageCtrl* msgCtrl = mMsgCtrl;
        MR::registerBranchFunc(msgCtrl, TalkMessageFunc< Rosetta >(this, &Rosetta::branchFunc));
        MR::registerEventFunc(msgCtrl, TalkMessageFunc< Rosetta >(this, &Rosetta::eventFunc));
        MR::onStartOnlyFront(msgCtrl);
    }

    MR::startBrk(this, "Normal");
    MR::getJMapInfoArg0NoInit(rIter, &_17C);
    AstroDemoFunction::tryRegisterAstroDemoAll(this, rIter);
    MR::tryRegisterDemoCast(this, "赤いスター", rIter);
    MR::tryRegisterDemoCast(this, "チコガイドデモ", rIter);
    MR::tryRegisterDemoCast(this, "エピローグデモ", rIter);
    AstroDemoFunction::tryRegisterSimpleCastIfAstroGalaxy(this);

    _178 = new TurnJointCtrl(this);
    _178->init(40.0f, 0.0f, 5.0f);
    _178->addFace("Head", 0.6f, TurnJointCtrl::AXIS_Z, TurnJointCtrl::AXIS_Y, TurnJointCtrl::AXIS_X);
    _178->addWaist("Spine2", 0.4f, TurnJointCtrl::AXIS_Z, TurnJointCtrl::AXIS_X, TurnJointCtrl::AXIS_Y);

    initAfterPlacement();

    if (MR::isDemoCast(this, "チコガイドデモ")) {
        _170 = new RosettaDemoHeavensDoor1(this, rIter);
    } else if (MR::isDemoCast(this, "赤いスター")) {
        _170 = new RosettaDemoHeavensDoor2(this, rIter);
    } else if (MR::isDemoCast(this, "ロゼッタ状況説明デモ")) {
        _170 = new RosettaDemoAstroDomeExplain(this, rIter);
    } else if (MR::isDemoCast(this, "エピローグデモ")) {
        _170 = reinterpret_cast< NerveExecutor* >(new RosettaDemoEpilogue(this, rIter));
    } else if (MR::isDemoCast(this, "ロゼッタ最終決戦デモ")) {
        _170 = new RosettaDemoAstroDomeFinalBattle(this, rIter);
    }

    _174 = new RosettaDemoAstroDomeTalk(this, rIter);
    MR::startBckNoInterpole(this, "WaitA");
    MR::calcAnimDirect(this);

    if (mMsgCtrl != nullptr) {
        MR::setDistanceToTalk(mMsgCtrl, 200.0f);
    }

    MR::useStageSwitchWriteB(this, rIter);
    mParam._14 = "WaitA";
    mParam._1C = "TalkA";
    mParam._18 = nullptr;
    mParam._20 = nullptr;
    mParam._0 = false;
    mParam._1 = false;
    _130 = "Spin";
    _134 = "Trampled";
    _138 = "Pointing";
    _13C = "Reaction";
    _12C = 1500.0f;
}

void Rosetta::calcAndSetBaseMtx() {
    _178->setCallBackFunction();
    NPCActor::calcAndSetBaseMtx();
}

bool Rosetta::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    return NPCActor::receiveMsgPlayerAttack(msg, pSender, pReceiver);
}

void Rosetta::control() {
    if (canUpdateStarePos()) {
        TVec3f starePos;
        MR::calcPlayerFaceStarePos(&starePos, MR::getJointMtx(this, "Head"), getBaseMtx());
        _178->setStarePos(starePos);
    }

    _178->update();

    if (mMsgCtrl != nullptr && MR::isTalkStart(mMsgCtrl)) {
        switch (MR::getRandom((s32)0, (s32)3)) {
        case 0:
            mParam._1C = "TalkA";
            break;
        case 1:
            mParam._1C = "TalkB";
            break;
        case 2:
            mParam._1C = "TalkC";
            break;
        default:
            break;
        }
    }

    if (MR::isIntervalStep(this, 300)) {
        switch (MR::getRandom((s32)0, (s32)2)) {
        case 0:
            mParam._14 = "WaitA";
            break;
        case 1:
            mParam._14 = "WaitB";
            break;
        default:
            break;
        }
    }

    NPCActor::control();
}

bool Rosetta::branchFunc(u32) {
    return false;
}

bool Rosetta::eventFunc(u32 event) {
    switch (event) {
    case 0:
        if (!MR::isPlayingStageBgmName("BGM_FLYING_A")) {
            MR::setNextStageBGM("BGM_SENARIO_SEL_3");
            MR::stopStageBGM(30);
        }
        break;
    case 1:
        MR::onGameEventFlagGalaxyOpen("KoopaBattleVs3Galaxy");
        if (!isEmptyNerve()) {
            popNerve();
        }
        tryPushNullNerve();
        break;
    case 2:
        if (!MR::isPlayingStageBgmName("BGM_FLYING_A")) {
            MR::setNextStageBGM("STM_ASTRO_OUT_3");
            MR::stopStageBGM(60);
        }
        break;
    case 4:
        if (_15C.update()) {
            MR::onSwitchA(this);
            return true;
        }
        return false;
    default:
        break;
    }

    return true;
}

void Rosetta::startDemo(NerveExecutor* pExecutor) {
    _170 = pExecutor;
    pushNerve(&NrvRosetta::RosettaNrvDemo::sInstance);
    _178->invalidate();
}

void Rosetta::endDemo() {
    MR::startBckNoInterpole(this, "WaitA");
    _170 = nullptr;
    setToDefault();
    popNerve();
    _178->validate();
}

void Rosetta::endDemoWithInterpole() {
    _170 = nullptr;
    popNerve();
    _178->startCtrl(60);
}

bool Rosetta::canUpdateStarePos() const {
    if (isNerve(&NrvRosetta::RosettaNrvReaction::sInstance)) {
        return false;
    }

    TVec3f playerDir(*MR::getPlayerPos() - mPosition);

    TVec3f upDir;
    MR::extractMtxYDir(getBaseMtx(), &upDir);

    if (MR::normalizeOrZero(&playerDir)) {
        return false;
    }

    return MR::vecKillElement(playerDir, upDir, &playerDir) <= 0.95f;
}

void Rosetta::exeDemo() {
    _170->updateNerve();
}

void Rosetta::exeReaction() {
    MR::isFirstStep(this);

    if (_D8) {
        MR::startSound(this, "SE_SM_ROSETTA_BARRIER", -1, -1);
    }

    if (isPointingSe()) {
        MR::startDPDHitSound();
        MR::startSound(this, "SE_SV_ROSETTA_POINT", -1, -1);
    }

    if (_D9) {
        MR::startSound(this, "SE_SV_ROSETTA_SPIN", -1, -1);
    }

    if (_DB) {
        MR::startSound(this, "SE_SV_ROSETTA_STAR_PIECE_HIT", -1, -1);
    }

    MR::tryStartReactionAndPopNerve(this);
}
