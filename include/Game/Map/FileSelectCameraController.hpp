#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "JSystem/JGeometry.hpp"
#include <revolution.h>

class FileSelectCameraController : public LiveActor {
public:
    FileSelectCameraController(const char*);

    virtual ~FileSelectCameraController();
    virtual void init(const JMapInfoIter&);
    virtual void appear();
    virtual void kill();
    virtual void control();

    void goToFarPoint();
    void goToNearPoint(const TVec3f&);
    bool isAtFarPoint() const;
    bool isAtNearPoint() const;
    bool isToOrAtFarPoint() const;
    bool isToOrAtNearPoint() const;

    void exeTitle();
    void exeMoveToFarPoint();
    void exeFarPoint();
    void exeMoveToNearPoint();
    void exeNearPoint();

    /* 0x8C */ TVec3f _8C;
    /* 0x98 */ TVec3f _98;
    /* 0xA4 */ TVec3f _A4;
    /* 0xB0 */ TVec3f _B0;
    /* 0xBC */ f32 _BC;
    /* 0xC0 */ f32 _C0;
    /* 0xC4 */ TVec3f _C4;
};
