#include "Game/Map/FileSelectCameraController.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "JSystem/JGeometry/TVec.hpp"

namespace {
    const Vec cFarTarget = {0.0f, 800.0f, 0.0f};
    const Vec cFarPoint = {0.0f, 0.0f, 15000.0f};
    const Vec cNearTargetOffset = {0.0f, 1100.0f, 0.0f};
    const Vec cNearPointOffset = {0.0f, 0.0f, 4800.0f};

    NEW_NERVE(FileSelectCameraControllerNrvTitle, FileSelectCameraController, Title);
    NEW_NERVE(FileSelectCameraControllerNrvMoveToFarPoint, FileSelectCameraController, MoveToFarPoint);
    NEW_NERVE(FileSelectCameraControllerNrvFarPoint, FileSelectCameraController, FarPoint);
    NEW_NERVE(FileSelectCameraControllerNrvMoveToNearPoint, FileSelectCameraController, MoveToNearPoint);
    NEW_NERVE(FileSelectCameraControllerNrvNearPoint, FileSelectCameraController, NearPoint);
};  // namespace

FileSelectCameraController::FileSelectCameraController(const char* pName) : LiveActor(pName) {
    _A4.set< f32 >(0.0f, 0.0f, 0.0f);
    _B0.set< f32 >(0.0f, 0.0f, 0.0f);
    _BC = 60.0f;
    _C0 = 60.0f;
    _C4.set< f32 >(0.0f, 1.0f, 0.0f);
}

void FileSelectCameraController::init(const JMapInfoIter& rIter) {
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
    TVec3f offset(cNearTargetOffset);
    TVec3f target(rPos);
    target.add(offset);
    _8C.set< f32 >(target);
    setNerve(&FileSelectCameraControllerNrvMoveToNearPoint::sInstance);
}

bool FileSelectCameraController::isAtFarPoint() const {
    return isNerve(&FileSelectCameraControllerNrvFarPoint::sInstance);
}

bool FileSelectCameraController::isAtNearPoint() const {
    return isNerve(&FileSelectCameraControllerNrvNearPoint::sInstance);
}

bool FileSelectCameraController::isToOrAtFarPoint() const {
    return isNerve(&FileSelectCameraControllerNrvMoveToFarPoint::sInstance) || isNerve(&FileSelectCameraControllerNrvFarPoint::sInstance);
}

bool FileSelectCameraController::isToOrAtNearPoint() const {
    return isNerve(&FileSelectCameraControllerNrvMoveToNearPoint::sInstance) || isNerve(&FileSelectCameraControllerNrvNearPoint::sInstance);
}

void FileSelectCameraController::exeTitle() {
    if (MR::isFirstStep(this)) {
        _A4.set< f32 >(cFarTarget.x, cFarTarget.y + 15000.0f, cFarTarget.z);
        mPosition.set< f32 >(cFarPoint.x, cFarTarget.y + 15000.0f, cFarPoint.z);
        _C4.set< f32 >(0.0f, 1.0f, 0.0f);
    }
}

void FileSelectCameraController::exeMoveToFarPoint() {
    if (MR::isFirstStep(this)) {
        _C4.set< f32 >(0.0f, 1.0f, 0.0f);
    }

    f32 t = static_cast< f32 >(getNerveStep()) / 60.0f;
    t *= t;

    TVec3f target(cFarTarget);
    TVec3f targetDiff = target - _A4;
    _A4.add(targetDiff * t);

    _BC += (40.0f - _BC) * t;

    TVec3f point(cFarPoint);
    TVec3f pointDiff = point - mPosition;
    mPosition.add(pointDiff * t);

    MR::setNerveAtStep(this, &FileSelectCameraControllerNrvFarPoint::sInstance, 60);
}

void FileSelectCameraController::exeFarPoint() {
    TVec3f target(cFarTarget);
    _A4 = target;
    _BC = 40.0f;

    TVec3f point(cFarPoint);
    mPosition.set< f32 >(point);
}

void FileSelectCameraController::exeMoveToNearPoint() {
    TVec3f offset(cNearPointOffset);
    TVec3f point(_8C);
    point.add(offset);

    f32 t = static_cast< f32 >(getNerveStep()) / 60.0f;
    t *= t;

    TVec3f targetDiff = _8C - _A4;
    _A4.add(targetDiff * t);

    _BC += (50.0f - _BC) * t;

    TVec3f pointDiff = point - mPosition;
    mPosition.add(pointDiff * t);

    MR::setNerveAtStep(this, &FileSelectCameraControllerNrvNearPoint::sInstance, 60);
}

void FileSelectCameraController::exeNearPoint() {
    TVec3f offset(cNearPointOffset);
    TVec3f point(_8C);
    point.add(offset);

    _BC = 50.0f;
    mPosition.set< f32 >(point);
}

void FileSelectCameraController::control() {
    MR::setProgrammableCameraParam(this, _B0, mPosition, _C4);
    MR::setProgrammableCameraParamFovy(this, _C0);
    _B0 = _A4;
    _C0 = _BC;
}

FileSelectCameraController::~FileSelectCameraController() {}
