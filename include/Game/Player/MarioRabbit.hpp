#pragma once

#include "Game/Player/MarioState.hpp"

class MarioActor;

class MarioRabbit : public MarioState {
public:
    MarioRabbit(MarioActor*);

    virtual bool start();
    virtual bool close();
    virtual bool update();
    virtual bool notice();

    void hop();
    void forceJump();
    void impact();

    /* 0x14 */ f32 mVerticalSpeed;
    /* 0x18 */ TVec3f mMoveVelocity;
    /* 0x24 */ bool _24;
    /* 0x25 */ bool mIsWallJump;
    /* 0x26 */ bool mIsForceJump;
    /* 0x27 */ bool mDidImpact;
    /* 0x28 */ bool mIsHighJump;
    /* 0x29 */ u8 _29;
    /* 0x2A */ u16 mTurnTimer;
    /* 0x2C */ TMtx34f mJointMtx;
    /* 0x5C */ TVec3f mPrevFrontVec;
    /* 0x68 */ u8 mJumpAnimationIndex;
    /* 0x69 */ u8 mJumpRequestTimer;
    /* 0x6A */ bool mPlayLandingSound;
    /* 0x6B */ u8 _6B;
};
