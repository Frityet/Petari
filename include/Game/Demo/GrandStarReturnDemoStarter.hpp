#pragma once

#include "Game/Demo/ReturnDemoRailMove.hpp"
#include "Game/MapObj/PowerStar.hpp"
#include "Game/Screen/StageResultInformer.hpp"

class GrandStarReturnDemoStarter : public LiveActor {
public:
    GrandStarReturnDemoStarter(const char*);
    virtual ~GrandStarReturnDemoStarter();
    virtual void init(const JMapInfoIter&);
    virtual void appear();
    virtual void control();

    void updateRailMoveEndDir();
    void calcOffsetStarToCore(TVec3f*) const;
    void emitEffectRush();
    void updateRushStarPos(const TVec3f*, long);
    void tryStartStageResult(const char*);
    
    void exeMove();
    void exeFlyWait();
    void exeRushToCore();
    void exeRevival();
    void exeStageResult();
    void exeFadeOut();
    void exeWaitDemoEnd();

    /*0x8C*/ ReturnDemoRailMove* mReturnDemoRailMove;
    /*0x90*/ StageResultInformer* mStageResultInformer;
    /*0x94*/ TPos3f mPrevTransform;
    /*0xC4*/ TPos3f mTransform;
    /*0xF4*/ PowerStar* mPowerstar;
    /*0xF8*/ TVec3f mDistanceToCore;
    /*0x104*/ TVec3f mPowerstarPosition;
    /*0x110*/ ActorCameraInfo* mActorCameraInfo;
};
