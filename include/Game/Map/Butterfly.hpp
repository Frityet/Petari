#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "Game/LiveActor/LiveActor.hpp"

class HitSensor;

class Butterfly : public LiveActor {
public:
    Butterfly(const char*);
    virtual ~Butterfly();
    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();
    virtual void control();
    virtual void calcAndSetBaseMtx();
    virtual void attackSensor(HitSensor*, HitSensor*);

    void updatePosture();
    void addRunAwayVelocity();
    bool tryRunAway();
    bool tryHive();
    bool tryPerchOnSleepingMario();
    bool tryAppearStarPeace(long);
    void exeWait();
    void exeRunAway();
    void exeHive();
    void exePerchOn();
    void exeGotoSleepingMario();
    void exeReadyToPerchOnSleepingMario();
    void exePerchOnSleepingMario();

    TVec3f mHomePosition;    // 0x8C
    TQuat4f mQuat;           // 0x98
    s32 mColorFrame;         // 0xA8
    HitSensor* mPerchSensor; // 0xAC
    bool mAppearedStarPiece; // 0xB0
};
