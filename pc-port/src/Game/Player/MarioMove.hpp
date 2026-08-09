#pragma once

#include "Game/Player/MarioModule.hpp"

class MarioActor;

class MarioMove : public MarioModule {
public:
    MarioMove(MarioActor*);
    void initAfter();

    /* 0x08 */ TVec3f _8;
    /* 0x14 */ TVec3f _14;
    /* 0x20 */ TVec3f _20;
    /* 0x2C */ TVec3f _2C;
    /* 0x38 */ TVec3f _38;
    /* 0x44 */ TVec3f _44;
    /* 0x50 */ f32 _50;
    /* 0x54 */ f32 _54;
};
