#include "Game/NPC/RosettaDemoHeavensDoor.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/Util.hpp"

namespace MR {
    void timeKeepDemoFadeOut();
    void timeKeepDemoFadeIn();
}  // namespace MR

namespace NrvRosettaDemoHeavensDoor1 {
    NEW_NERVE(RosettaDemoHeavensDoor1NrvWait, RosettaDemoHeavensDoor1, Wait);
    NEW_NERVE(RosettaDemoHeavensDoor1NrvFade, RosettaDemoHeavensDoor1, Fade);
    NEW_NERVE(RosettaDemoHeavensDoor1NrvDemo, RosettaDemoHeavensDoor1, Demo);
}  // namespace NrvRosettaDemoHeavensDoor1

namespace NrvRosettaDemoHeavensDoor2 {
    NEW_NERVE(RosettaDemoHeavensDoor2NrvWait, RosettaDemoHeavensDoor2, Wait);

    class RosettaDemoHeavensDoor2NrvDemo : public Nerve {
    public:
        virtual void execute(Spine*) const {
        }

        static RosettaDemoHeavensDoor2NrvDemo sInstance;
    };

    RosettaDemoHeavensDoor2NrvDemo RosettaDemoHeavensDoor2NrvDemo::sInstance;
}  // namespace NrvRosettaDemoHeavensDoor2

RosettaDemoHeavensDoor1::RosettaDemoHeavensDoor1(Rosetta* pRosetta, const JMapInfoIter& rIter)
    : NerveExecutor("ロゼッタデモ実行者"), mRosetta(pRosetta) {
    DemoFunction::tryCreateDemoTalkAnimCtrlForActor(mRosetta, "DemoGetPower", "スピンゲット[デモ1]");
    initNerve(&NrvRosettaDemoHeavensDoor1::RosettaDemoHeavensDoor1NrvWait::sInstance);

    mLightDome = MR::createPartsModelNpc(mRosetta, "LightDome", "LightDome", nullptr);
    mLightDome->makeActorAppeared();
    mLightDome->initFixedPosition(TVec3f(0.0f, -13.0f, -30.0f), TVec3f(0.0f, 0.0f, 0.0f), "Center");
    MR::startBrk(mLightDome, "LightDome");
    MR::startBck(mLightDome, "Appear", nullptr);

    if (MR::isDemoCast(mRosetta, nullptr)) {
        MR::tryRegisterDemoCast(mLightDome, rIter);
    }

    DemoFunction::tryCreateDemoTalkAnimCtrlForActor(mLightDome, "DemoGetPower", "スピンゲット[デモ1]");
    DemoFunction::registerDemoTalkMessageCtrl(mRosetta, mRosetta->mMsgCtrl);

    mDomeHalo = MR::createPartsModelNpc(mRosetta, "DomeHalo", "DomeHalo", nullptr);
    mDomeHalo->initFixedPosition(TVec3f(0.0f, 25.14f, -6.16f), TVec3f(0.0f, 0.0f, 0.0f), "Center");
    mDomeHalo->makeActorAppeared();

    if (MR::isDemoCast(mRosetta, nullptr)) {
        MR::tryRegisterDemoCast(mDomeHalo, rIter);
    }

    mDomeHalo->mCalcOwnMtx = false;
    mDomeHalo->mPosition.set< f32 >(15064.593f, -7917.67f, 7541.112f);

    MR::needStageSwitchWriteA(mRosetta, rIter);
    MR::needStageSwitchWriteB(mRosetta, rIter);
    MR::registerDemoActionFunctor(mRosetta, MR::Functor(this, &RosettaDemoHeavensDoor1::preDemo), "高楼出現[デモ]");
    MR::registerDemoActionFunctor(mRosetta, MR::Functor(this, &RosettaDemoHeavensDoor1::pstDemo), "高楼出現[デモ後]");
    MR::registerDemoActionFunctor(mRosetta, MR::Functor(this, &RosettaDemoHeavensDoor1::fadeOut), "高楼出現[フェードアウト]");
    MR::registerDemoActionFunctor(mRosetta, MR::Functor(this, &RosettaDemoHeavensDoor1::fadeIn), "高楼出現[フェードイン]");
    MR::registerDemoActionFunctor(
        mRosetta, MR::Functor(this, &RosettaDemoHeavensDoor1::changeNerve< NrvRosettaDemoHeavensDoor1::RosettaDemoHeavensDoor1NrvDemo >),
        "スピンゲット[デモ1]");

    MR::invalidateShadowAll(mRosetta);
    MR::invalidateHitSensors(mRosetta);
    MR::setClippingTypeSphere(mRosetta, 1500.0f);
    mRosetta->startDemo(this);
    mRosetta->kill();
}

void RosettaDemoHeavensDoor1::makeArchiveList(NameObjArchiveListCollector* pCollector, const JMapInfoIter&) {
    pCollector->addArchive("LightDome");
    pCollector->addArchive("DomeHalo");
}

void RosettaDemoHeavensDoor1::preDemo() {
    MR::hidePlayer();
}

void RosettaDemoHeavensDoor1::pstDemo() {
    MR::startSound(mRosetta, "SE_OJ_ROSETTA_HALO_APPEAR", -1, -1);
}

void RosettaDemoHeavensDoor1::fadeOut() {
    MR::timeKeepDemoFadeOut();
}

void RosettaDemoHeavensDoor1::fadeIn() {
    mDomeHalo->mCalcOwnMtx = true;
    MR::emitEffect(mLightDome, "Light");
    MR::showPlayer();
    MR::timeKeepDemoFadeIn();
    MR::onSwitchB(mRosetta);
}

void RosettaDemoHeavensDoor1::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startAction(mRosetta, "DemoGetPowerStartWait");
    }

    if (MR::isNearPlayer(mRosetta->mMsgCtrl, 500.0f) && !MR::isTimeKeepDemoActive()) {
        MR::offPlayerControl();
        MR::timeKeepDemoFadeOut();
        MR::startBrk(mDomeHalo, "Disappear");
        setNerve(&NrvRosettaDemoHeavensDoor1::RosettaDemoHeavensDoor1NrvFade::sInstance);
    }
}

void RosettaDemoHeavensDoor1::exeFade() {
    if (MR::isStep(this, 90)) {
        MR::onPlayerControl(true);
        MR::startTimeKeepDemoMarioPuppetable(mRosetta, "チコガイドデモ", "スピンゲット[デモ1]");
        mDomeHalo->kill();
    }
}

void RosettaDemoHeavensDoor1::exeDemo() {
    if (MR::isDemoPartFirstStep("スピンゲット[デモ6]")) {
        MR::deleteEffect(mLightDome, "Light");
    }

    if (MR::isDemoPartActive("スピンゲット[デモ3]")) {
        s32 step = MR::getDemoPartStep("スピンゲット[デモ3]");

        if (step == 70) {
            MR::startSound(mRosetta, "SE_SV_ROSETTA_SWING", -1, -1);
        }

        if (step == 80) {
            MR::startSound(mRosetta, "SE_SM_ROSETTA_OP_SWING", -1, -1);
        }
    }

    if (MR::isDemoPartActive("スピンゲット[会話1]") || MR::isDemoPartActive("スピンゲット[会話2]") || MR::isDemoPartActive("スピンゲット[会話3]") ||
        MR::isDemoPartActive("スピンゲット[会話4]") || MR::isDemoPartActive("スピンゲット[デモ1]") || MR::isDemoPartActive("スピンゲット[デモ2]") ||
        MR::isDemoPartActive("スピンゲット[デモ3]") || MR::isDemoPartActive("スピンゲット[デモ4]") || MR::isDemoPartActive("スピンゲット[デモ5]")) {
        MR::startLevelSound(mRosetta, "SE_SM_LV_TICO_OP_WAIT", -1, -1, -1);
    }

    if (MR::isDemoPartActive("スピンゲット[デモ6]")) {
        s32 step = MR::getDemoPartStep("スピンゲット[デモ6]");

        if (step < 10) {
            MR::startLevelSound(mRosetta, "SE_SM_LV_TICO_OP_WAIT", -1, -1, -1);
        }

        if (step >= 10) {
            MR::startLevelSound(mRosetta, "SE_SM_LV_ROSETTA_OP_HIDE", -1, -1, -1);
        }
    }
}

RosettaDemoHeavensDoor2::RosettaDemoHeavensDoor2(Rosetta* pRosetta, const JMapInfoIter& rIter)
    : NerveExecutor("ロゼッタデモ実行者"), mDemoStarter(pRosetta), mRosetta(pRosetta) {
    DemoFunction::tryCreateDemoTalkAnimCtrlForScene(mRosetta, rIter, "DemoRedStar", "郷愁[開始]", 0, 0);
    DemoFunction::registerDemoTalkMessageCtrl(mRosetta, mRosetta->mMsgCtrl);
    MR::registerDemoActionFunctor(
        mRosetta, MR::Functor(this, &RosettaDemoHeavensDoor2::changeNerve< NrvRosettaDemoHeavensDoor2::RosettaDemoHeavensDoor2NrvDemo >),
        "郷愁[開始]");
    MR::needStageSwitchWriteA(mRosetta, rIter);

    if (MR::isOnGameEventFlagRosettaTalkAboutTicoInTower()) {
        mRosetta->kill();
    } else {
        MR::onSwitchA(mRosetta);
    }

    MR::invalidateShadowAll(mRosetta);
    MR::invalidateHitSensors(mRosetta);
    mRosetta->startDemo(this);
    initNerve(&NrvRosettaDemoHeavensDoor2::RosettaDemoHeavensDoor2NrvWait::sInstance);
}

void RosettaDemoHeavensDoor2::makeArchiveList(NameObjArchiveListCollector*, const JMapInfoIter&) {
}

void RosettaDemoHeavensDoor2::exeWait() {
    if (MR::isFirstStep(this)) {
        MR::startAction(mRosetta, "WaitB");
    }

    if (MR::isNearPlayer(mRosetta, 400.0f)) {
        mDemoStarter.start();
    }

    if (mDemoStarter.update()) {
        MR::tryStartTimeKeepDemoMarioPuppetable(mRosetta, "赤いスター", "赤いスター[開始]");
        MR::onGameEventFlagRosettaTalkAboutTicoInTower();
    }
}
