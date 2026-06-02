#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "Game/LiveActor/LiveActor.hpp"

class ActorCameraInfo;

class ShockWaveGenerator : public LiveActor {
public:
    ShockWaveGenerator(const char*);
    virtual ~ShockWaveGenerator();
    virtual void init(const JMapInfoIter&);
    virtual bool receiveMsgPlayerAttack(u32, HitSensor*, HitSensor*);

    void exeWait();
    void exeDemoEcho();
    void exeGenerate();
    void startShockWave();
    void sendMsgShockWaveToNearEnemy();
    bool isHitCylinder(HitSensor*, HitSensor*) const;

private:
    ActorCameraInfo* mCameraInfo;
};
