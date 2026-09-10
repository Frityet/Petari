#include "Game/Demo/DemoTimeKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/StringUtil.hpp"

DemoTimeKeeper::DemoTimeKeeper(const DemoExecutor* pExecutor)
    : mExecutor(pExecutor), mMainPartInfos(nullptr), mSubPartInfos(nullptr), mNumPartInfos(0), _10(-1), mCurrentStep(-1), _18(-1),
      mIsPaused(false) {
    JMapInfo* map = nullptr;
    mNumPartInfos = DemoFunction::createSheetParser(mExecutor, "Time", &map);
    mMainPartInfos = new DemoTimePartInfo[mNumPartInfos];
    for (s32 i = 0; i < mNumPartInfos; i++) {
        DemoTimePartInfo* part = &mMainPartInfos[i];
        MR::getCsvDataStrOrNULL(&part->mPartName, map, "PartName", i);
        MR::getCsvDataS32(&part->mTotalStep, map, "TotalStep", i);
        s32 suspendFlag = 0;
        MR::getCsvDataS32(&suspendFlag, map, "SuspendFlag", i);
        part->mSuspendFlag = suspendFlag != 0;
    }
}

DemoTimePartInfo::DemoTimePartInfo() : mPartName(nullptr), mTotalStep(1), mSuspendFlag(false) {
}

void DemoTimeKeeper::start() {
    _18 = 0;
    mSubPartInfos = &mMainPartInfos[_18];
}

void DemoTimeKeeper::update() {
    if (mIsPaused) {
        if (_10 <= 0) {
            _10++;
        }
        if (mCurrentStep <= 0) {
            mCurrentStep++;
        }
        return;
    }

    mCurrentStep++;
    _10++;
    if (mCurrentStep >= mSubPartInfos->mTotalStep && !mSubPartInfos->mSuspendFlag) {
        _18++;
        if (_18 < mNumPartInfos) {
            mCurrentStep = 0;
            mSubPartInfos = &mMainPartInfos[_18];
        }
    }
}

void DemoTimeKeeper::end() {
    _10 = -1;
    mCurrentStep = -1;
    _18 = -1;
    mSubPartInfos = nullptr;
}

bool DemoTimeKeeper::isDemoEnd() const {
    if (mIsPaused) {
        return false;
    }
    if (mSubPartInfos->mSuspendFlag && mSubPartInfos->mTotalStep <= mCurrentStep) {
        return true;
    }
    if (mSubPartInfos->mTotalStep >= mCurrentStep && mNumPartInfos == _18) {
        return true;
    }
    return false;
}

void DemoTimeKeeper::setStartPart(const char* pPartName) {
    setCurrentPart(pPartName);
}

bool DemoTimeKeeper::isExistSuspendFlagCurrentPart() const {
    return mSubPartInfos->mSuspendFlag;
}

bool DemoTimeKeeper::isPartLast() const {
    if (mIsPaused) {
        return false;
    }
    return mNumPartInfos - 1 == _18;
}

void DemoTimeKeeper::setCurrentPart(const char* pPartName) {
    s32 part = 0;
    for (; part < mNumPartInfos; part++) {
        if (MR::isEqualString(mMainPartInfos[part].mPartName, pPartName)) {
            goto found;
        }
    }
    part = -1;
found:
    _18 = part;
    mSubPartInfos = &mMainPartInfos[part];
}

bool DemoTimeKeeper::isCurrentDemoPartLastStep() const {
    return mCurrentStep >= mSubPartInfos->mTotalStep - 1;
}
