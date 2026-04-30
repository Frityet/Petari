#include "Game/Demo/GrandStarReturnDemoStarter.hpp"
#include "Game/Demo/AstroDemoFunction.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include <cstdio>

namespace {
    const char* cDemoMovePartName = "移動";
    const char* cDemoWaitPartName;
}

GrandStarReturnDemoStarter::GrandStarReturnDemoStarter(const char* pName)
    : LiveActor(pName), mReturnDemoRailMove(), mStageResultInformer(), mPowerstar(),
    mDistanceToCore(0.0f, 0.0f, 1.0f), mPowerstarPosition(gZeroVec), mActorCameraInfo() {
    mPrevTransform.identity();
    mTransform.identity();
}

namespace NrvGrandStarReturnDemoStarter {
    NEW_NERVE(GrandStarReturnDemoStarterNrvMove, GrandStarReturnDemoStarter, Move);
    NEW_NERVE(GrandStarReturnDemoStarterNrvFlyWait, GrandStarReturnDemoStarter, FlyWait);
    NEW_NERVE(GrandStarReturnDemoStarterNrvRushToCore, GrandStarReturnDemoStarter, RushToCore);
    NEW_NERVE(GrandStarReturnDemoStarterNrvRevival, GrandStarReturnDemoStarter, Revival);
    NEW_NERVE(GrandStarReturnDemoStarterNrvStageResult, GrandStarReturnDemoStarter, StageResult);
    NEW_NERVE(GrandStarReturnDemoStarterNrvFadeOut, GrandStarReturnDemoStarter, FadeOut);
    NEW_NERVE(GrandStarReturnDemoStarterNrvWaitDemoEnd, GrandStarReturnDemoStarter, WaitDemoEnd);
}

void GrandStarReturnDemoStarter::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    MR::connectToSceneMapObjMovement(this);
    MR::invalidateClipping(this);
    
    StageResultInformer* stageResultInformer = new StageResultInformer;
    mStageResultInformer = stageResultInformer;
    mStageResultInformer->initWithoutIter();

    PowerStar* powerstar = (PowerStar *) MR::createModelObjNoSilhouettedMapObjStrongLight("スターデモモデル", "GrandStar", mTransform);
    mPowerstar = powerstar;
    MR::invalidateClipping(powerstar);
    mPowerstar->kill();

    ReturnDemoRailMove* returnDemoRailMove = new ReturnDemoRailMove(this, mPowerstar, rIter, true, &mPrevTransform);
    
    mReturnDemoRailMove = returnDemoRailMove;
    mReturnDemoRailMove->setupPathDrawForGraneStarReturnDemo();

    if (MR::tryRegisterDemoCast(this, rIter)) {
        MR::tryRegisterDemoCast(mPowerstar, rIter);
    } else {
        int index = 1;
        do {
            const char* demoName = AstroDemoFunction::getGrandStarReturnDemoName(index);
            if (MR::isDemoExist(demoName) && MR::tryRegisterDemoCast(this, demoName, rIter)) {
                MR::tryRegisterDemoCast(mPowerstar, demoName, rIter);
            }
            index++;
        } while (index < 6);
    }

    MR::initMultiActorCamera(this, rIter, &mActorCameraInfo, "移動");
    MR::initMultiActorCamera(this, rIter, &mActorCameraInfo, "ウェイト");
    MR::initMultiActorCamera(this, rIter, &mActorCameraInfo, "リザルト");

    for (int i = 0; i < 6; i++) {
        char buffer[0x20];
        snprintf(buffer, 0x20, "DemoAstroReturn%d", i + 1);
        MR::initAnimCamera(this, mActorCameraInfo, "DemoAstroReturn.arc", buffer);
    }

    initNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvMove::sInstance);
    makeActorDead();
}

void GrandStarReturnDemoStarter::calcOffsetStarToCore(TVec3f* offset) const {
    TVec3f namePos;
    TVec3f jointPos;

    MR::findNamePos("コア中心", &namePos, 0x0);
    MR::copyJointPos(mPowerstar, "PowerStar", &jointPos);

    offset->subInline(namePos, jointPos);
}

void GrandStarReturnDemoStarter::updateRailMoveEndDir() {
    TVec3f offset;
    calcOffsetStarToCore(&offset);
    MR::normalize(&offset);

    mReturnDemoRailMove->mForward.set(offset);
}

void GrandStarReturnDemoStarter::appear() {
    LiveActor::appear();

    mPowerstar->appear();
    MR::showJointAndChildren(mPowerstar, "PowerStar");
    mReturnDemoRailMove->posToStart();
    mReturnDemoRailMove->start();
    PowerStar::setupColorAtResultSequence(mPowerstar, true);

    setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvMove::sInstance);
}

void GrandStarReturnDemoStarter::control() {
    if (isNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvMove::sInstance)
        || isNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvFlyWait::sInstance)) {
        MR::setPlayerBaseMtx(mPrevTransform);

        mTransform.setInline(mPrevTransform);
    }
    
    if (!MR::isDead(mPowerstar)) {
        PowerStar::requestPointLightAtResultSequence(mPowerstar);
    }
        
}

void GrandStarReturnDemoStarter::emitEffectRush() {
    MR::emitEffect(mPowerstar, "Blur");
    MR::emitEffect(mPowerstar, "Blur1");
    MR::emitEffect(mPowerstar, "Blur2");
    MR::emitEffect(mPowerstar, "Blur3");
    MR::emitEffect(mPowerstar, "Blur4");
    MR::emitEffect(mPowerstar, "Blur5");
}

void GrandStarReturnDemoStarter::updateRushStarPos(const TVec3f* position, long frame) {
    TVec3f offset;
    TVec3f newPosition;

    offset.scale(100.0f * frame, mDistanceToCore);
    newPosition.add(mPowerstarPosition, offset);
    mTransform.setTrans(newPosition);
}

void GrandStarReturnDemoStarter::tryStartStageResult(const char* demoName) {
    if (MR::isDemoPartLastStep(demoName)) {
        if (GameSequenceFunction::hasStageResultSequence()) {
            mStageResultInformer->appear();
            MR::requestMovementOn(mStageResultInformer);
        }

        MR::pauseTimeKeepDemo(this);
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvStageResult::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeMove() {
    if (MR::isFirstStep(this)) {
        if (!AstroDemoFunction::getActiveGrandStarReturnDemoIndex()) {
            MR::startStageBGM("STM_SECOND_ASTRO", false);
        } else {
            MR::startStageBGM("STM_FIRST_ASTRO", false);
        }
    }

    if (!AstroDemoFunction::getActiveGrandStarReturnDemoIndex()) {
        if (MR::isFirstStep(this)) {
            MR::startMultiActorCameraTargetPlayer(this, mActorCameraInfo, cDemoMovePartName, -1);
        }
        if (MR::isDemoPartStep(cDemoMovePartName, 300)) {
            MR::startMultiActorCameraTargetPlayer(this, mActorCameraInfo, "ウェイト", -1);
        }
    }

    const char* demoName = cDemoMovePartName;
    s32 total = MR::getDemoPartTotalStep(demoName);
    s32 step = MR::getDemoPartStep(demoName);
    mReturnDemoRailMove->update(step + 1, total);

    MR::startLevelSoundPlayer("SE_PM_LV_SPIN_DRV_FLY", -1);
    
    updateRailMoveEndDir();

    if (MR::isDemoPartLastStep(cDemoMovePartName)) {
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvFlyWait::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeFlyWait() {
    if (MR::isFirstStep(this)) {
        updateRailMoveEndDir();
        mReturnDemoRailMove->posToEnd();
    }

    if (MR::isStep(this, 60)) {
        emitEffectRush();
    }

    if (!MR::isLessStep(this, 60)) {
        MR::startLevelSound(this, "SE_OJ_LV_RET_POW_STAR_FLY", -1, -1, -1);
    }

    if (MR::isBckOneTimeAndStoppedPlayer()) {
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvRushToCore::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeRushToCore() {
    TVec3f position;
    position.set(mPrevTransform[0][3], mPrevTransform[1][3], mPrevTransform[2][3]);
    
    if (MR::isFirstStep(this)) {
        MR::startBckPlayer("ResultFlyGrandStarRush", (char *) nullptr);
        MR::startBck(mPowerstar, "ResultFlyGrandStarRush", (char *) nullptr);
        MR::startSound(mPowerstar, "SE_OJ_GND_STAR_RUSH", -1, -1);
        
        MR::hideJointAndChildren(mPowerstar, "PowerStar");
        calcOffsetStarToCore(&mDistanceToCore);
        mPowerstarPosition.add(position, mDistanceToCore);
        MR::normalize(&mDistanceToCore);
    }

    s32 step = getNerveStep();
    updateRushStarPos(&position, step);
    MR::startLevelSound(mPowerstar, "SE_OJ_LV_GND_STAR_RUSH", -1, -1, -1);

    if (MR::isDemoPartLastStep("ウェイト→コア突入")) {
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvRevival::sInstance);
    }
}
void GrandStarReturnDemoStarter::exeRevival() {
    char buffer[0x20];
    int index = AstroDemoFunction::getActiveGrandStarReturnDemoIndex() + 1;
    snprintf(buffer, 0x20, "DemoAstroReturn%d", index);

    if (MR::isFirstStep(this)) {
        mReturnDemoRailMove->offPathDraw();
        MR::forceDeleteEffectAll(mPowerstar);
        emitEffectRush();
        
        mPosition.zero();
        mRotation.zero();
        
        MR::startAnimCameraTargetSelf(this, mActorCameraInfo, buffer, 0, 1.0f);
    }

    if (MR::isLessStep(this, 40)) {
        s32 step = getNerveStep();
        updateRushStarPos(&mPowerstarPosition, -(40-step));
        MR::startLevelSound(mPowerstar, "SE_OJ_LV_GND_STAR_RUSH", -1, -1, -1);
    }

    if (MR::isStep(this, 40)) {
        MR::startSound(mPowerstar, "SE_OJ_GND_STAR_ENTER_CORE", -1, -1);
        MR::forceDeleteEffect(mPowerstar, "DemoKoopaGrandStarVs3");
        mPowerstar->kill();
    }

    if (MR::isDemoPartLastStep("ドーム復活")) {
        MR::endAnimCamera(this, mActorCameraInfo, buffer, 0, true);
        MR::overlayWithPreviousScreen(2);
    }

    if (!MR::isDemoPartFirstStep("リザルト画面")) {
        MR::startMultiActorCameraTargetPlayer(this, mActorCameraInfo, "リザルト", -1);
    }

    tryStartStageResult("リザルト画面");
}

void GrandStarReturnDemoStarter::exeStageResult() {
    if (MR::isDead(mStageResultInformer)) {
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvFadeOut::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeFadeOut() {
    if (MR::isFirstStep(this)) {
        MR::closeWipeFade(60);
    }

    if (!MR::isWipeActive()) {
        MR::endMultiActorCamera(this, mActorCameraInfo, "リザルト", false, -1);
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvWaitDemoEnd::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeWaitDemoEnd() {
    if (MR::isFirstStep(this)) {
        if (AstroDemoFunction::getActiveGrandStarReturnDemoIndex() != 0) {
            MR::startStageBGM("MBGM_GALAXY_24", false);
        } else if (AstroDemoFunction::getActiveGrandStarReturnDemoIndex() == 1) {
            MR::startStageBGM("STM_ASTRO_OUT", false);
        } else if (AstroDemoFunction::getActiveGrandStarReturnDemoIndex() == 2) {
            MR::startStageBGM("STM_ASTRO_OUT2", false);
        } else if (AstroDemoFunction::getActiveGrandStarReturnDemoIndex() == 3) {
            MR::startStageBGM("STM_ASTRO_OUT2", false);
        } else {
        MR::startStageBGM("STM_ASTRO_OUT3", false);
        }

        MR::resumeTimeKeepDemo(this);
    }

    if (!MR::isTimeKeepDemoActive()) {
        kill();
    }
}

GrandStarReturnDemoStarter::~GrandStarReturnDemoStarter() {}
