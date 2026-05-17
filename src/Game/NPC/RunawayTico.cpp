#include "Game/NPC/RunawayTico.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/Util.hpp"
#include <cstdio>

namespace NrvRunawayTico {
    NEW_NERVE(RunawayTicoNrvGuide0, RunawayTico, Guide0);
    NEW_NERVE(RunawayTicoNrvGuide1, RunawayTico, Guide1);
    NEW_NERVE(RunawayTicoNrvWhiteIn, RunawayTico, WhiteIn);
    NEW_NERVE(RunawayTicoNrvWhiteOut, RunawayTico, WhiteOut);

    class RunawayTicoNrvWait : public Nerve {
    public:
        virtual void execute(Spine*) const;
        static RunawayTicoNrvWait sInstance;
    };

    NEW_NERVE(RunawayTicoNrvAppear, RunawayTico, Appear);
    NEW_NERVE(RunawayTicoNrvTalk, RunawayTico, Talk);

    RunawayTicoNrvWait RunawayTicoNrvWait::sInstance;

    void RunawayTicoNrvWait::execute(Spine* pSpine) const {
        RunawayTico* actor = reinterpret_cast< RunawayTico* >(pSpine->mExecutor);

        if (MR::isFirstStep(actor)) {
            MR::startAction(actor, "Wait");
        }
    }
};  // namespace NrvRunawayTico

RunawayTico::RunawayTico(const char* pName) : Tico(pName) {
    mCameraInfo = nullptr;
    mMode = 0;
    mDemoCastID = 0;
    mIsStartRunaway = false;
    _19D = false;
}

RunawayTico::~RunawayTico() {}

void RunawayTico::makeArchiveList(NameObjArchiveListCollector* pArchiveList, const JMapInfoIter& rIter) {
    s32 arg0 = 0;
    MR::getJMapInfoArg0NoInit(rIter, &arg0);

    if (arg0 == -1) {
        pArchiveList->addArchive("TicoBaby");
    }
    else {
        pArchiveList->addArchive("Tico");
        pArchiveList->addArchive("TicoMiddle");
        pArchiveList->addArchive("TicoLow");
    }
}

void RunawayTico::init(const JMapInfoIter& rIter) {
    s32 baseType = 0;
    MR::getJMapInfoArg0NoInit(rIter, &baseType);

    s32 arg1 = 0;
    MR::getJMapInfoArg1NoInit(rIter, &arg1);
    mMode = arg1;

    if (MR::tryRegisterDemoCast(this, rIter)) {
        mDemoCastID = MR::getDemoCastID(rIter);

        if (mDemoCastID > 0) {
            mMode = 2;
            baseType = 0;
        }
        else {
            mMode = 1;
            baseType = -1;
        }
    }

    initBase(rIter, baseType);
    initMessage(rIter, "RunawayTico");

    if (mMode == 1) {
        mCameraInfo = MR::createActorCameraInfo(rIter);
        MR::initAnimCamera(this, mCameraInfo, "DemoMeetTico");
        MR::registerDemoActionFunctor(this, MR::Functor(this, &RunawayTico::setDemoTrans), "チコとの出会い[開始]");
        MR::registerDemoActionNerve(this, &NrvRunawayTico::RunawayTicoNrvGuide1::sInstance, "チコとの出会い[チコ変身]");
        MR::registerDemoActionFunctor(this, MR::Functor(this, &RunawayTico::startRunaway), "ウサギ追いかけ[開始]");
        setNerve(&NrvRunawayTico::RunawayTicoNrvGuide0::sInstance);
    }
    else if (mMode == 2) {
        MR::registerDemoActionFunctor(this, MR::Functor(this, &RunawayTico::setPosAllCaught), "高楼出現[フェードイン]");
        makeActorDead();
    }

    MR::offRootNodeAutomatic(mMsgCtrl);
    MR::useStageSwitchWriteA(this, rIter);
    MR::invalidateClipping(this);
    MR::onCalcGravity(this);
}

void RunawayTico::initAfterPlacement() {
    if (mMode == 1 && MR::isOnGameEventFlagEndTicoGuideDemo()) {
        makeActorDead();
    }
}

void RunawayTico::appearBushComment(const TVec3f& rPosition) {
    MR::forwardNodeNextBranchLeft(mMsgCtrl);
    appear();
    setPosAfterCaught(rPosition);
    MR::requestStartDemoMarioPuppetable(this, "ぼやき", &NrvRunawayTico::RunawayTicoNrvAppear::sInstance,
                                        &NrvRunawayTico::RunawayTicoNrvWait::sInstance);
}

void RunawayTico::appearHoleComment(const TVec3f& rPosition) {
    MR::forwardNodeNextBranchRight(mMsgCtrl);
    MR::forwardNodeCurrentBranchLeft(mMsgCtrl);
    appear();
    setPosAfterCaught(rPosition);
    MR::requestStartDemoMarioPuppetable(this, "ぼやき", &NrvRunawayTico::RunawayTicoNrvAppear::sInstance,
                                        &NrvRunawayTico::RunawayTicoNrvWait::sInstance);
}

void RunawayTico::appearPipeComment(const TVec3f& rPosition) {
    MR::forwardNodeNextBranchRight(mMsgCtrl);
    MR::forwardNodeCurrentBranchRight(mMsgCtrl);
    appear();
    setPosAfterCaught(rPosition);
    MR::requestStartDemoMarioPuppetable(this, "ぼやき", &NrvRunawayTico::RunawayTicoNrvAppear::sInstance,
                                        &NrvRunawayTico::RunawayTicoNrvWait::sInstance);
}

void RunawayTico::appearMamaComment(const TVec3f& rPosition) {
    _19D = true;
    MR::forwardNode(mMsgCtrl);
    appear();
    setPosAfterCaught(rPosition);
    MR::requestStartDemoMarioPuppetable(this, "ぼやき", &NrvRunawayTico::RunawayTicoNrvAppear::sInstance,
                                        &NrvRunawayTico::RunawayTicoNrvWait::sInstance);
}

void RunawayTico::setPosAfterCaught(const TVec3f& rPosition) {
    TVec3f playerFront;
    MR::getPlayerFrontVec(&playerFront);

    TVec3f front = -playerFront;
    TVec3f up = -mGravity;

    TPos3f mtx;
    MR::makeMtxUpFrontPos(&mtx, up, front, rPosition);
    setBaseMtx(mtx);
    setInitPose();
}

void RunawayTico::setPosAllCaught() {
    MR::forwardNode(mMsgCtrl);
    MR::onRootNodeAutomatic(mMsgCtrl);

    char posName[0x109];
    snprintf(posName, sizeof(posName), "TicoDemoPos%d", mDemoCastID + 1);
    MR::setNPCActorPos(this, posName);
    setInitPose();

    if (MR::isDead(this)) {
        makeActorAppeared();
    }

    setNerveWait();
}

bool RunawayTico::isStartRunaway() const {
    return mIsStartRunaway;
}

void RunawayTico::startRunaway() {
    mIsStartRunaway = true;

    if (MR::isValidSwitchA(this)) {
        MR::onSwitchA(this);
    }
}

void RunawayTico::setDemoTrans() {
    MR::startAction(this, "DemoMeetTico");
    setBaseMtx(MR::getPlayerBaseMtx());
}

void RunawayTico::exeGuide0() {
    if (MR::isFirstStep(this)) {
        MR::startTimeKeepDemoMarioPuppetable(this, "チコガイドデモ", nullptr);
        MR::onGameEventFlagEndTicoGuideDemo();
        MR::endStartPosCamera();
        MR::startAnimCameraTargetPlayer(this, mCameraInfo, "DemoMeetTico", 0, 1.0f);
        MR::forceToFrameCinemaFrame();
    }

    if (MR::isGreaterEqualStep(this, 0xD2)) {
        MR::startLevelSound(this, "SE_SM_LV_TICO_WAIT", -1, -1, -1);
    }

    if (MR::isGreaterEqualStep(this, 0x230)) {
        MR::startLevelSound(this, "SE_SM_LV_TICO_FLOAT_DEMO", static_cast< s32 >(100.0f * Tico::sFloatSeMinVolume), -1, -1);
    }

    if (MR::isStep(this, 0x269)) {
        MR::startSubBGM("BGM_MEET_TICO_ZOOM_OUT", false);
    }

    if (MR::isGreaterEqualStep(this, 0x276) && MR::isLessStep(this, 0x2DA)) {
        MR::startSystemLevelSE("SE_DM_LV_MEET_TICO_ZOOM_OUT", -1, -1);
    }
}

void RunawayTico::exeGuide1() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Metamorphosis");

        TPos3f mtx;
        mtx.setInline(MR::getJointMtx(this, "Body"));
        MR::addTransMtxLocalY(mtx.toMtxPtr(), -49.7357f);
        setBaseMtx(mtx);
        mLodCtrl->invalidate();
    }

    if (MR::isBckStopped(this)) {
        MR::hideModel(this);
    }

    if (MR::isDemoPartLastStep("チコとの出会い[ウサギ逃走]")) {
        MR::startStartPosCamera(true);
        MR::endAnimCamera(this, mCameraInfo, "DemoMeetTico", -1, true);
        MR::startStageBGM("MBGM_GALAXY_24", false);
        MR::showModel(this);
        mLodCtrl->validate();
        kill();
    }
}

void RunawayTico::exeWhiteOut() {
    if (MR::isFirstStep(this)) {
        MR::closeWipeWhiteFade(90);
        MR::stopStageBGM(135);
    }

    MR::limitedSound("SE_SM_LV_TICO_WAIT", 1);

    if (MR::isLessEqualStep(this, 30)) {
        MR::startLevelSound(this, "SE_SM_LV_TICO_OP_WAIT", -1, -1, -1);
    }

    if (!MR::isWipeActive()) {
        setNerve(&NrvRunawayTico::RunawayTicoNrvWhiteIn::sInstance);
    }
}

void RunawayTico::exeWhiteIn() {
    if (MR::isLessStep(this, 75)) {
        MR::limitedSound("SE_SM_LV_TICO_WAIT", 1);
    }

    if (MR::isStep(this, 75)) {
        MR::openWipeWhiteFade(90);
        MR::endDemo(this, "ぼやき");
        MR::startTimeKeepDemoMarioPuppetable(this, "チコガイドデモ", "高楼出現[デモ]");
        kill();
    }
}

void RunawayTico::exeAppear() {
    if (MR::isFirstStep(this)) {
        MR::startBckPlayer("WatchupMore", static_cast< const char* >(nullptr));
        MR::startAction(this, "Appear");
        MR::startSound(this, "SE_SM_RUNAWAY_RABBIT_APPEAR", -1, -1);
    }

    if (MR::isStep(this, 23)) {
        MR::startSound(this, "SE_SM_METAMORPHOSE_SMOKE", -1, -1);
    }

    if (MR::isBckStopped(this)) {
        setNerve(&NrvRunawayTico::RunawayTicoNrvTalk::sInstance);
    }
}

void RunawayTico::exeTalk() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "Talk");
    }

    turnToPlayer();

    if (MR::tryTalkForceWithoutDemoAtEnd(mMsgCtrl)) {
        if (_19D) {
            TalkMessageCtrl* ctrl = mMsgCtrl;
            MtxPtr baseMtx = getBaseMtx();
            MR::startNPCTalkCamera(ctrl, baseMtx, 1.0f, 0);
            setNerve(&NrvRunawayTico::RunawayTicoNrvWhiteOut::sInstance);
        }
        else {
            MR::endDemo(this, "ぼやき");
            setNerveWait();
        }
    }
}
