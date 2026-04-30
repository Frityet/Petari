#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/MapObj/SpinDriverPathDrawer.hpp"
#include "Game/MapObj/SpinDriverShootPath.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include "JSystem/JGeometry/TVec.hpp"

class ReturnDemoRailMove {
public:
    ReturnDemoRailMove(LiveActor*, LiveActor*,
                       const JMapInfoIter&, bool, TPos3f*);

    void posToStart();
    void posToEnd();
    void offPathDraw();
    s32 getDemoFlyBrakeFrame() const;
    void calcPathPosDir(TVec3f*, TVec3f*, float) const;
    void setupPathDrawForGraneStarReturnDemo();
    void start();
    void update(long, long);

    /* 0x0 */ LiveActor* demoStarter;
    /* 0x4 */ LiveActor* powerStar;
    /* 0x8 */ bool isGrandStar;
    /* 0xC */ TPos3f* transform;
    /* 0x10 */ SpinDriverShootPath* shootPath;
    /* 0x14 */ SpinDriverPathDrawer* pathDrawer;
    /* 0x18 */ TVec3f forward;
};
