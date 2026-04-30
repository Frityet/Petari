#include "Game/Demo/ReturnDemoRailMove.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/MtxUtil.hpp"

void setResultFlyStartFrame(LiveActor* liveActor, long frame) {
    int maxFrames = MR::getBckFrameMax(liveActor);
    MR::setBckFrame(liveActor, maxFrames - (frame - (frame / maxFrames) * maxFrames));
}

ReturnDemoRailMove::ReturnDemoRailMove(
    LiveActor* demoStarter, LiveActor* powerStar,
    const JMapInfoIter &rIter, bool isGrandstar,
    TPos3f* transform)
    : mDemoStarter(demoStarter), mPowerStar(powerStar),
    mIsGrandStar(isGrandstar), mTransform(transform),
    mShootPath(nullptr), mPathDrawer(nullptr), mForward(0.0f, 0.0f, 1.0f) {
    SpinDriverShootPath* shootPath = new SpinDriverShootPath;
    mShootPath = shootPath;
    mShootPath->init(rIter);

    SpinDriverPathDrawer* pathDrawer = new SpinDriverPathDrawer(mShootPath);
    mPathDrawer = pathDrawer;
    mPathDrawer->initWithoutIter();
    mPathDrawer->setColorNormal();

    calcPathPosDir((TVec3f *) 0x0, &mForward, 1.0f);
}

void ReturnDemoRailMove::posToStart() {
    TVec3f position;
    TVec3f forward;
    calcPathPosDir(&position, &forward, 0.0f);

    TVec3f down = TVec3f(0.0f, -1.0f, 0.0f);
    MR::makeMtxUpFront(mTransform, forward, down);
    mTransform->setTrans(position);
    MR::setPlayerBaseMtx(*mTransform);
};

void ReturnDemoRailMove::posToEnd() {
    TVec3f position;
    calcPathPosDir(&position, (TVec3f *) nullptr, 1.0f);

    TVec3f up = TVec3f(0.0f, 1.0f, 0.0f);
    MR::makeMtxUpFront(mTransform, up, mForward);
    mTransform->setTrans(position);
    MR::setPlayerBaseMtx(*mTransform);
};

void ReturnDemoRailMove::offPathDraw() {
    mPathDrawer->kill();
};

s32 ReturnDemoRailMove::getDemoFlyBrakeFrame() const {
    if (mIsGrandStar != false) {
        return 296;
    }
    return 45;
};

void ReturnDemoRailMove::calcPathPosDir(TVec3f* position, TVec3f* direction, float t) const {
    if (position != nullptr) {
        mShootPath->calcPosition(position, t);
    }

    if (direction != nullptr) {
        mShootPath->calcDirection(direction, t, 0.01f);
    }
};

void ReturnDemoRailMove::setupPathDrawForGraneStarReturnDemo() {
    mPathDrawer->setMaskLength(100.0f);
    mPathDrawer->setFadeScale(0.1f);
};

void ReturnDemoRailMove::start() {
    const char* bckName = "ResultFly";
    if (mIsGrandStar != false) {
        bckName = "ResultFlyGrandStar";
    }
    MR::startBckPlayer(bckName, (char *) nullptr);

    MR::startBck(mPowerStar, bckName, nullptr);
    mPathDrawer->_B0 = 0.0f;
    mPathDrawer->appear();
};

void ReturnDemoRailMove::update(long currentStep, long maxSteps) {
    int firstDemoSteps = 45;
    if (mIsGrandStar != false) {
        firstDemoSteps = 296;
    }
    firstDemoSteps = maxSteps - firstDemoSteps;
    
    int secondDemoSteps = 34;
    if (mIsGrandStar != false) {
        secondDemoSteps = 98;
    }

    float progress = ((float)currentStep/maxSteps) - 1.0f;
    float t = 1.0f - progress * progress;

    if ((t < 0 && MR::isFirstStep(mDemoStarter))
         || MR::isStep(mDemoStarter, firstDemoSteps)) {
        const char* bckName = "ResultFly";
        if (mIsGrandStar != false) {
            bckName = "ResultFlyGrandStar";
        }

        MR::startBckPlayer(bckName, (char *) nullptr);
        MR::startBck(mPowerStar, bckName, nullptr);

        if (mIsGrandStar == false) {
            MR::startSoundPlayer("SE_PM_S_SPIN_DRV_COOL_DOWN", -1);
        }
    }

    if (MR::isFirstStep(mDemoStarter)) {
        if (firstDemoSteps < 0) {
            firstDemoSteps = -firstDemoSteps;
            MarioActor* marioActor = (MarioActor *) MR::getPlayerDemoActor();
            MR::setBckFrame(marioActor, firstDemoSteps);
            MR::setBckFrame(mPowerStar, firstDemoSteps);
        } else {
            MarioActor* marioActor = (MarioActor *) MR::getPlayerDemoActor();
            setResultFlyStartFrame(marioActor, firstDemoSteps);
            setResultFlyStartFrame(mPowerStar, firstDemoSteps);
        }
    }

    MR::startLevelSound(mPowerStar, "SE_OJ_LV_RET_POW_STAR_FLY", -1, -1, -1);
    
    TVec3f position;
    TVec3f direction;
    calcPathPosDir(&position, &direction, t);

    TVec3f down = TVec3f(0.0f, -1.0f, 0.0f);
    MR::makeMtxUpFront(mTransform, direction, down);

    if (MR::isGreaterStep(mDemoStarter, maxSteps - secondDemoSteps)) {
        float rate = MR::calcNerveEaseOutRate(mDemoStarter, maxSteps - secondDemoSteps, maxSteps);
        
        TVec3f up = TVec3f(0.0f, 1.0f, 0.0f);
        TPos3f transform;
        MR::makeMtxUpFront(&transform, up, mForward);
        MR::blendMtxRotateSlerp(*mTransform, transform, rate, *mTransform);
    }

    TVec3f translation;
    MR::setMtxTrans(*mTransform, *translation);
    mPathDrawer->setCoord(t);
    MR::setPlayerBaseMtx(*mTransform);
};
