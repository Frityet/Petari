#pragma once

#include "Game/Demo/ReturnDemoRailMove.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/MapObj/PowerStar.hpp"
#include "Game/Screen/StageResultInformer.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include "JSystem/JGeometry/TVec.hpp"
#include <revolution.h>

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

private:
    /*0x8C*/ ReturnDemoRailMove* returnDemoRailMove; //ReturnDemoRailMove
    /*0x90*/ StageResultInformer* stageResultInformer;
    /*0x94*/ TPos3f prevTransform;
    /*0xC4*/ TPos3f transform;
    /*0xF4*/ PowerStar* powerstar;
    /*0xF8*/ TVec3f distanceToCore;
    /*0x104*/ TVec3f position;
    /*0x110*/ ActorCameraInfo* actorCameraInfo;
};
