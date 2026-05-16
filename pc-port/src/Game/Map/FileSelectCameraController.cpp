#include "Game/Map/FileSelectCameraController.hpp"

#include <algorithm>

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace {
    constexpr auto cMoveToFarPointFrames = 60.0F;

    NEW_NERVE(FileSelectCameraControllerNrvTitle, FileSelectCameraController, Title);
    NEW_NERVE(FileSelectCameraControllerNrvMoveToFarPoint, FileSelectCameraController, MoveToFarPoint);
    NEW_NERVE(FileSelectCameraControllerNrvFarPoint, FileSelectCameraController, FarPoint);

    [[nodiscard]] smgpc::game::CameraParamVec3 lerp_vec(const smgpc::game::CameraParamVec3& from, const smgpc::game::CameraParamVec3& to, float t) {
        return smgpc::game::CameraParamVec3{
            .x = from.x + ((to.x - from.x) * t),
            .y = from.y + ((to.y - from.y) * t),
            .z = from.z + ((to.z - from.z) * t),
        };
    }

    [[nodiscard]] smgpc::game::CameraPoseCompat lerp_pose(const smgpc::game::CameraPoseCompat& from, const smgpc::game::CameraPoseCompat& to,
                                                          float t) {
        return smgpc::game::CameraPoseCompat{
            .eye = lerp_vec(from.eye, to.eye, t),
            .watch = lerp_vec(from.watch, to.watch, t),
            .up = to.up,
            .fovy_degrees = from.fovy_degrees + ((to.fovy_degrees - from.fovy_degrees) * t),
            .aspect_ratio = to.aspect_ratio,
            .near_clip = to.near_clip,
            .far_clip = to.far_clip,
        };
    }

    void note_camera_event(std::string_view message) {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->note_debug_event(message);
        }
    }
}  // namespace

FileSelectCameraController::FileSelectCameraController(const char* pName)
    : NerveExecutor(pName), mCameraPose(smgpc::game::file_select_title_camera_pose()), mMoveStartPose(mCameraPose) {
    initNerve(&FileSelectCameraControllerNrvTitle::sInstance);
}

void FileSelectCameraController::update() {
    updateNerve();
}

void FileSelectCameraController::goToFarPoint() {
    setNerve(&FileSelectCameraControllerNrvMoveToFarPoint::sInstance);
}

bool FileSelectCameraController::isAtFarPoint() const {
    return isNerve(&FileSelectCameraControllerNrvFarPoint::sInstance);
}

bool FileSelectCameraController::isToOrAtFarPoint() const {
    return isNerve(&FileSelectCameraControllerNrvMoveToFarPoint::sInstance) || isAtFarPoint();
}

void FileSelectCameraController::exeTitle() {
    if (MR::isFirstStep(this)) {
        mCameraPose = smgpc::game::file_select_title_camera_pose();
        mMoveStartPose = mCameraPose;
    }
}

void FileSelectCameraController::exeMoveToFarPoint() {
    if (MR::isFirstStep(this)) {
        mMoveStartPose = mCameraPose;
        note_camera_event("FileSelectCameraController started far-point transition");
    }

    const auto progress = std::clamp(static_cast< float >(getNerveStep()) / cMoveToFarPointFrames, 0.0F, 1.0F);
    const auto eased_progress = progress * progress;
    mCameraPose = lerp_pose(mMoveStartPose, smgpc::game::file_select_far_camera_pose(), eased_progress);

    if (getNerveStep() >= static_cast< s32 >(cMoveToFarPointFrames)) {
        setNerve(&FileSelectCameraControllerNrvFarPoint::sInstance);
    }
}

void FileSelectCameraController::exeFarPoint() {
    mCameraPose = smgpc::game::file_select_far_camera_pose();
}

const smgpc::game::CameraPoseCompat& FileSelectCameraController::getCameraPose() const {
    return mCameraPose;
}

s32 FileSelectCameraController::getFarPointTransitionStep() const {
    if (!isNerve(&FileSelectCameraControllerNrvMoveToFarPoint::sInstance)) {
        return 0;
    }

    return getNerveStep();
}
