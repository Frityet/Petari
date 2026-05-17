#pragma once

#include "Game/System/NerveExecutor.hpp"
#include "Game/compat/CameraPose.hpp"

class FileSelectCameraController : public NerveExecutor {
public:
    FileSelectCameraController(const char* pName);

    void update();
    void goToFarPoint();
    void goToNearPoint(const smgpc::game::CameraParamVec3& basePosition);
    bool isAtFarPoint() const;
    bool isToOrAtFarPoint() const;
    bool isAtNearPoint() const;
    bool isToOrAtNearPoint() const;

    void exeTitle();
    void exeMoveToFarPoint();
    void exeFarPoint();
    void exeMoveToNearPoint();
    void exeNearPoint();

    [[nodiscard]] const smgpc::game::CameraPoseCompat& getCameraPose() const;
    [[nodiscard]] s32 getFarPointTransitionStep() const;
    [[nodiscard]] s32 getNearPointTransitionStep() const;

private:
    smgpc::game::CameraPoseCompat mCameraPose;
    smgpc::game::CameraPoseCompat mMoveStartPose;
    smgpc::game::CameraParamVec3 mNearBasePosition{};
};
