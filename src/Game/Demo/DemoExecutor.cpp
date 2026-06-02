#include "Game/Demo/DemoExecutor.hpp"

#include "Game/Demo/DemoActionKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Demo/DemoPlayerKeeper.hpp"
#include "Game/Demo/DemoTalkAnimCtrl.hpp"
#include "Game/Demo/DemoTimeKeeper.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Util.hpp"
#include <algorithm>

class DemoSheetKeeperBase {
public:
    virtual const char* getName() const = 0;
    virtual const char* getTypeString() const = 0;
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

    virtual const char* getName() const;
    virtual const char* getTypeString() const;
    virtual void start();
    virtual void update();

    u8 _4[0x14];
};

class DemoSoundKeeper : public DemoSheetKeeperBase {
public:
    DemoSoundKeeper(DemoExecutor*);

    virtual const char* getName() const;
    virtual const char* getTypeString() const;
    virtual void update();

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
        MR::FunctorV0M< DemoExecutor*, void (DemoExecutor::*)() > startDemoSystemFunc(this, &DemoExecutor::startProperDemoSystem);
        MR::listenNameObjStageSwitchOnAppear(this, mStageSwitchCtrl, startDemoSystemFunc);
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

        std::mem_fun_t< void, DemoSheetKeeperBase > updateSheet = std::mem_func(&DemoSheetKeeperBase::update);
        std::mem_fun_t< void, DemoSheetKeeperBase > updateSheetResult = std::for_each(mSheetKeepers.begin(), mSheetKeepers.end(), updateSheet);
    }

    std::mem_fun_t< bool, DemoTalkAnimCtrl > updateDemo = std::mem_func(&DemoTalkAnimCtrl::updateDemo);
    for (DemoTalkAnimCtrl** it = mTalkAnimCtrls; it != mTalkAnimCtrls + mNumTalkAnimCtrls; ++it) {
        updateDemo(*it);
    }
}

void DemoExecutor::start(NameObj* pStarter, const char* pDemoName, s32 startType) {
    mDemoStarter = pStarter;
    mDemoName = pDemoName;
    mStartType = startType;

    mTimeKeeper->start();
    mCameraKeeper->start();

    std::mem_fun_t< void, DemoSheetKeeperBase > startSheet = std::mem_func(&DemoSheetKeeperBase::start);
    std::mem_fun_t< void, DemoSheetKeeperBase > startSheetResult = std::for_each(mSheetKeepers.begin(), mSheetKeepers.end(), startSheet);

    std::mem_fun_t< void, DemoTalkAnimCtrl > startDemo = std::mem_func(&DemoTalkAnimCtrl::startDemo);
    for (DemoTalkAnimCtrl** it = mTalkAnimCtrls; it != mTalkAnimCtrls + mNumTalkAnimCtrls; ++it) {
        startDemo(*it);
    }

    mNumInvalidateClippingActors = 0;
    for (s32 i = 0; i < mGroup->mObjectCount; i++) {
        LiveActor* actor = mGroup->getActor(i);
        MR::sendMsgStartDemo(actor);
        DemoFunction::requestDemoCastMovementOn(actor);
        if (!MR::isInvalidClipping(actor)) {
            MR::invalidateClipping(actor);
            s32 idx = mNumInvalidateClippingActors;
            mNumInvalidateClippingActors = idx + 1;
            mInvalidateClippingActors[idx] = actor;
        }
    }
}

void DemoExecutor::startPart(NameObj* pStarter, const char* pDemoName, const char* pPartName, s32 startType) {
    std::for_each(
        mTalkAnimCtrls,
        mTalkAnimCtrls + mNumTalkAnimCtrls,
        std::bind2nd(std::mem_fun(&DemoTalkAnimCtrl::setupStartDemoPart), pPartName));

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
    std::for_each(
        mTalkAnimCtrls,
        mTalkAnimCtrls + mNumTalkAnimCtrls,
        std::bind2nd(std::mem_fun(&DemoTalkAnimCtrl::setupStartDemoPart), pPartName));

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
    std::for_each(
        mTalkAnimCtrls,
        mTalkAnimCtrls + mNumTalkAnimCtrls,
        std::bind2nd(std::mem_fun(&DemoTalkAnimCtrl::setupStartDemoPart), pPartName));

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
    s32 idx = mNumTalkAnimCtrls++;
    mTalkAnimCtrls[idx] = pCtrl;
}

void DemoExecutor::addTalkMessageCtrl(LiveActor* pActor, TalkMessageCtrl* pCtrl) {
    s32 idx = mNumTalkMessageInfos++;
    mTalkMessageInfos[idx].mActor = pActor;
    mTalkMessageInfos[idx].mMessageCtrl = pCtrl;
}

TalkMessageCtrl* DemoExecutor::findTalkMessageCtrl(const LiveActor* pActor) const {
    TalkMessageInfo* it = const_cast< TalkMessageInfo* >(mTalkMessageInfos);
    TalkMessageInfo* end = const_cast< TalkMessageInfo* >(mTalkMessageInfos + mNumTalkMessageInfos);
    for (; it != end; ++it) {
        if (it->mActor == pActor) {
            return it->mMessageCtrl;
        }
    }

    return nullptr;
}

void DemoExecutor::setTalkMessageCtrl(const LiveActor* pActor, TalkMessageCtrl* pCtrl) {
    TalkMessageInfo* it = mTalkMessageInfos;
    TalkMessageInfo* end = mTalkMessageInfos + mNumTalkMessageInfos;
    for (; it != end; ++it) {
        if (it->mActor == pActor) {
            it->mMessageCtrl = pCtrl;
            return;
        }
    }
}

void DemoExecutor::end() {
    mTimeKeeper->end();
    mSubPartKeeper->end();
    mCameraKeeper->end();

    std::mem_fun_t< void, DemoSheetKeeperBase > endSheet = std::mem_func(&DemoSheetKeeperBase::end);
    std::mem_fun_t< void, DemoSheetKeeperBase > endSheetResult = std::for_each(mSheetKeepers.begin(), mSheetKeepers.end(), endSheet);

    switch (mStartType) {
    case 1:
        MR::endDemo(mDemoStarter, mDemoName);
        break;
    case 2:
        MR::endDemo(mDemoStarter, mDemoName);
        break;
    }

    if (mStageSwitchCtrl->isValidSwitchDead()) {
        mStageSwitchCtrl->onSwitchDead();
    }

    LiveActor** it = mInvalidateClippingActors;
    LiveActor** end = mInvalidateClippingActors + mNumInvalidateClippingActors;

    mDemoStarter = nullptr;
    mDemoName = nullptr;
    mStartType = -1;
    void (*validate)(LiveActor*) = MR::validateClipping;
    for (; it != end; ++it) {
        validate(*it);
    }

    mNumInvalidateClippingActors = 0;
}

void DemoSheetKeeperBase::initCast(LiveActor*, const JMapInfoIter&) {}

void DemoSheetKeeperBase::update() {}

void DemoSheetKeeperBase::start() {}

void DemoSheetKeeperBase::end() {}

DemoExecutor::~DemoExecutor() {}
