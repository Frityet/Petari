#include "camera/OriginalGameCamera.hpp"
#include "camera/PublishedCameraTarget.hpp"

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

    }  // namespace

    struct OriginalGameCamera::Impl {
        Impl(const smgpc::scene::StageZoneTransform &zone, const CameraParamChunk &param,
             const StageCameraTargetState &initial_target, float default_fovy,
             const StageCameraCalculationState &initial_state)
            : zone_transform(zone), camera_param(param), default_fovy_degrees(default_fovy),
              man("OriginalGameCameraMan"), camera("OriginalGameCameraParallel"),
              man_pose(man.mPoseParam), camera_pose(camera.mPoseParam), height(camera.mVPan),
              height_next(height->mNextParam), height_current(height->mCurrParam) {
            camera.mCameraMan = &man;

            CameraLocalUtil::setWatchPos(&man, TVec3f(0.0F, 0.0F, 300.0F));
            CameraLocalUtil::setLocalOffset(&man, game_vec(initial_state.local_offset));
            apply_parameter();

            target.publish(initial_target);
            const auto binding = smgpc::compat::ScopedCameraTargetBinding(
                camera, target, smgpc::compat::OriginalCameraMode::Game);
            camera.reset();
            CameraLocalUtil::calcSafePose(&man, &camera);
        }

        void apply_parameter() {
            const auto &param = camera_param;
            const auto &zone = zone_transform;
            const auto default_fovy = default_fovy_degrees;
            // CamTranslatorParallel::setParam's resource-to-controller units.
            // Constructing CameraParamChunk requires the complete CameraHolder;
            // this adapter calls the real controller setter with those fields.
            camera.setParam(TVec2f(180.0F * param.general.angle_b / MR::pi(),
                                   180.0F * param.general.angle_a / MR::pi()),
                            param.general.dist, param.general.num1 == 1);
            for (auto row = 0U; row < 3U; ++row) {
                for (auto column = 0U; column < 4U; ++column) {
                    camera.mZoneMatrix.mMtx[row][column] = zone.matrix[row * 4U + column];
                }
            }

            // The remaining fields correspond to CameraManGame::applyParameter.
            CameraLocalUtil::setGlobalOffset(&camera, game_vec(param.extra.w_offset));
            CameraLocalUtil::setLocalOffset(&camera, CameraLocalUtil::getLocalOffset(&man));
            CameraLocalUtil::setFrontOffset(&camera, param.extra.l_offset);
            CameraLocalUtil::setUpperOffset(&camera, param.extra.l_offset_v);
            CameraLocalUtil::setFovy(&camera, param.is_on_use_fovy() ? param.extra.fovy : default_fovy);
            camera.mIsLOfsErpOff = param.is_l_offset_erp_off();
            CameraLocalUtil::setRoll(&camera, param.extra.roll);

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
        PublishedCameraTarget target;
        CameraMan man;
        CameraParallel camera;
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
                                         const StageCameraCalculationState &initial_state) {
        if (camera_param.camera_type != "CAM_TYPE_XZ_PARA") {
            throw std::invalid_argument("Original game camera does not support " + camera_param.camera_type + ".");
        }
        validate_original_camera_target(initial_target);
        _impl = std::make_unique<Impl>(zone_transform, camera_param, initial_target,
                                       default_fovy_degrees, initial_state);
    }

    OriginalGameCamera::~OriginalGameCamera() = default;

    void OriginalGameCamera::reset(const StageCameraTargetState &target) {
        _impl->target.publish(target);
        _impl->apply_parameter();
        const auto binding = smgpc::compat::ScopedCameraTargetBinding(
            _impl->camera, _impl->target, smgpc::compat::OriginalCameraMode::Game);
        // CameraManGame::checkReset preserves the manager's last safe pose
        // and local offset; calcSafePose runs after the following calc.
        _impl->camera.reset();
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
        if (!_impl->parameter_applied_for_reset) {
            _impl->apply_parameter();
        }
        const auto binding = smgpc::compat::ScopedCameraTargetBinding(
            _impl->camera, _impl->target, smgpc::compat::OriginalCameraMode::Game);
        if (_impl->camera.calc() != &_impl->target) {
            throw std::logic_error("Original game camera returned a different target owner.");
        }
        CameraLocalUtil::calcSafePose(&_impl->man, &_impl->camera);
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
