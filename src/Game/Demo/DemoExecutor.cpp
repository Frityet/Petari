#include "Game/Demo/DemoExecutor.hpp"

#include "Game/Demo/DemoActionKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Demo/DemoPlayerKeeper.hpp"
#include "Game/Demo/DemoTalkAnimCtrl.hpp"
#include "Game/Demo/DemoTimeKeeper.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Util.hpp"
#include <algorithm>

class __declspec(novtable) DemoSheetKeeperBase {
public:
    virtual ~DemoSheetKeeperBase() {}
    virtual void initCast(LiveActor*, const JMapInfoIter&);
    virtual void start();
    virtual void end();
    virtual void update();
};

class DemoCameraKeeper {
public:
    DemoCameraKeeper(DemoExecutor*, const JMapInfoIter&);

    void initCast(LiveActor*, const JMapInfoIter&);
    void start();
    void update();
    void end();

    u8 _0[0x14];
};

class DemoWipeKeeper : public DemoSheetKeeperBase {
public:
    DemoWipeKeeper(DemoExecutor*);

    u8 _4[0x14];
};

class DemoSoundKeeper : public DemoSheetKeeperBase {
public:
    DemoSoundKeeper(DemoExecutor*);

    u8 _4[0x14];
};

DemoExecutor::DemoExecutor(const char* pName) : DemoCastGroup(pName) {
    mSheetName = nullptr;
    mTimeKeeper = nullptr;
    mSubPartKeeper = nullptr;
    mPlayerKeeper = nullptr;
    mCameraKeeper = nullptr;
    mActionKeeper = nullptr;
    mWipeKeeper = nullptr;
    mSoundKeeper = nullptr;
    mStageSwitchCtrl = nullptr;
    mDemoStarter = nullptr;
    mDemoName = nullptr;
    mStartType = -1;
    _50 = false;
    mNumInvalidateClippingActors = 0;
    mNumTalkAnimCtrls = 0;
    mNumTalkMessageInfos = 0;
}

DemoExecutor::~DemoExecutor() {}

void DemoExecutor::init(const JMapInfoIter& rIter) {
    DemoCastGroup::init(rIter);
    mSheetName = MR::getDemoSheetName(rIter);
    mTimeKeeper = new DemoTimeKeeper(this);
    mSubPartKeeper = new DemoSubPartKeeper(this);
    mPlayerKeeper = new DemoPlayerKeeper(this);
    mCameraKeeper = new DemoCameraKeeper(this, rIter);
    mActionKeeper = new DemoActionKeeper(this);
    mWipeKeeper = new DemoWipeKeeper(this);
    mSheetKeepers.push_back(mWipeKeeper);
    mSoundKeeper = new DemoSoundKeeper(this);
    mSheetKeepers.push_back(mSoundKeeper);

    mStageSwitchCtrl = MR::createStageSwitchCtrl(this, rIter);
    if (mStageSwitchCtrl->isValidSwitchAppear()) {
        MR::listenNameObjStageSwitchOnAppear(this, mStageSwitchCtrl, MR::Functor(this, &DemoExecutor::startProperDemoSystem));
    }

    DemoFunction::registerDemoExecutor(this);
}

void DemoExecutor::registerDemoActor(LiveActor* pActor, const JMapInfoIter& rIter) {
    DemoCastGroup::registerDemoActor(pActor, rIter);
    mCameraKeeper->initCast(pActor, rIter);
    mActionKeeper->initCast(pActor, rIter);

    for (DemoSheetKeeperBase** it = mSheetKeepers.begin(); it != mSheetKeepers.end(); ++it) {
        (*it)->initCast(pActor, rIter);
    }
}

void DemoExecutor::movement() {
    mTimeKeeper->update();
    if (mTimeKeeper->isDemoEnd()) {
        end();
        return;
    }

    if (!mTimeKeeper->mIsPaused) {
        mSubPartKeeper->update();
        mPlayerKeeper->update();
        mCameraKeeper->update();
        mActionKeeper->update();
        std::for_each(mSheetKeepers.begin(), mSheetKeepers.end(), std::mem_func(&DemoSheetKeeperBase::update));
    }

    std::for_each(mTalkAnimCtrls, &mTalkAnimCtrls[mNumTalkAnimCtrls], std::mem_func(&DemoTalkAnimCtrl::updateDemo));
}

void DemoExecutor::start(NameObj* pStarter, const char* pDemoName, s32 startType) {
    mDemoStarter = pStarter;
    mDemoName = pDemoName;
    mStartType = startType;

    mTimeKeeper->start();
    mCameraKeeper->start();
    std::for_each(mSheetKeepers.begin(), mSheetKeepers.end(), std::mem_func(&DemoSheetKeeperBase::start));
    std::for_each(mTalkAnimCtrls, &mTalkAnimCtrls[mNumTalkAnimCtrls], std::mem_func(&DemoTalkAnimCtrl::startDemo));

    mNumInvalidateClippingActors = 0;
    for (s32 i = 0; i < mGroup->mObjectCount; i++) {
        LiveActor* actor = mGroup->getActor(i);
        MR::sendMsgStartDemo(actor);
        DemoFunction::requestDemoCastMovementOn(actor);
        if (!MR::isInvalidClipping(actor)) {
            MR::invalidateClipping(actor);
            mInvalidateClippingActors[mNumInvalidateClippingActors] = actor;
            mNumInvalidateClippingActors++;
        }
    }
}

void DemoExecutor::startPart(NameObj* pStarter, const char* pDemoName, const char* pPartName, s32 startType) {
    for (s32 i = 0; i < mNumTalkAnimCtrls; i++) {
        mTalkAnimCtrls[i]->setupStartDemoPart(pPartName);
    }
    start(pStarter, pDemoName, startType);
    mTimeKeeper->setStartPart(pPartName);
}

void DemoExecutor::startProperDemoSystem() {
    if (mPlayerKeeper->mNumPlayerInfos > 0) {
        MR::requestStartTimeKeepDemoMarioPuppetable(this, mName, nullptr);
    } else {
        MR::requestStartTimeKeepDemo(this, mName, nullptr);
    }
}

void DemoExecutor::startDemoSystemPart(const char* pPartName, s32 startType) {
    for (s32 i = 0; i < mNumTalkAnimCtrls; i++) {
        mTalkAnimCtrls[i]->setupStartDemoPart(pPartName);
    }

    switch (startType) {
    case 1:
        MR::startTimeKeepDemo(this, mName, nullptr);
        break;
    case 2:
        MR::startTimeKeepDemoMarioPuppetable(this, mName, nullptr);
        break;
    }

    mTimeKeeper->setStartPart(pPartName);
}

bool DemoExecutor::tryStartProperDemoSystem() {
    if (mPlayerKeeper->mNumPlayerInfos > 0) {
        return MR::requestStartTimeKeepDemoMarioPuppetable(this, mName, nullptr);
    }
    return MR::requestStartTimeKeepDemo(this, mName, nullptr);
}

bool DemoExecutor::tryStartDemoSystemPart(const char* pPartName, s32 startType) {
    for (s32 i = 0; i < mNumTalkAnimCtrls; i++) {
        mTalkAnimCtrls[i]->setupStartDemoPart(pPartName);
    }

    bool started = false;
    switch (startType) {
    case 1:
        started = MR::tryStartTimeKeepDemo(this, mName, nullptr);
        break;
    case 2:
        started = MR::tryStartTimeKeepDemoMarioPuppetable(this, mName, nullptr);
        break;
    }

    if (!started) {
        return false;
    }

    mTimeKeeper->setStartPart(pPartName);
    return true;
}

bool DemoExecutor::tryStartProperDemoSystemPart(const char* pPartName) {
    if (mPlayerKeeper->mNumPlayerInfos > 0) {
        return MR::tryStartTimeKeepDemoMarioPuppetable(this, mName, pPartName);
    }
    return MR::tryStartTimeKeepDemo(this, mName, pPartName);
}

void DemoExecutor::pause() {
    mTimeKeeper->mIsPaused = true;
}

void DemoExecutor::resume() {
    mTimeKeeper->mIsPaused = false;
}

void DemoExecutor::addTalkAnimCtrl(DemoTalkAnimCtrl* pCtrl) {
    mTalkAnimCtrls[mNumTalkAnimCtrls] = pCtrl;
    mNumTalkAnimCtrls++;
}

void DemoExecutor::addTalkMessageCtrl(LiveActor* pActor, TalkMessageCtrl* pCtrl) {
    mTalkMessageInfos[mNumTalkMessageInfos].mActor = pActor;
    mTalkMessageInfos[mNumTalkMessageInfos].mMessageCtrl = pCtrl;
    mNumTalkMessageInfos++;
}

TalkMessageCtrl* DemoExecutor::findTalkMessageCtrl(const LiveActor* pActor) const {
    for (s32 i = 0; i < mNumTalkMessageInfos; i++) {
        if (mTalkMessageInfos[i].mActor == pActor) {
            return mTalkMessageInfos[i].mMessageCtrl;
        }
    }

    return nullptr;
}

void DemoExecutor::setTalkMessageCtrl(const LiveActor* pActor, TalkMessageCtrl* pCtrl) {
    for (s32 i = 0; i < mNumTalkMessageInfos; i++) {
        if (mTalkMessageInfos[i].mActor == pActor) {
            mTalkMessageInfos[i].mMessageCtrl = pCtrl;
            return;
        }
    }
}

void DemoExecutor::end() {
    mTimeKeeper->end();
    mSubPartKeeper->end();
    mCameraKeeper->end();
    std::for_each(mSheetKeepers.begin(), mSheetKeepers.end(), std::mem_func(&DemoSheetKeeperBase::end));

    switch (mStartType) {
    case 1:
    case 2:
        MR::endDemo(mDemoStarter, mDemoName);
        break;
    }

    if (mStageSwitchCtrl->isValidSwitchDead()) {
        mStageSwitchCtrl->onSwitchDead();
    }

    mDemoStarter = nullptr;
    mDemoName = nullptr;
    mStartType = -1;

    for (s32 i = 0; i < mNumInvalidateClippingActors; i++) {
        MR::validateClipping(mInvalidateClippingActors[i]);
    }

    mNumInvalidateClippingActors = 0;
}

void DemoSheetKeeperBase::initCast(LiveActor*, const JMapInfoIter&) {}

void DemoSheetKeeperBase::update() {}

void DemoSheetKeeperBase::start() {}

void DemoSheetKeeperBase::end() {}
