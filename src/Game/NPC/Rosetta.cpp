#include "Game/NPC/Rosetta.hpp"
#include "Game/Demo/AstroDemoFunction.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NPC/RosettaDemoAstroDome.hpp"
#include "Game/NPC/RosettaDemoEpilogue.hpp"
#include "Game/NPC/RosettaDemoHeavensDoor.hpp"
#include "Game/NPC/TalkMessageFunc.hpp"
#include "Game/NPC/TurnJointCtrl.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/Util.hpp"

inline void Rosetta::exeDemo() {
    _170->updateNerve();
}

namespace NrvRosetta {
    NEW_NERVE(RosettaNrvDemo, Rosetta, Demo);
    NEW_NERVE(RosettaNrvReaction, Rosetta, Reaction);
};  // namespace NrvRosetta

Rosetta::Rosetta(const char* pName) : NPCActor(pName), _15C(this, -1), _170(nullptr), _174(nullptr), _17C(-1) {
}

void Rosetta::makeArchiveList(NameObjArchiveListCollector* pCollector, const JMapInfoIter& rIter) {
    pCollector->addArchive("Rosetta");
    pCollector->addArchive("RosettaMiddle");
    pCollector->addArchive("RosettaLow");

    s32 arg0;
    MR::getJMapInfoArg0WithInit(rIter, &arg0);
    switch (arg0) {
    case 0:
        RosettaDemoHeavensDoor1::makeArchiveList(pCollector, rIter);
        break;
    case 1:
        RosettaDemoHeavensDoor2::makeArchiveList(pCollector, rIter);
        break;
    case 2:
        RosettaDemoAstroDomeExplain::makeArchiveList(pCollector, rIter);
        break;
    }
}

void Rosetta::init(const JMapInfoIter& rIter) {
    NPCActorCaps caps("Rosetta");
    caps.setDefault();
    caps.setIndirect();
    caps.mSensor = false;
    caps.mMakeActor = false;
    caps.mBinder = false;
    caps.mMessageOffset.set(0.0f, 0.0f, 0.0f);
    caps.mTalkJointName = "Chin";
    caps._5D = true;
    caps.mReactionNerve = &NrvRosetta::RosettaNrvReaction::sInstance;
    initialize(rIter, caps);

    initHitSensor(2);
    MR::addHitSensorNpc(this, "Head", 8, 70.0f, TVec3f(0.0f, 160.0f, 0.0f));
    MR::addHitSensorNpc(this, "Body", 8, 80.0f, TVec3f(0.0f, 50.0f, 0.0f));

    if (mMsgCtrl != nullptr) {
        MR::registerBranchFunc(mMsgCtrl, TalkMessageFunc(this, &Rosetta::branchFunc));
        MR::registerEventFunc(mMsgCtrl, TalkMessageFunc(this, &Rosetta::eventFunc));
        MR::onStartOnlyFront(mMsgCtrl);
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
    _178->addFace("Head", 0.6f, TurnJointCtrl::Z, TurnJointCtrl::Y, TurnJointCtrl::X);
    _178->addWaist("Spine2", 0.4f, TurnJointCtrl::Z, TurnJointCtrl::X, TurnJointCtrl::Y);
    makeActorAppeared();

    if (MR::isDemoCast(this, "チコガイドデモ")) {
        _170 = new RosettaDemoHeavensDoor1(this, rIter);
    } else if (MR::isDemoCast(this, "赤いスター")) {
        _170 = new RosettaDemoHeavensDoor2(this, rIter);
    } else if (MR::isDemoCast(this, "ロゼッタ状況説明デモ")) {
        _170 = new RosettaDemoAstroDomeExplain(this, rIter);
    } else if (MR::isDemoCast(this, "エピローグデモ")) {
        _170 = new RosettaDemoEpilogue(this, rIter);
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
    mParam.setMoveTalkNoTurnAction("WaitA", "TalkA");
    setDefaults();
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
        switch (MR::getRandom(static_cast< s32 >(0), static_cast< s32 >(3))) {
        case 0:
            mParam._1C = "TalkA";
            break;
        case 1:
            mParam._1C = "TalkB";
            break;
        case 2:
            mParam._1C = "TalkC";
            break;
        }
    }

    if (MR::isIntervalStep(this, 300)) {
        switch (MR::getRandom(static_cast< s32 >(0), static_cast< s32 >(2))) {
        case 0:
            mParam._14 = "WaitA";
            break;
        case 1:
            mParam._14 = "WaitB";
            break;
        }
    }

    NPCActor::control();
}

bool Rosetta::branchFunc(u32 arg) {
    return false;
}

bool Rosetta::eventFunc(u32 arg) {
    switch (arg) {
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

    TVec3f dir(*MR::getPlayerPos() - mPosition);
    TVec3f up;
    MR::extractMtxYDir(getBaseMtx(), &up);
    if (MR::normalizeOrZero(&dir)) {
        return false;
    }

    return !(MR::vecKillElement(dir, up, &dir) > 0.95f);
}

void Rosetta::exeReaction() {
    MR::isFirstStep(this);
    if (_D8) {
        MR::startSound(this, "SE_SM_ROSETTA_BARRIER");
    }
    if (isPointingSe()) {
        MR::startDPDHitSound();
        MR::startSound(this, "SE_SV_ROSETTA_POINT");
    }
    if (_D9) {
        MR::startSound(this, "SE_SV_ROSETTA_SPIN");
    }
    if (_DB) {
        MR::startSound(this, "SE_SV_ROSETTA_STAR_PIECE_HIT");
    }
    MR::tryStartReactionAndPopNerve(this);
}
