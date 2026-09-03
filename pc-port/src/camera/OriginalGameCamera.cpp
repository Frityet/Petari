#include "camera/OriginalGameCamera.hpp"
#include "camera/PublishedCameraTarget.hpp"

#include "Game/Camera/CameraFixedPoint.hpp"
#include "Game/Camera/CameraHeightArrange.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraParallel.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraTargetObj.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/CameraLocalUtilRuntime.hpp"

#include <cmath>
#include <stdexcept>

namespace smgpc::camera {
    namespace {

        [[nodiscard]] TVec3f game_vec(const CameraParamVec3 &value) {
            return {value.x, value.y, value.z};
        }

        [[nodiscard]] CameraParamVec3 host_vec(const TVec3f &value) {
            return {value.x, value.y, value.z};
        }

        [[nodiscard]] std::unique_ptr<Camera> create_camera(const CameraParamChunk &param) {
            if (param.camera_type == "CAM_TYPE_XZ_PARA") {
                return std::make_unique<CameraParallel>("OriginalCameraParallel");
            }
            if (param.camera_type == "CAM_TYPE_EYEPOS_FIX") {
                return std::make_unique<CameraFixedPoint>("OriginalCameraFixedPoint");
            }
            throw std::invalid_argument("Original game camera does not support " + param.camera_type + ".");
        }

    }  // namespace

    struct OriginalGameCamera::Impl {
        Impl(const smgpc::scene::StageZoneTransform &zone, const CameraParamChunk &param,
             const StageCameraTargetState &initial_target, float default_fovy,
             const StageCameraCalculationState &initial_state,
             smgpc::compat::OriginalCameraMode mode, const CameraPoseParam *manager_seed,
             bool reset_local_offset, CameraTargetObj *original_target = nullptr)
            : zone_transform(zone), camera_param(param), default_fovy_degrees(default_fovy),
              mode(mode), man("OriginalGameCameraMan"), camera(create_camera(param)),
              man_pose(man.mPoseParam), camera_pose(camera->mPoseParam), height(camera->mVPan),
              height_next(height ? height->mNextParam : nullptr),
              height_current(height ? height->mCurrParam : nullptr) {
            camera->mCameraMan = &man;
            man.mRequestLOfsReset = reset_local_offset;

            if (manager_seed != nullptr) {
                man_pose->copyFrom(*manager_seed);
            } else if (mode != smgpc::compat::OriginalCameraMode::Event) {
                CameraLocalUtil::setWatchPos(&man, TVec3f(0.0F, 0.0F, 300.0F));
                CameraLocalUtil::setLocalOffset(&man, game_vec(initial_state.local_offset));
            }
            apply_parameter();

            if (original_target == nullptr) {
                target.publish(initial_target);
                original_target = &target;
            }
            const auto binding = smgpc::compat::ScopedCameraTargetBinding(*camera, *original_target, mode);
            camera->reset();
            if (mode == smgpc::compat::OriginalCameraMode::Event) {
                // CameraManEvent resets and calculates in the same phase,
                // then publishes one safe pose. FixedPoint::reset calculates
                // internally; preserve that accumulated state for calc.
                parameter_applied_for_reset = true;
            } else {
                CameraLocalUtil::calcSafePose(&man, camera.get());
            }
        }

        void apply_parameter() {
            const auto &param = camera_param;
            const auto &zone = zone_transform;
            const auto default_fovy = default_fovy_degrees;
            // The original translators' resource-to-controller fields/units.
            // Constructing CameraParamChunk requires the complete CameraHolder;
            // this adapter calls the real controller setter with those fields.
            if (param.camera_type == "CAM_TYPE_XZ_PARA") {
                static_cast<CameraParallel *>(camera.get())->setParam(
                    TVec2f(180.0F * param.general.angle_b / MR::pi(),
                           180.0F * param.general.angle_a / MR::pi()),
                    param.general.dist, param.general.num1 == 1);
            } else {
                static_cast<CameraFixedPoint *>(camera.get())->setParam(
                    game_vec(param.general.w_point), static_cast<u32>(param.general.num1));
            }
            for (auto row = 0U; row < 3U; ++row) {
                for (auto column = 0U; column < 4U; ++column) {
                    camera->mZoneMatrix.mMtx[row][column] = zone.matrix[row * 4U + column];
                }
            }

            // Shared CameraManGame::applyParameter / CameraManEvent::setExtraParam.
            CameraLocalUtil::setGlobalOffset(camera.get(), game_vec(param.extra.w_offset));
            CameraLocalUtil::setLocalOffset(camera.get(), CameraLocalUtil::getLocalOffset(&man));
            CameraLocalUtil::setFrontOffset(camera.get(), param.extra.l_offset);
            CameraLocalUtil::setUpperOffset(camera.get(), param.extra.l_offset_v);
            CameraLocalUtil::setFovy(camera.get(), param.is_on_use_fovy() ? param.extra.fovy : default_fovy);
            camera->mIsLOfsErpOff = param.is_l_offset_erp_off();
            CameraLocalUtil::setRoll(camera.get(), param.extra.roll);

            if (!height) {
                return;
            }
            height->resetParameter();
            height->mFocalScaleUpper = param.extra.upper;
            height->mFocalScaleLower = param.extra.lower;
            height->mGndInt = param.extra.gnd_int;
            height->mPosOffsetMinRiseLag = param.extra.u_play;
            height->mPosOffsetMinDropLag = param.extra.l_play;
            height->mRiseDelay = param.extra.push_delay;
            height->mDropDelay = param.extra.push_delay_low;
            height->mMaxRiseEaseTime = param.extra.u_down;
            height->mVPanUse = param.extra.v_pan_use != 0;
            auto axis = game_vec(param.extra.v_pan_axis);
            // CameraParamChunk::getVPanAxis.
            if (MR::isNearZero(axis)) {
                axis.set<f32>(0.0F, 1.0F, 0.0F);
            }
            MR::normalize(&axis);
            height->mVPanAxis.set(axis);
            height->mUpdateGlobalAxis = true;

        }

        [[nodiscard]] StageCameraPoseCalculation calculation() const {
            auto view = TPos3f{};
            smgpc::compat::calcCameraViewMtxFromPoseParam(&view, man_pose.get());
            auto up = TVec3f{};
            view.getYDir(up);
            return {
                .pose = {
                    .eye = host_vec(man_pose->mPos),
                    .watch = host_vec(man_pose->mWatchPos),
                    .up = host_vec(up),
                    .fovy_degrees = man_pose->mFovy,
                },
                .state = {
                    .local_offset = host_vec(man_pose->mLocalOffset),
                },
            };
        }

        smgpc::scene::StageZoneTransform zone_transform;
        CameraParamChunk camera_param;
        float default_fovy_degrees;
        smgpc::compat::OriginalCameraMode mode;
        PublishedCameraTarget target;
        CameraMan man;
        std::unique_ptr<Camera> camera;
        // These Game objects use stage-arena allocation and do not release
        // their children in destructors. The native owner retires them here.
        std::unique_ptr<CameraPoseParam> man_pose;
        std::unique_ptr<CameraPoseParam> camera_pose;
        std::unique_ptr<CameraHeightArrange> height;
        std::unique_ptr<CameraPoseParam> height_next;
        std::unique_ptr<CameraPoseParam> height_current;
        bool parameter_applied_for_reset = false;
    };

    OriginalGameCamera::OriginalGameCamera(const smgpc::scene::StageZoneTransform &zone_transform,
                                         const CameraParamChunk &camera_param,
                                         const StageCameraTargetState &initial_target,
                                         float default_fovy_degrees,
                                         const StageCameraCalculationState &initial_state,
                                         smgpc::compat::OriginalCameraMode mode,
                                         const CameraPoseParam *manager_seed,
                                         bool reset_local_offset) {
        if (camera_param.camera_type != "CAM_TYPE_XZ_PARA" &&
            camera_param.camera_type != "CAM_TYPE_EYEPOS_FIX") {
            throw std::invalid_argument("Original game camera does not support " + camera_param.camera_type + ".");
        }
        validate_original_camera_target(initial_target);
        _impl = std::make_unique<Impl>(zone_transform, camera_param, initial_target,
                                       default_fovy_degrees, initial_state, mode, manager_seed, reset_local_offset);
    }

    OriginalGameCamera::~OriginalGameCamera() = default;

    OriginalGameCamera::OriginalGameCamera(const smgpc::scene::StageZoneTransform &zone_transform,
                                         const CameraParamChunk &camera_param, CameraTargetObj &target,
                                         float default_fovy_degrees, const CameraPoseParam *manager_seed,
                                         bool reset_local_offset) {
        if (camera_param.camera_type != "CAM_TYPE_XZ_PARA" &&
            camera_param.camera_type != "CAM_TYPE_EYEPOS_FIX") {
            throw std::invalid_argument("Original game camera does not support " + camera_param.camera_type + ".");
        }
        _impl = std::make_unique<Impl>(zone_transform, camera_param, StageCameraTargetState{},
                                       default_fovy_degrees, StageCameraCalculationState{},
                                       smgpc::compat::OriginalCameraMode::Event, manager_seed,
                                       reset_local_offset, &target);
    }

    void OriginalGameCamera::reset(const StageCameraTargetState &target) {
        _impl->target.publish(target);
        _impl->apply_parameter();
        const auto binding = smgpc::compat::ScopedCameraTargetBinding(
            *_impl->camera, _impl->target, _impl->mode);
        // CameraManGame::checkReset preserves the manager's last safe pose
        // and local offset; calcSafePose runs after the following calc.
        _impl->camera->reset();
        _impl->parameter_applied_for_reset = true;
    }

    void OriginalGameCamera::reset_manager(const StageCameraTargetState &target) {
        _impl->target.publish(target);
        // CameraDirector::resetCameraMan seeds the current manager before
        // its deactivate/activate pair requests the next controller reset.
        const auto &published = _impl->target;
        const auto position = published.getPosition() - published.getFrontVec() * 800.0F +
                              published.getUpVec() * 300.0F;
        CameraLocalUtil::setPos(&_impl->man, position);
        CameraLocalUtil::setWatchPos(&_impl->man, published.getPosition());
        CameraLocalUtil::setUpVec(&_impl->man, published.getUpVec());
        CameraLocalUtil::setWatchUpVec(&_impl->man, published.getUpVec());
        reset(target);
    }

    StageCameraPoseCalculation OriginalGameCamera::calc(const StageCameraTargetState &target) {
        _impl->target.publish(target);
        return calc(_impl->target);
    }

    StageCameraPoseCalculation OriginalGameCamera::calc(CameraTargetObj &target) {
        if (!_impl->parameter_applied_for_reset) {
            _impl->apply_parameter();
        }
        const auto binding = smgpc::compat::ScopedCameraTargetBinding(
            *_impl->camera, target, _impl->mode);
        if (_impl->camera->calc() != &target) {
            throw std::logic_error("Original game camera returned a different target owner.");
        }
        CameraLocalUtil::calcSafePose(&_impl->man, _impl->camera.get());
        _impl->parameter_applied_for_reset = false;
        _impl->man.mRequestLOfsReset = false;
        return _impl->calculation();
    }

    StageCameraPoseCalculation OriginalGameCamera::calculation() const {
        return _impl->calculation();
    }

    const CameraPoseParam &OriginalGameCamera::pose_param() const {
        return *_impl->man_pose;
    }

}  // namespace smgpc::camera
