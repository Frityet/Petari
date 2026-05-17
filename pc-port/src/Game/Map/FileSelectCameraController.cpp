#include "Game/Map/FileSelectCameraController.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    const auto cFarTarget = TVec3f{0.0F, 800.0F, 0.0F};
    const auto cFarPoint = TVec3f{0.0F, 0.0F, 15000.0F};
    const auto cNearTargetOffset = TVec3f{0.0F, 1100.0F, 0.0F};
    const auto cNearPointOffset = TVec3f{0.0F, 0.0F, 4800.0F};

    NEW_NERVE(FileSelectCameraControllerNrvTitle, FileSelectCameraController, Title);
    NEW_NERVE(FileSelectCameraControllerNrvMoveToFarPoint, FileSelectCameraController, MoveToFarPoint);
    NEW_NERVE(FileSelectCameraControllerNrvFarPoint, FileSelectCameraController, FarPoint);
    NEW_NERVE(FileSelectCameraControllerNrvMoveToNearPoint, FileSelectCameraController, MoveToNearPoint);
    NEW_NERVE(FileSelectCameraControllerNrvNearPoint, FileSelectCameraController, NearPoint);

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

    [[nodiscard]] smgpc::game::CameraPoseCompat file_select_near_camera_pose(const smgpc::game::CameraParamVec3& basePosition) {
        constexpr auto cNearTargetYOffset = 1100.0F;
        constexpr auto cNearPointZOffset = 4800.0F;
        const auto watch = smgpc::game::CameraParamVec3{
            .x = basePosition.x,
            .y = basePosition.y + cNearTargetYOffset,
            .z = basePosition.z,
        };

        return smgpc::game::CameraPoseCompat{
            .eye = {watch.x, watch.y, watch.z + cNearPointZOffset},
            .watch = watch,
            .up = {0.0F, 1.0F, 0.0F},
            .fovy_degrees = 50.0F,
            .aspect_ratio = 608.0F / 456.0F,
            .near_clip = 100.0F,
            .far_clip = 800000.0F,
        };
    }
}  // namespace

FileSelectCameraController::FileSelectCameraController(const char* pName) : LiveActor(pName) {
    _A4.set(0.0F, 0.0F, 0.0F);
    _B0.set(0.0F, 0.0F, 0.0F);
    _BC = 60.0F;
    _C0 = 60.0F;
    _C4.set(0.0F, 1.0F, 0.0F);
}

FileSelectCameraController::~FileSelectCameraController() = default;

void FileSelectCameraController::init(const JMapInfoIter&) {
    MR::connectToSceneMapObjMovement(this);
    MR::invalidateClipping(this);
    MR::initActorCameraProgrammable(this);
    initNerve(&FileSelectCameraControllerNrvTitle::sInstance);
    makeActorDead();
}

void FileSelectCameraController::appear() {
    LiveActor::appear();
    setNerve(&FileSelectCameraControllerNrvTitle::sInstance);
    MR::startActorCameraProgrammable(this, -1);
}

void FileSelectCameraController::kill() {
    LiveActor::kill();
    MR::endActorCameraProgrammable(this, -1, true);
}

void FileSelectCameraController::goToFarPoint() {
    setNerve(&FileSelectCameraControllerNrvMoveToFarPoint::sInstance);
}

void FileSelectCameraController::goToNearPoint(const TVec3f& rPos) {
    auto target = rPos;
    target.add(cNearTargetOffset);
    _8C.set(target);
    setNerve(&FileSelectCameraControllerNrvMoveToNearPoint::sInstance);
}

bool FileSelectCameraController::isAtFarPoint() const {
    return isNerve(&FileSelectCameraControllerNrvFarPoint::sInstance);
}

bool FileSelectCameraController::isToOrAtFarPoint() const {
    return isNerve(&FileSelectCameraControllerNrvMoveToFarPoint::sInstance) || isAtFarPoint();
}

bool FileSelectCameraController::isAtNearPoint() const {
    return isNerve(&FileSelectCameraControllerNrvNearPoint::sInstance);
}

bool FileSelectCameraController::isToOrAtNearPoint() const {
    return isNerve(&FileSelectCameraControllerNrvMoveToNearPoint::sInstance) || isAtNearPoint();
}

void FileSelectCameraController::exeTitle() {
    if (MR::isFirstStep(this)) {
        _A4.set(cFarTarget.x, cFarTarget.y + 15000.0F, cFarTarget.z);
        mPosition.set(cFarPoint.x, cFarTarget.y + 15000.0F, cFarPoint.z);
        _C4.set(0.0F, 1.0F, 0.0F);
    }
}

void FileSelectCameraController::exeMoveToFarPoint() {
    if (MR::isFirstStep(this)) {
        mMoveStartPose = mCameraPose;
    }

    auto t = static_cast<f32>(getNerveStep()) / 60.0F;
    t *= t;

    const auto target_diff = cFarTarget - _A4;
    _A4.add(target_diff * t);

    _BC += (40.0F - _BC) * t;

    const auto point_diff = cFarPoint - mPosition;
    mPosition.add(point_diff * t);

    MR::setNerveAtStep(this, &FileSelectCameraControllerNrvFarPoint::sInstance, 60);
}

void FileSelectCameraController::exeFarPoint() {
    _A4 = cFarTarget;
    _BC = 40.0F;
    mPosition.set(cFarPoint);
}

void FileSelectCameraController::exeMoveToNearPoint() {
    if (MR::isFirstStep(this)) {
        mMoveStartPose = mCameraPose;
    }

    auto t = static_cast<f32>(getNerveStep()) / 60.0F;
    t *= t;

    const auto target_diff = _8C - _A4;
    _A4.add(target_diff * t);

    _BC += (50.0F - _BC) * t;

    const auto point_diff = point - mPosition;
    mPosition.add(point_diff * t);

    MR::setNerveAtStep(this, &FileSelectCameraControllerNrvNearPoint::sInstance, 60);
}

void FileSelectCameraController::exeNearPoint() {
    auto point = _8C;
    point.add(cNearPointOffset);

    _BC = 50.0F;
    mPosition.set(point);
}

void FileSelectCameraController::control() {
    MR::setProgrammableCameraParam(this, _B0, mPosition, _C4);
    MR::setProgrammableCameraParamFovy(this, _C0);
    _B0 = _A4;
    _C0 = _BC;
}
