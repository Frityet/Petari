#pragma once

#include <revolution/types.h>

class DemoExecutor;

class DemoTimePartInfo {
public:
    DemoTimePartInfo();

    const char* mName;  // _0
    s32 mTotalSteps;    // _4
    bool _8;
};

class DemoTimeKeeper {
public:
    DemoTimeKeeper(const DemoExecutor*);

    void start();
    void update();
    void end();
    bool isDemoEnd() const;
    void setStartPart(const char*);
    bool isExistSuspendFlagCurrentPart() const;
    bool isPartLast() const;
    void setCurrentPart(const char*);
    bool isCurrentDemoPartLastStep() const;

    const DemoExecutor* mExecutor;
    DemoTimePartInfo* mMainPartInfos;  // _4
    DemoTimePartInfo* mSubPartInfos;   // _8
    s32 mNumPartInfos;
    s32 _10;
    s32 mCurrentStep;  // _14
    s32 _18;
    bool mIsPaused;  // _1C
};
