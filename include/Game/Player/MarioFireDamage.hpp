#pragma once

#include "Game/Player/MarioState.hpp"

class MarioActor;

class MarioFireDamage : public MarioState {
public:
    MarioFireDamage(MarioActor*);

    void decAfterTimer();

    /* 0x12 */ u16 _12;
};
