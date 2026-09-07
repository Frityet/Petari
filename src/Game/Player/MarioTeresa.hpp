#pragma once

#include "Game/Player/MarioState.hpp"

class MarioActor;

class MarioTeresa : public MarioState {
public:
    MarioTeresa(MarioActor*);

    virtual bool start();
    virtual bool close();
    virtual bool update();
    virtual bool notice();
    virtual bool keep();

    void updateDropFlag();
    void resetTeresaMode();
    bool isTeresaAccel() const;
    f32 checkHeight();
    void checkAccel();
    void checkGroundReflect();
    void procNoControl();
    void procNearGroundControl();
    void procDrop();
    void addTeresaVerticalVelocity(f32);
    void addTeresaHorizontalVelocity(const TVec3f&);
    void doTeresaReflection(const TVec3f&, bool);
    void checkWind();
    void checkWallCeilReflect();
    void procAirControl();
    void procControl();

    /* 0x14 */ TVec3f _14;
    /* 0x20 */ f32 _20;
    /* 0x24 */ f32 _24;
    /* 0x28 */ TVec3f _28;
    /* 0x34 */ TVec3f _34;
    /* 0x40 */ u16 _40;
    /* 0x42 */ u16 _42;
    /* 0x44 */ u16 _44;
    /* 0x46 */ u16 _46;
    /* 0x48 */ u16 _48;
    /* 0x4A */ u16 _4A;
    /* 0x4C */ f32 _4C;
    /* 0x50 */ f32 _50;
    /* 0x54 */ f32 _54;
    /* 0x58 */ bool _58;
    /* 0x59 */ bool _59;
    /* 0x5A */ u8 _5A[2];
};
