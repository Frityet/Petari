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
        MR::getCsvDataStrOrNULL(&partInfo->mName, map, "PartName", i);
        MR::getCsvDataS32(&partInfo->mTotalSteps, map, "TotalStep", i);

        s32 suspendFlag = 0;
        MR::getCsvDataS32(&suspendFlag, map, "SuspendFlag", i);
        partInfo->_8 = suspendFlag != 0;
    }
}

DemoTimePartInfo::DemoTimePartInfo() : mName(nullptr), mTotalSteps(1), _8(false) {}

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

    if (mSubPartInfos->mTotalSteps > mCurrentStep) {
        return;
    }

    if (mSubPartInfos->_8) {
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

    if (mSubPartInfos->_8 && mSubPartInfos->mTotalSteps <= mCurrentStep) {
        return true;
    }

    if (mSubPartInfos->mTotalSteps >= mCurrentStep) {
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
    return mSubPartInfos->_8;
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
        if (!MR::isEqualString(mMainPartInfos[partIndex].mName, pPartName)) {
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
    return mCurrentStep >= mSubPartInfos->mTotalSteps - 1;
}
