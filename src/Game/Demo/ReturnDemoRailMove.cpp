#include "Game/Demo/ReturnDemoRailMove.hpp"

void setResultFlyStartFrame(LiveActor* liveActor, long frame) NO_INLINE {
    int maxFrames = MR::getBckFrameMax(liveActor);
    MR::setBckFrame(liveActor, maxFrames - frame%maxFrames);
}

ReturnDemoRailMove::ReturnDemoRailMove(
    LiveActor* pDemoStarter, LiveActor* pPowerStar,
    const JMapInfoIter &rIter, bool isGrandstar,
    TPos3f* pTransform)
    : mDemoStarter(pDemoStarter), mPowerStar(pPowerStar),
    mIsGrandStar(isGrandstar), mTransform(pTransform),
    mShootPath(nullptr), mPathDrawer(nullptr), mForward(0.0f, 0.0f, 1.0f) {
    mShootPath = new SpinDriverShootPath();
    mShootPath->init(rIter);

    mPathDrawer = new SpinDriverPathDrawer(mShootPath);
    mPathDrawer->initWithoutIter();
    mPathDrawer->setColorNormal();

    calcPathPosDir(nullptr, &mForward, 1.0f);
}

void ReturnDemoRailMove::posToStart() {
    TVec3f position;
    TVec3f forward;
    calcPathPosDir(&position, &forward, 0.0f);

    TPos3f* transform = mTransform; // Necessary to match
    MR::makeMtxUpFront(transform, forward, TVec3f(0.0f, -1.0f, 0.0f));
    mTransform->setTrans(position);
    MR::setPlayerBaseMtx(*mTransform);
};

void ReturnDemoRailMove::posToEnd() {
    TVec3f position;
    calcPathPosDir(&position, nullptr, 1.0f);

    TPos3f* transform = mTransform; // Necessary to match
    MR::makeMtxUpFront(transform, TVec3f(0.0f, 1.0f, 0.0f), mForward);
    mTransform->setTrans(position);
    MR::setPlayerBaseMtx(*mTransform);
};

void ReturnDemoRailMove::offPathDraw() {
    mPathDrawer->kill();
};

inline s32 ReturnDemoRailMove::getDemoFlyBrakeFrame() const {
    if (mIsGrandStar != false) {
        return 296;
    }
    return 45;
};

void ReturnDemoRailMove::calcPathPosDir(TVec3f* position, TVec3f* direction, f32 t) const {
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
    const char* pBckName = "ResultFly";
    if (mIsGrandStar != false) {
        pBckName = "ResultFlyGrandStar";
    }
    MR::startBckPlayer(pBckName, reinterpret_cast<char *>(nullptr));

    MR::startBck(mPowerStar, pBckName, nullptr);
    mPathDrawer->_B0 = 0.0f;
    mPathDrawer->appear();
};

void ReturnDemoRailMove::update(long currentStep, long maxSteps) {
    int firstDemoSteps = maxSteps - getDemoFlyBrakeFrame();
    
    int secondDemoSteps = 34;
    if (mIsGrandStar != false) {
        secondDemoSteps = 98;
    }

    f32 progress = ((f32)currentStep/maxSteps) - 1.0f;
    f32 t = 1.0f - progress * progress;

    if ((t < 0 && MR::isFirstStep(mDemoStarter))
         || MR::isStep(mDemoStarter, firstDemoSteps)) {
        const char* pBckName = "ResultFly";
        if (mIsGrandStar != false) {
            pBckName = "ResultFlyGrandStar";
        }

        MR::startBckPlayer(pBckName, reinterpret_cast<char *>(nullptr));
        MR::startBck(mPowerStar, pBckName, nullptr);

        if (mIsGrandStar == false) {
            MR::startSoundPlayer("SE_PM_S_SPIN_DRV_COOL_DOWN", -1);
        }
    }

    if (MR::isFirstStep(mDemoStarter)) {
        if (firstDemoSteps < 0) {
            firstDemoSteps = -firstDemoSteps;
            MR::setBckFrame(MR::getPlayerDemoActor(), firstDemoSteps);
            MR::setBckFrame(mPowerStar, firstDemoSteps);
        } else {
            setResultFlyStartFrame(MR::getPlayerDemoActor(), firstDemoSteps);
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
        f32 rate = MR::calcNerveEaseOutRate(mDemoStarter, maxSteps - secondDemoSteps, maxSteps);
        
        TPos3f transform;
        MR::makeMtxUpFront(&transform, TVec3f(0.0f, 1.0f, 0.0f), mForward);
        MR::blendMtxRotateSlerp(*mTransform, transform, rate, *mTransform);
    }

    TVec3f translation;
    mTransform->setTrans(translation);
    mPathDrawer->setCoord(t);
    MR::setPlayerBaseMtx(*mTransform);
};
