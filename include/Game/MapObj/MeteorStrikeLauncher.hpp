#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class MeteorStrike;

class MeteorStrikeLauncher : public LiveActor {
public:
    MeteorStrikeLauncher(const char*);
    virtual ~MeteorStrikeLauncher();
    virtual void init(const JMapInfoIter&);
    virtual void appear();
    virtual void kill();

    void initMapToolInfo(const JMapInfoIter&);
    MeteorStrike* getUnusedMeteorStrike();
    bool create();
    void exeCreate();

    MeteorStrike** mMeteorStrikes;    // 0x8C
    s32 mMeteorStrikeCount;           // 0x90
    s32 mCreateInterval;              // 0x94
    s32 mCreateOffset;                // 0x98
    bool mIsObjectMeteorStrike;       // 0x9C
    bool mUseScreenPositionCheck;     // 0x9D
};
