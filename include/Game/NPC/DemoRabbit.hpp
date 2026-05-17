#pragma once

#include "Game/NPC/NPCActor.hpp"
#include "Game/NameObj/NameObj.hpp"

class NameObjArchiveListCollector;
class JMapInfoIter;

class DemoRabbit : public NPCActor {
public:
    DemoRabbit(const char*);
    virtual ~DemoRabbit();
    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();
    virtual void control();

    static void makeArchiveList(NameObjArchiveListCollector*, const JMapInfoIter&);

    void fadeOut();
    void fadeIn();
    void updateStopVelocity();
    void updateNormalVelocity();
    void updateRun(const TVec3f&, bool);
    void updateJump();
    bool tryGuide();
    bool tryWait();
    bool tryGoal();
    void exeAppear();
    void exeDemo();
    void exeTalk();
    void exeWait();
    void exeGoal();
    void exeGuide();
    void exeRunaway();
    void exeChange();
    void exeStartBGM();

    /* 0x15C */ TVec3f mFrontVec;
    /* 0x168 */ s32 mNoGroundTimer;
};
