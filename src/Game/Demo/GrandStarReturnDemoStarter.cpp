#include "Game/Demo/GrandStarReturnDemoStarter.hpp"
#include "Game/Demo/AstroDemoFunction.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Demo/ReturnDemoRailMove.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/MapObj/PowerStar.hpp"
#include "Game/Screen/StageResultInformer.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "JSystem/JGeometry/TVec.hpp"
#include "math_types.hpp"
#include "revolution/types.h"
#include <cstdio>

namespace {
    const char* cDemoMovePartName = "移動";
    const char* cDemoWaitPartName;
}

GrandStarReturnDemoStarter::GrandStarReturnDemoStarter(const char* pName)
    : LiveActor(pName), returnDemoRailMove(), stageResultInformer(), powerstar(),
    distanceToCore(0.0f, 0.0f, 1.0f), position(gZeroVec), actorCameraInfo() {
    prevTransform.identity();
    transform.identity();
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
    this->stageResultInformer = stageResultInformer;
    stageResultInformer->initWithoutIter();

    PowerStar* powerstar = (PowerStar *) MR::createModelObjNoSilhouettedMapObjStrongLight("スターデモモデル", "GrandStar", this->transform);
    this->powerstar = powerstar;
    MR::invalidateClipping(powerstar);
    this->powerstar->kill();

    ReturnDemoRailMove* returnDemoRailMove = new ReturnDemoRailMove(this, this->powerstar, rIter, true, &this->prevTransform);
    
    this->returnDemoRailMove = returnDemoRailMove;
    this->returnDemoRailMove->setupPathDrawForGraneStarReturnDemo();

    if (MR::tryRegisterDemoCast(this, rIter)) {
        MR::tryRegisterDemoCast(this->powerstar, rIter);
    } else {
        int index = 1;
        do {
            const char* demoName = AstroDemoFunction::getGrandStarReturnDemoName(index);
            if (MR::isDemoExist(demoName) && MR::tryRegisterDemoCast(this, demoName, rIter)) {
                MR::tryRegisterDemoCast(this->powerstar, demoName, rIter);
            }
            index++;
        } while (index < 6);
    }

    MR::initMultiActorCamera(this, rIter, &this->actorCameraInfo, "移動");
    MR::initMultiActorCamera(this, rIter, &this->actorCameraInfo, "ウェイト");
    MR::initMultiActorCamera(this, rIter, &this->actorCameraInfo, "リザルト");

    for (int i = 0; i < 6; i++) {
        char buffer[0x20];
        snprintf(buffer, 0x20, "DemoAstroReturn%d", i + 1);
        MR::initAnimCamera(this, this->actorCameraInfo, "DemoAstroReturn.arc", buffer);
    }

    initNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvMove::sInstance);
    this->makeActorDead();
}

void GrandStarReturnDemoStarter::calcOffsetStarToCore(TVec3f* offset) const {
    TVec3f namePos;
    TVec3f jointPos;

    MR::findNamePos("コア中心", &namePos, 0x0);
    MR::copyJointPos(this->powerstar, "PowerStar", &jointPos);

    /* namePos - jointPos */
    asm {
        psq_l f0, 0x14(r1), 0, 0 /* qr0 */
        psq_l f1, 0x8(r1), 0, 0 /* qr0 */
        ps_sub f0, f0, f1
        psq_st f0, 0x0(r31), 0, 0 /* qr0 */
        psq_l f0, 0x1c(r1), 1, 0 /* qr0 */
        psq_l f1, 0x10(r1), 1, 0 /* qr0 */
        ps_sub f0, f0, f1
        psq_st f0, 0x8(r31), 1, 0 /* qr0 */
    };
}

void GrandStarReturnDemoStarter::updateRailMoveEndDir() {
    TVec3f offset;
    calcOffsetStarToCore(&offset);
    MR::normalize(&offset);

    this->returnDemoRailMove->forward.set(offset);
}

void GrandStarReturnDemoStarter::appear() {
    LiveActor::appear();

    this->powerstar->appear();
    MR::showJointAndChildren(this->powerstar, "PowerStar");
    this->returnDemoRailMove->posToStart();
    this->returnDemoRailMove->start();
    PowerStar::setupColorAtResultSequence(this->powerstar, true);

    setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvMove::sInstance);
}

void GrandStarReturnDemoStarter::control() {
    if (isNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvMove::sInstance)
        || isNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvFlyWait::sInstance)) {
        MR::setPlayerBaseMtx(this->prevTransform);

        // this->prevTransform = this->transform
        asm {
            psq_l f0, 0x94(r31), 0, 0 /* qr0 */
            psq_l f1, 0x9c(r31), 0, 0 /* qr0 */
            psq_l f2, 0xa4(r31), 0, 0 /* qr0 */
            psq_l f3, 0xac(r31), 0, 0 /* qr0 */
            psq_l f4, 0xb4(r31), 0, 0 /* qr0 */
            psq_l f5, 0xbc(r31), 0, 0 /* qr0 */
            psq_st f0, 0xc4(r31), 0, 0 /* qr0 */
            psq_st f1, 0xcc(r31), 0, 0 /* qr0 */
            psq_st f2, 0xd4(r31), 0, 0 /* qr0 */
            psq_st f3, 0xdc(r31), 0, 0 /* qr0 */
            psq_st f4, 0xe4(r31), 0, 0 /* qr0 */
            psq_st f5, 0xec(r31), 0, 0 /* qr0 */
        };

    }
    
    if (!MR::isDead(this)) {
        PowerStar::requestPointLightAtResultSequence(this->powerstar);
        // requestPointLightAtResultSequence__9PowerStarFPC9LiveActor
    }
        
}

void GrandStarReturnDemoStarter::emitEffectRush() {
    MR::emitEffect(this->powerstar, "Blur");
    MR::emitEffect(this->powerstar, "Blur1");
    MR::emitEffect(this->powerstar, "Blur2");
    MR::emitEffect(this->powerstar, "Blur3");
    MR::emitEffect(this->powerstar, "Blur4");
    MR::emitEffect(this->powerstar, "Blur5");
}

void GrandStarReturnDemoStarter::updateRushStarPos(const TVec3f* position, long frame) {
    TVec3f offset;
    TVec3f* newPosition;

    this->distanceToCore.scale(100.0f * frame, offset);
    offset.add(*newPosition, *position);

    this->transform[0][3] = newPosition->x;
    this->transform[1][3] = newPosition->y;
    this->transform[2][3] = newPosition->z;
}

void GrandStarReturnDemoStarter::tryStartStageResult(const char* demoName) {
    if (MR::isDemoPartLastStep(demoName)) {
        if (GameSequenceFunction::hasStageResultSequence()) {
            this->stageResultInformer->appear();
            MR::requestMovementOn(this->stageResultInformer);
        }

        MR::pauseTimeKeepDemo(this);
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvStageResult::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeMove() {
    if (MR::isFirstStep(this)) {
        int index = AstroDemoFunction::getActiveGrandStarReturnDemoIndex();
        if (index != 0) {
            MR::startStageBGM("STM_SECOND_ASTRO", false);
        } else {
            MR::startStageBGM("STM_FIRST_ASTRO", false);
        }
    }

    int index = AstroDemoFunction::getActiveGrandStarReturnDemoIndex();
    if (index != 0) {
        if (MR::isFirstStep(this)) {
            MR::startMultiActorCameraTargetPlayer(this, this->actorCameraInfo, cDemoMovePartName, -1);
        }
        if (MR::isDemoPartStep(cDemoMovePartName, 300)) {
            MR::startMultiActorCameraTargetPlayer(this, this->actorCameraInfo, "ウェイト", -1);
        }
    }

    s32 total = MR::getDemoPartTotalStep(cDemoMovePartName);
    s32 step = MR::getDemoPartStep(cDemoMovePartName);
    this->returnDemoRailMove->update(step + 1, total);

    MR::startLevelSoundPlayer("SE_PM_LV_SPIN_DRV_FLY", -1);
    
    this->updateRailMoveEndDir();

    if (MR::isDemoPartLastStep(cDemoMovePartName)) {
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvFlyWait::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeFlyWait() {
    if (MR::isFirstStep(this)) {
        updateRailMoveEndDir();
        // posToEnd__18ReturnDemoRailMoveFv
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
    position.set(this->prevTransform[0][3], this->prevTransform[1][3], this->prevTransform[2][3]);

    if (MR::isFirstStep(this)) {
        MR::startBckPlayer("ResultFlyGrandStarRush", (char *) nullptr);
        MR::startBck(this->powerstar, "ResultFlyGrandStarRush", (char *) nullptr);
        MR::startSound(this->powerstar, "SE_OJ_GND_STAR_RUSH", -1, -1);
        
        MR::hideJointAndChildren(this->powerstar, "PowerStar");
        calcOffsetStarToCore(&this->distanceToCore);
        this->distanceToCore.add(this->position, position);
        MR::normalize(&this->distanceToCore);
    }

    s32 step = getNerveStep();
    updateRushStarPos(&position, step);
    MR::startLevelSound(this->powerstar, "SE_OJ_LV_GND_STAR_RUSH", -1, -1, -1);

    if (MR::isDemoPartLastStep("ウェイト→コア突入")) {
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvRevival::sInstance);
    }
}
void GrandStarReturnDemoStarter::exeRevival() {
    char buffer[0x20];
    int index = AstroDemoFunction::getActiveGrandStarReturnDemoIndex() + 1;
    snprintf(buffer, 0x20, "DemoAstroReturn%d", index);

    if (MR::isFirstStep(this)) {
        this->returnDemoRailMove->offPathDraw();
        MR::forceDeleteEffectAll(this->powerstar);
        emitEffectRush();
        
        this->mPosition.zero();
        this->mRotation.zero();
        
        MR::startAnimCameraTargetSelf(this, this->actorCameraInfo, buffer, 0, 1.0f);
    }

    if (MR::isLessStep(this, 40)) {
        s32 step = getNerveStep();
        updateRushStarPos(&this->position, -(40-step));
        MR::startLevelSound(this->powerstar, "SE_OJ_LV_GND_STAR_RUSH", -1, -1, -1);
    }

    if (MR::isStep(this, 40)) {
        MR::startSound(this->powerstar, "SE_OJ_GND_STAR_ENTER_CORE", -1, -1);
        MR::forceDeleteEffect(this->powerstar, "DemoKoopaGrandStarVs3");
        this->powerstar->kill();
    }

    if (MR::isDemoPartLastStep("ドーム復活")) {
        MR::endAnimCamera(this, this->actorCameraInfo, buffer, 0, true);
        MR::overlayWithPreviousScreen(2);
    }

    if (!MR::isDemoPartFirstStep("リザルト画面")) {
        MR::startMultiActorCameraTargetPlayer(this, this->actorCameraInfo, "リザルト", -1);
    }

    tryStartStageResult("リザルト画面");
}

void GrandStarReturnDemoStarter::exeStageResult() {
    if (MR::isDead(this->stageResultInformer)) {
        setNerve(&NrvGrandStarReturnDemoStarter::GrandStarReturnDemoStarterNrvFadeOut::sInstance);
    }
}

void GrandStarReturnDemoStarter::exeFadeOut() {
    if (MR::isFirstStep(this)) {
        MR::closeWipeFade(60);
    }

    if (!MR::isWipeActive()) {
        MR::endMultiActorCamera(this, this->actorCameraInfo, "リザルト", false, -1);
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
        this->kill();
    }
}

GrandStarReturnDemoStarter::~GrandStarReturnDemoStarter() {}
