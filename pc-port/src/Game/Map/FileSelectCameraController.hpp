#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class FileSelectCameraController : public LiveActor {
public:
    FileSelectCameraController(const char* pName);
    ~FileSelectCameraController() override;

    void init(const JMapInfoIter& rIter) override;
    void appear() override;
    void kill() override;
    void control() override;
    void goToFarPoint();
    void goToNearPoint(const TVec3f& rPos);
    bool isAtFarPoint() const;
    bool isToOrAtFarPoint() const;
    bool isAtNearPoint() const;
    bool isToOrAtNearPoint() const;

    void exeTitle();
    void exeMoveToFarPoint();
    void exeFarPoint();
    void exeMoveToNearPoint();
    void exeNearPoint();

private:
    TVec3f _8C{};
    TVec3f _98{};
    TVec3f _A4{};
    TVec3f _B0{};
    f32 _BC = 60.0F;
    f32 _C0 = 60.0F;
    TVec3f _C4{0.0F, 1.0F, 0.0F};
};
