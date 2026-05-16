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
};
