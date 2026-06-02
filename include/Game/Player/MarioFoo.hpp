#pragma once

#include "Game/Player/MarioState.hpp"

class MarioActor;

class MarioFoo : public MarioState {
public:
    MarioFoo(MarioActor*);

    /* 0x14 */ u8 _14[0x38];
    /* 0x4C */ u16 _4C;
    /* 0x4E */ u8 _4E[0x671];
};
