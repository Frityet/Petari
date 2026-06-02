#pragma once

#include "Game/NPC/NPCActor.hpp"

class JMapInfoIter;

class Rabbit : public NPCActor {
public:
    Rabbit(const char*);
    virtual ~Rabbit();

    virtual void init(const JMapInfoIter&);
    virtual void control();
    virtual void calcAndSetBaseMtx();

    void calcRailPos(TVec3f*);
    bool isNeedTurn(const TVec3f&);
    void updateJump();
    void exeAppear();
    void exeAppearLand();
    void exeWait();
    void exeForwardLand();
    void exePreJump();
    void exeMove();
    void exeGoal();
    void exeFinish();
    void exeReaction();
    void exeTalk();
    void exeJumpV();
    void exeJumpH();
    void exeBackwardLand();
    void exePreJumpBack();
    void exeNear();

    s32 mRabbitType;           // 0x15C
    bool mJumpLanding;         // 0x160
    u16 mOnGround;             // 0x162
    u16 mWaitTimer;            // 0x164
    f32 mJumpOffset;           // 0x168
    f32 mJumpVelocity;         // 0x16C
    f32 mRailMoveSpeed;        // 0x170
    TVec3f mRailMoveDirection; // 0x174
    f32 mRailNormalFactor;     // 0x180
};
