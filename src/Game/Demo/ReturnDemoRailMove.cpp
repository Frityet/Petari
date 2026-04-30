#include "Game/Demo/ReturnDemoRailMove.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/MapObj/SpinDriverPathDrawer.hpp"
#include "Game/MapObj/SpinDriverShootPath.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include "JSystem/JGeometry/TVec.hpp"
#include "revolution/types.h"

namespace {}

void setResultFlyStartFrame(LiveActor* liveActor, long frame) {
    int maxFrames = MR::getBckFrameMax(liveActor);
    MR::setBckFrame(liveActor, maxFrames - (frame - (frame / maxFrames) * maxFrames));
}

ReturnDemoRailMove::ReturnDemoRailMove(
    LiveActor* demoStarter, LiveActor* powerStar,
    const JMapInfoIter &rIter, bool isGrandstar,
    TPos3f* transform)
    : demoStarter(demoStarter), powerStar(powerStar),
    isGrandStar(isGrandstar), transform(transform),
    shootPath(nullptr), pathDrawer(nullptr), forward(0.0f, 0.0f, 1.0f) {
    SpinDriverShootPath* shootPath = new SpinDriverShootPath;
    this->shootPath = shootPath;
    this->shootPath->init(rIter);

    SpinDriverPathDrawer* pathDrawer = new SpinDriverPathDrawer(this->shootPath);
    this->pathDrawer = pathDrawer;
    this->pathDrawer->initWithoutIter();
    this->pathDrawer->setColorNormal();

    calcPathPosDir((TVec3f *) 0x0, &this->forward, 1.0f);
}

void ReturnDemoRailMove::posToStart() {
    TVec3f position;
    TVec3f forward;
    this->calcPathPosDir(&position, &forward, 0.0f);

    TVec3f down = TVec3f(0.0f, -1.0f, 0.0f);
    MR::makeMtxUpFront(this->transform, forward, down);
    MR::setMtxTrans(*this->transform, position);
    MR::setPlayerBaseMtx(*this->transform);
};

void ReturnDemoRailMove::posToEnd() {
    TVec3f position;
    this->calcPathPosDir(&position, (TVec3f *) nullptr, 1.0f);

    TVec3f up = TVec3f(0.0f, 1.0f, 0.0f);
    MR::makeMtxUpFront(this->transform, up, this->forward);
    MR::setMtxTrans(*this->transform, position);
    MR::setPlayerBaseMtx(*this->transform);
};

void ReturnDemoRailMove::offPathDraw() {
    this->pathDrawer->kill();
};

s32 ReturnDemoRailMove::getDemoFlyBrakeFrame() const {
    if (this->isGrandStar != false) {
        return 296;
    }
    return 45;
};

void ReturnDemoRailMove::calcPathPosDir(TVec3f* position, TVec3f* direction, float t) const {
    if (position != nullptr) {
        this->shootPath->calcPosition(position, t);
    }

    if (direction != nullptr) {
        this->shootPath->calcDirection(direction, t, 0.01);
    }
};

void ReturnDemoRailMove::setupPathDrawForGraneStarReturnDemo() {
    this->pathDrawer->setMaskLength(100.0f);
    this->pathDrawer->setFadeScale(0.1f);
};

void ReturnDemoRailMove::start() {
    const char* bckName = "ResultFly";
    if (this->isGrandStar != false) {
        bckName = "ResultFlyGrandStar";
    }
    MR::startBckPlayer(bckName, (char *) nullptr);

    MR::startBck(this->powerStar, bckName, nullptr);
    this->pathDrawer->_B0 = 0.0f;
    this->pathDrawer->appear();
};

void ReturnDemoRailMove::update(long currentStep, long maxSteps) {
    int demoSteps = 45;
    if (this->isGrandStar != false) {
        demoSteps = 296;
    }
    demoSteps = maxSteps - demoSteps;
    
    int smth2 = 34;
    if (this->isGrandStar != false) {
        smth2 = 98;
    }

    float progress = (currentStep/maxSteps) - 1.0f;
    float t = 1.0f - progress * progress;

    if ((t < 0 && MR::isFirstStep(this->demoStarter))
         || MR::isStep(this->demoStarter, demoSteps)) {
        const char* bckName = "ResultFly";
        if (this->isGrandStar != false) {
            bckName = "ResultFlyGrandStar";
        }

        MR::startBckPlayer(bckName, (char *) nullptr);
        MR::startBck(this->powerStar, bckName, (char *) nullptr);

        if (this->isGrandStar == false) {
            MR::startSoundPlayer("SE_PM_S_SPIN_DRV_COOL_DOWN", -1);
        }
    }

    if (MR::isFirstStep(this->demoStarter)) {
        if (demoSteps < 0) {
            demoSteps = -demoSteps;
            MarioActor* marioActor = (MarioActor *) MR::getPlayerDemoActor();
            MR::setBckFrame(marioActor, demoSteps);
            
            MR::setBckFrame(this->powerStar, demoSteps);
        } else {
            MarioActor* marioActor = (MarioActor *) MR::getPlayerDemoActor();
            setResultFlyStartFrame(marioActor, demoSteps);

            setResultFlyStartFrame(this->powerStar, demoSteps);
        }
    }

    MR::startLevelSound(this->powerStar, "SE_OJ_LV_RET_POW_STAR_FLY", -1, -1, -1);
    
    TVec3f position;
    TVec3f direction;
    calcPathPosDir(&position, &direction, t);

    TVec3f down = TVec3f(0.0f, -1.0f, 0.0f);
    MR::makeMtxUpFront(this->transform, direction, down);

    if (MR::isGreaterStep(this->demoStarter, maxSteps - smth2)) {
        float rate = MR::calcNerveEaseOutRate(this->demoStarter, maxSteps - smth2, maxSteps);
        
        TVec3f up = TVec3f(0.0f, 1.0f, 0.0f);
        TPos3f transform;
        MR::makeMtxUpFront(&transform, up, this->forward);
        MR::blendMtxRotateSlerp(*this->transform, transform, rate, *this->transform);
    }

    TVec3f translation;
    MR::setMtxTrans(*this->transform, *translation);
    this->pathDrawer->setCoord(t);
    MR::setPlayerBaseMtx(*this->transform);
};
