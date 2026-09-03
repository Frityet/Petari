#include "camera/OriginalCameraView.hpp"

#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraViewInterpolator.hpp"
#include "Game/Camera/OnlyCamera.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "compat/CameraLocalUtilRuntime.hpp"

namespace smgpc::camera {

    struct OriginalCameraView::Impl {
        Impl() : only_camera("OriginalOnlyCamera"), only_pose(only_camera.mPoseParam),
                 manual_manager("NativeSceneCameraMan"), manual_pose(manual_manager.mPoseParam),
                 binder(interpolator.mBinder),
                 filter(static_cast<TriangleFilterFunc *>(binder->mTriangleFilter)) {}

        OnlyCamera only_camera;
        std::unique_ptr<CameraPoseParam> only_pose;
        CameraMan manual_manager;
        std::unique_ptr<CameraPoseParam> manual_pose;
        CameraViewInterpolator interpolator;
        std::unique_ptr<Binder> binder;
        std::unique_ptr<TriangleFilterFunc> filter;
        smgpc::compat::CameraViewOutput output;
    };

    OriginalCameraView::OriginalCameraView() : _impl(std::make_unique<Impl>()) {}
    OriginalCameraView::~OriginalCameraView() = default;

    CameraPose OriginalCameraView::update(const CameraPoseParam &pose, const CameraTargetObj *target,
                                         const CameraPose &projection, OriginalCameraViewFlags flags) {
        _impl->manual_pose->copyFrom(pose);
        return update(_impl->manual_manager, target, projection, flags);
    }

    CameraPose OriginalCameraView::update(CameraMan &manager, const CameraTargetObj *target,
                                         const CameraPose &projection, OriginalCameraViewFlags flags) {
        auto &interpolator = _impl->interpolator;
        // CameraDirector::switchAntiOscillation / calcPose.
        interpolator.mIsAntiOscillation = !(flags.interpolation_off || interpolator.mIsForceCameraChange);
        if (flags.collision_off) {
            interpolator.mIsCollisionOff = true;
        }
        if (flags.correcting_position_off) {
            interpolator.mIsCorrectErpPositionOn = false;
        }
        if (flags.zero_frame_move_off) {
            _impl->only_camera.mIsZeroFrameMoveOff = true;
        }
        _impl->only_camera.calcPose(&manager);
        const auto &pose = *_impl->only_pose;
        auto matrix = TPos3f{};
        smgpc::compat::calcCameraViewMtxFromPoseParam(&matrix, &pose);
        const auto binding = smgpc::compat::ScopedCameraViewOutput(_impl->output);
        // CameraDirector gets FOV from the manager, not OnlyCamera's pose.
        interpolator.updateCameraMtx(matrix, pose.getWatchPos(), target, manager.mPoseParam->mFovy);
        manager.mMatrix.set(_impl->output.inverse_view);

        auto result = projection;
        auto eye = TVec3f{};
        auto up = TVec3f{};
        auto back = TVec3f{};
        _impl->output.inverse_view.getTrans(eye);
        _impl->output.inverse_view.getYDir(up);
        _impl->output.inverse_view.getZDir(back);
        // The output is a view matrix. Preserve its direction independently
        // of the manager's watch point, which damping may no longer face.
        const auto focus_distance = (pose.mWatchPos - pose.mPos).length();
        const auto watch = eye - back * (focus_distance > 0.0F ? focus_distance : 1.0F);
        result.eye = {eye.x, eye.y, eye.z};
        result.watch = {watch.x, watch.y, watch.z};
        result.up = {up.x, up.y, up.z};
        result.fovy_degrees = _impl->output.fovy;
        return result;
    }

    void OriginalCameraView::request_pose_reset() { _impl->only_camera.mIsResetting = true; }
    OnlyCamera &OriginalCameraView::pose_processor() { return _impl->only_camera; }
    const CameraPoseParam &OriginalCameraView::processed_pose() const { return *_impl->only_pose; }

    void OriginalCameraView::set_interpolation(std::uint32_t frames, bool start_camera_active) {
        _impl->interpolator.setInterpolation(frames);
        // CameraDirector::setInterpolation suppresses collision for a cut.
        if (frames == 0U && !start_camera_active) {
            _impl->interpolator.mIsCollisionOff = true;
        }
    }

    CameraViewInterpolator &OriginalCameraView::original() { return _impl->interpolator; }
    const CameraViewInterpolator &OriginalCameraView::original() const { return _impl->interpolator; }
    const smgpc::compat::CameraViewOutput &OriginalCameraView::output() const { return _impl->output; }

}  // namespace smgpc::camera
