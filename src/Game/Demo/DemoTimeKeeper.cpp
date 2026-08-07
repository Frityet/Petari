#include "Game/Demo/DemoTimeKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Util.hpp"

DemoTimeKeeper::DemoTimeKeeper(const DemoExecutor* pExecutor) :
    mExecutor(pExecutor),
    mMainPartInfos(nullptr),
    mSubPartInfos(nullptr),
    mNumPartInfos(0),
    _10(-1),
    mCurrentStep(-1),
    _18(-1),
    mIsPaused(false) {
    JMapInfo* map = nullptr;
    mNumPartInfos = DemoFunction::createSheetParser(mExecutor, "Time", &map);
    mMainPartInfos = new DemoTimePartInfo[mNumPartInfos];

    for (s32 i = 0; i < mNumPartInfos; i++) {
        DemoTimePartInfo* partInfo = &mMainPartInfos[i];
        MR::getCsvDataStrOrNULL(&partInfo->mPartName, map, "PartName", i);
        MR::getCsvDataS32(&partInfo->mTotalStep, map, "TotalStep", i);

        s32 suspendFlag = 0;
        MR::getCsvDataS32(&suspendFlag, map, "SuspendFlag", i);
        partInfo->mSuspendFlag = suspendFlag != 0;
    }
}

DemoTimePartInfo::DemoTimePartInfo() : mPartName(nullptr), mTotalStep(1), mSuspendFlag(false) {}

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

    if (mSubPartInfos->mTotalStep > mCurrentStep) {
        return;
    }

    if (mSubPartInfos->mSuspendFlag) {
        return;
    }

    _18++;
    if (mNumPartInfos <= _18) {
        return;
    }

    DemoTimePartInfo* partInfo = &mMainPartInfos[_18];
    mCurrentStep = 0;
    mSubPartInfos = partInfo;
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

    if (mSubPartInfos->mTotalStep >= mCurrentStep) {
        if (mNumPartInfos == _18) {
            return true;
        }
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

    return _18 == mNumPartInfos - 1;
}

void DemoTimeKeeper::setCurrentPart(const char* pPartName) {
    s32 partIndex = 0;
    while (partIndex < mNumPartInfos) {
        if (!MR::isEqualString(mMainPartInfos[partIndex].mPartName, pPartName)) {
            partIndex++;
        } else {
            goto set_part;
        }
    }

    partIndex = -1;

set_part:
    _18 = partIndex;
    mSubPartInfos = &mMainPartInfos[partIndex];
}

bool DemoTimeKeeper::isCurrentDemoPartLastStep() const {
    return mCurrentStep >= mSubPartInfos->mTotalStep - 1;
}
