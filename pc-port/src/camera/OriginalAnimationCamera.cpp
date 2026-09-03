#include "camera/OriginalAnimationCamera.hpp"

#include "Game/Camera/CameraAnim.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "camera/PublishedCameraTarget.hpp"
#include "compat/CameraLocalUtilRuntime.hpp"

#include <cmath>
#include <stdexcept>

namespace smgpc::camera {
    namespace {
        void validate_speed(float speed) {
            if (!std::isfinite(speed) || !(speed > 0.0F)) {
                throw std::invalid_argument("Original animation camera requires a positive finite speed.");
            }
        }

        CameraParamVec3 host_vec(const TVec3f &value) {
            return {value.x, value.y, value.z};
        }
    }

    struct OriginalAnimationCamera::Impl {
        Impl(const CameraAnimation &animation, const StageCameraTargetState &initial_target,
             float speed, const CameraPoseParam *manager_seed)
            : data(animation.native_data()), man("OriginalAnimationCameraMan"),
              camera("OriginalAnimationCamera"), man_pose(man.mPoseParam),
              camera_pose(camera.mPoseParam),
              linear_accessor(camera.mDataAccessor), key_accessor(camera.mKeyDataAccessor) {
            if (manager_seed != nullptr) {
                man_pose->copyFrom(*manager_seed);
            }
            camera.mCameraMan = &man;
            // loadBin and both original accessors only read this block. The
            // immutable owner retains its alignment and lifetime independently
            // of the declaration/catalog which supplied the animation.
            camera.setParam(const_cast<u8*>(data.bytes().data()), speed);
            target.publish(initial_target);
            const auto binding = smgpc::compat::ScopedCameraTargetBinding(
                camera, target, smgpc::compat::OriginalCameraMode::Event);
            camera.reset();
        }

        CameraPose pose() const {
            auto view = TPos3f{};
            smgpc::compat::calcCameraViewMtxFromPoseParam(&view, man_pose.get());
            auto up = TVec3f{};
            view.getYDir(up);
            return {.eye = host_vec(man_pose->mPos), .watch = host_vec(man_pose->mWatchPos),
                    .up = host_vec(up), .fovy_degrees = man_pose->mFovy};
        }

        NativeCameraAnimationData data;
        PublishedCameraTarget target;
        CameraMan man;
        CameraAnim camera;
        // Original Game allocations use a stage arena. Retire those children
        // explicitly with the native controller's ownership lifetime.
        std::unique_ptr<CameraPoseParam> man_pose;
        std::unique_ptr<CameraPoseParam> camera_pose;
        std::unique_ptr<CamAnmDataAccessor> linear_accessor;
        std::unique_ptr<KeyCamAnmDataAccessor> key_accessor;
    };

    OriginalAnimationCamera::OriginalAnimationCamera(
        const CameraAnimation &animation, const StageCameraTargetState &target,
        float speed, const CameraPoseParam *manager_seed) {
        validate_speed(speed);
        validate_original_camera_target(target);
        if (animation.native_data().bytes().empty()) {
            throw std::invalid_argument("Original animation camera requires a decoded CANM or CKAN resource.");
        }
        _impl = std::make_unique<Impl>(animation, target, speed, manager_seed);
    }

    OriginalAnimationCamera::~OriginalAnimationCamera() = default;

    CameraPose OriginalAnimationCamera::calc(const StageCameraTargetState &target) {
        _impl->target.publish(target);
        const auto binding = smgpc::compat::ScopedCameraTargetBinding(
            _impl->camera, _impl->target, smgpc::compat::OriginalCameraMode::Event);
        (void)_impl->camera.calc();
        // The existing original helper uses the same finite-pose arithmetic
        // as CameraManEvent::setSafePose, including its 300-unit watch distance.
        CameraLocalUtil::calcSafePose(&_impl->man, &_impl->camera);
        return _impl->pose();
    }

    CameraPose OriginalAnimationCamera::pose() const { return _impl->pose(); }
    const CameraPoseParam &OriginalAnimationCamera::pose_param() const { return *_impl->man_pose; }
    float OriginalAnimationCamera::current_frame() const { return _impl->camera.mCurrentFrame; }
    bool OriginalAnimationCamera::is_end() const { return _impl->camera.isAnimEnd(); }

    void OriginalAnimationCamera::set_speed(float speed) {
        validate_speed(speed);
        _impl->camera.mSpeed = speed;
    }

    void OriginalAnimationCamera::set_paused(bool paused) { _impl->camera.mIsPaused = paused; }

}  // namespace smgpc::camera
