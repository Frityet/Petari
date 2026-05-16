#pragma once

#include "Game/System/NerveExecutor.hpp"
#include "Game/compat/CameraPose.hpp"

class FileSelectCameraController : public NerveExecutor {
public:
    FileSelectCameraController(const char* pName);

    void update();
    void goToFarPoint();
    bool isAtFarPoint() const;
    bool isToOrAtFarPoint() const;

    void exeTitle();
    void exeMoveToFarPoint();
    void exeFarPoint();

    [[nodiscard]] const smgpc::game::CameraPoseCompat& getCameraPose() const;
    [[nodiscard]] s32 getFarPointTransitionStep() const;

private:
    smgpc::game::CameraPoseCompat mCameraPose;
    smgpc::game::CameraPoseCompat mMoveStartPose;
};
