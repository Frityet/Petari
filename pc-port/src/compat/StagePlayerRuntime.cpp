#include "compat/StagePlayerRuntime.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/StageGravityService.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace smgpc::compat {
    namespace {
        constexpr auto VECTOR_EPSILON = 1.0e-6F;
        constexpr auto PLAYER_PUNCH_MESSAGE = 0x1U;

        [[nodiscard]] TVec3f normalized_or(TVec3f value, const TVec3f &fallback) {
            const auto magnitude = value.length();
            if (std::isfinite(magnitude) && magnitude > VECTOR_EPSILON) {
                value.scale(1.0F / magnitude);
                return value;
            }
            return fallback;
        }

        [[nodiscard]] TVec3f tangent_to(const TVec3f &value, const TVec3f &up) {
            return value - up * value.dot(up);
        }

        [[nodiscard]] TVec3f stage_vec(const smgpc::camera::CameraParamVec3 &value) {
            return {value.x, value.y, value.z};
        }

        [[nodiscard]] smgpc::camera::CameraParamVec3 camera_vec(const TVec3f &value) {
            return {.x = value.x, .y = value.y, .z = value.z};
        }

        [[nodiscard]] TVec3f local_coordinates(const TVec3f &value, const StagePlayerBasis &basis) {
            return {value.dot(basis.side), value.dot(basis.up), value.dot(basis.front)};
        }

        [[nodiscard]] TVec3f world_coordinates(const TVec3f &value, const StagePlayerBasis &basis) {
            return basis.side * value.x + basis.up * value.y + basis.front * value.z;
        }

        [[nodiscard]] TVec3f start_vec(const std::array<f32, 3U> &value) {
            return {value[0U], value[1U], value[2U]};
        }

        [[nodiscard]] smgpc::render::J3dMatrix3x4 stage_player_matrix(const StagePlayerBasis &basis,
                                                                      const TVec3f &position) {
            return {{
                basis.side.x,
                basis.up.x,
                basis.front.x,
                position.x,
                basis.side.y,
                basis.up.y,
                basis.front.y,
                position.y,
                basis.side.z,
                basis.up.z,
                basis.front.z,
                position.z,
            }};
        }
    }  // namespace

    StagePlayerBasis calculate_stage_player_basis(const TVec3f &gravity, const TVec3f &preferred_front) {
        const auto up = normalized_or(-gravity, {0.0F, 1.0F, 0.0F});
        auto front = tangent_to(preferred_front, up);
        if (front.squared() <= VECTOR_EPSILON) {
            const auto fallback = std::abs(up.z) < 0.9F ? TVec3f{0.0F, 0.0F, 1.0F} : TVec3f{1.0F, 0.0F, 0.0F};
            front = tangent_to(fallback, up);
        }
        front = normalized_or(front, {0.0F, 0.0F, 1.0F});
        auto side = normalized_or(up.cross(front), {1.0F, 0.0F, 0.0F});
        front = normalized_or(side.cross(up), front);
        return {.side = side, .up = up, .front = front};
    }

    TVec3f calculate_camera_relative_stage_player_input(float stick_x, float stick_y, const TVec3f &gravity,
                                                         const smgpc::camera::CameraPose &camera_pose,
                                                         const TVec3f &fallback_front, float dead_zone) {
        const auto raw_magnitude = std::sqrt(stick_x * stick_x + stick_y * stick_y);
        const auto clamped_dead_zone = std::clamp(dead_zone, 0.0F, 0.99F);
        if (!std::isfinite(raw_magnitude) || raw_magnitude <= clamped_dead_zone) {
            return {};
        }

        const auto up = calculate_stage_player_basis(gravity, fallback_front).up;
        auto camera_forward = tangent_to(stage_vec(camera_pose.watch) - stage_vec(camera_pose.eye), up);
        camera_forward = normalized_or(camera_forward,
                                       calculate_stage_player_basis(gravity, fallback_front).front);
        const auto camera_side = normalized_or(up.cross(camera_forward),
                                               calculate_stage_player_basis(gravity, fallback_front).side);
        auto direction = camera_side * stick_x + camera_forward * stick_y;
        direction = normalized_or(direction, {});
        const auto magnitude = std::clamp((raw_magnitude - clamped_dead_zone) / (1.0F - clamped_dead_zone), 0.0F, 1.0F);
        return direction * magnitude;
    }

    TVec3f calculate_stage_player_velocity(const TVec3f &velocity, const TVec3f &tangent_input,
                                            const TVec3f &gravity, bool on_ground, bool jump_triggered,
                                            const StagePlayerMotionConfig &config) {
        const auto gravity_direction = normalized_or(gravity, {0.0F, -1.0F, 0.0F});
        auto input = tangent_to(tangent_input, -gravity_direction);
        const auto input_length = input.length();
        if (input_length > 1.0F) {
            input.scale(1.0F / input_length);
        }

        const auto downward_speed = velocity.dot(gravity_direction);
        auto tangent_velocity = velocity - gravity_direction * downward_speed;
        const auto target_velocity = input * config.maximum_speed;
        const auto acceleration = on_ground ? config.ground_acceleration : config.air_acceleration;
        tangent_velocity += (target_velocity - tangent_velocity) * std::clamp(acceleration, 0.0F, 1.0F);

        if (on_ground) {
            const auto vertical = jump_triggered ? -gravity_direction * config.jump_speed :
                                                   gravity_direction * config.ground_snap_speed;
            return tangent_velocity + vertical;
        }

        const auto accelerated_downward_speed = std::min(downward_speed + config.gravity_acceleration,
                                                          config.terminal_speed);
        return tangent_velocity + gravity_direction * accelerated_downward_speed;
    }

    smgpc::camera::CameraPose make_stage_player_fallback_camera(const TVec3f &position, const TVec3f &gravity,
                                                                const TVec3f &preferred_front) {
        const auto basis = calculate_stage_player_basis(gravity, preferred_front);
        auto pose = smgpc::camera::CameraPose{};
        pose.eye = camera_vec(position - basis.front * 900.0F + basis.up * 350.0F);
        pose.watch = camera_vec(position + basis.up * 120.0F);
        pose.up = camera_vec(basis.up);
        return pose;
    }

    StagePlayerFollowCamera make_stage_player_follow_camera(const smgpc::camera::CameraPose &pose,
                                                            const TVec3f &position, const TVec3f &gravity,
                                                            const TVec3f &fallback_front) {
        const auto gravity_basis = calculate_stage_player_basis(gravity, fallback_front);
        auto reference_front = tangent_to(stage_vec(pose.watch) - stage_vec(pose.eye), gravity_basis.up);
        reference_front = normalized_or(reference_front, gravity_basis.front);
        const auto basis = calculate_stage_player_basis(gravity, reference_front);
        return {
            .reference_front = reference_front,
            .eye_local = local_coordinates(stage_vec(pose.eye) - position, basis),
            .watch_local = local_coordinates(stage_vec(pose.watch) - position, basis),
            .fovy_degrees = pose.fovy_degrees,
            .aspect_ratio = pose.aspect_ratio,
            .near_clip = pose.near_clip,
            .far_clip = pose.far_clip,
        };
    }

    smgpc::camera::CameraPose calculate_stage_player_follow_camera_pose(const StagePlayerFollowCamera &follow,
                                                                        const TVec3f &position,
                                                                        const TVec3f &gravity) {
        const auto basis = calculate_stage_player_basis(gravity, follow.reference_front);
        return {
            .eye = camera_vec(position + world_coordinates(follow.eye_local, basis)),
            .watch = camera_vec(position + world_coordinates(follow.watch_local, basis)),
            .up = camera_vec(basis.up),
            .fovy_degrees = follow.fovy_degrees,
            .aspect_ratio = follow.aspect_ratio,
            .near_clip = follow.near_clip,
            .far_clip = follow.far_clip,
        };
    }

    class StagePlayerActor final : public LiveActor {
    public:
        StagePlayerActor(smgpc::runtime::RuntimeContext &runtime, const smgpc::scene::StageStartInfo &start_info)
            : LiveActor("StagePlayer"), mRuntime(runtime), mStartInfo(start_info) {
            initModelManagerWithAnm("Mario", "MarioAnime", false);
            initHitSensor(2);
            (void)MR::addHitSensorPlayer(this, "body", 32U, 100.0F, {});
            mSpinSensor = MR::addHitSensorPlayer(this, "spin", 32U, 140.0F, {});

            // MarioActor uses a 60-unit sphere plus a 70-unit up-vector
            // binder offset. LiveActor's host binder stores that generic
            // offset as a scalar along the actor matrix's up column.
            initBinder(60.0F, 70.0F, 8U);
            mFlag.mIsNoBind = false;
            mFlag.mIsCalcGravity = true;
            mFlag.mIsInvalidClipping = true;
            MR::initLightCtrlForPlayer(this);
            MR::connectToScene(this, MR::MovementType_Player, MR::CalcAnimType_Player,
                               MR::DrawBufferType_Player, MR::DrawType_Player);
            reset_to_start();
            makeActorAppeared();
            mSpinSensor->invalidate();
            startBck("Wait", nullptr);
        }

        void reset_to_start() {
            mForcedMatrix.reset();
            mSpinFrames = 0U;
            mVisualSpinRadians = 0.0F;
            mSpinActive = false;
            mExternalMatrixAppliedThisFrame = false;
            mFlag.mIsNoBind = false;
            if (mSpinSensor != nullptr) {
                mSpinSensor->invalidate();
            }
            mPosition.set(start_vec(mStartInfo.world_position));
            mRotation.set(start_vec(mStartInfo.local_rotation));
            mVelocity.zero();
            mBindedGround = false;
            mBindedWall = false;
            mBindedRoof = false;

            auto gravity = TVec3f{};
            if (const auto *service = smgpc::scene::StageGravityService::active();
                service != nullptr && service->query(mPosition, &gravity) && gravity.length() > VECTOR_EPSILON) {
                mGravity.set(normalized_or(gravity, {0.0F, -1.0F, 0.0F}));
            } else {
                mGravity.set(normalized_or(-start_vec(mStartInfo.world_up), {0.0F, -1.0F, 0.0F}));
            }
            mPreferredFront = calculate_stage_player_basis(mGravity, start_vec(mStartInfo.world_front)).front;
            mObservedMatrixRevision = mRuntime.player_system().base_matrix_revision();
            calcAndSetBaseMtx();
        }

        void control() override {
            mExternalMatrixAppliedThisFrame = apply_external_matrix();
            if (mRuntime.player_system().consume_reset_condition_request()) {
                mVelocity.zero();
                mBindedGround = false;
            }

            auto movement_input = TVec3f{};
            auto jump_triggered = false;
            const auto control_enabled = mRuntime.player_system().is_control_enabled();
            if (control_enabled) {
                const auto stick = mRuntime.wpad().sub_stick(WPAD_CHAN0);
                const auto camera_pose = mRuntime.camera_system().effective_camera_pose();
                const auto fallback_pose = make_stage_player_fallback_camera(mPosition, mGravity, mPreferredFront);
                movement_input = calculate_camera_relative_stage_player_input(
                    stick.x, stick.y, mGravity, camera_pose.value_or(fallback_pose), mPreferredFront);
                jump_triggered = mRuntime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A);
            }

            const auto spin_triggered = control_enabled && mRuntime.player_system().is_swing_permitted() &&
                                        (mRuntime.wpad().is_core_swing_triggered(WPAD_CHAN0) ||
                                         mRuntime.wpad().is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_1));
            if (spin_triggered) {
                mSpinFrames = 20U;
#ifndef NDEBUG
                mRuntime.emit_semantic_trace_event("player", "stage_player_spin_triggered",
                                                  "input=core_swing;duration=20");
#endif
            }
            mSpinActive = mSpinFrames != 0U;
            if (mSpinActive) {
                --mSpinFrames;
                mVisualSpinRadians += 0.65F;
                mSpinSensor->validate();
            } else {
                mSpinSensor->invalidate();
            }

            if (!control_enabled && mForcedMatrix.has_value()) {
                // Demo/control code owns the complete matrix in this state.
                // Skipping binder response prevents a zero-velocity overlap
                // correction from perturbing the supplied translation.
                mFlag.mIsNoBind = true;
                mVelocity.zero();
                calcAndSetBaseMtx();
                return;
            }
            mFlag.mIsNoBind = false;
            if (control_enabled && !mExternalMatrixAppliedThisFrame) {
                mForcedMatrix.reset();
            }

            if (movement_input.squared() > VECTOR_EPSILON) {
                mPreferredFront = normalized_or(movement_input, mPreferredFront);
            }
            mVelocity = calculate_stage_player_velocity(mVelocity, movement_input, mGravity,
                                                        mBindedGround, jump_triggered);
            calcAndSetBaseMtx();
        }

        void calcAndSetBaseMtx() override {
            if (mForcedMatrix.has_value() &&
                (!mRuntime.player_system().is_control_enabled() || mExternalMatrixAppliedThisFrame)) {
                auto matrix = *mForcedMatrix;
                matrix.m[3U] = mPosition.x;
                matrix.m[7U] = mPosition.y;
                matrix.m[11U] = mPosition.z;
                setBaseMatrix(matrix);
                return;
            }

            const auto basis = calculate_stage_player_basis(mGravity, mPreferredFront);
            mPreferredFront = basis.front;
            if (mSpinActive) {
                const auto visual_front = basis.front * std::cos(mVisualSpinRadians) +
                                          basis.side * std::sin(mVisualSpinRadians);
                setBaseMatrix(stage_player_matrix(calculate_stage_player_basis(mGravity, visual_front), mPosition));
            } else {
                setBaseMatrix(stage_player_matrix(basis, mPosition));
            }
        }

        void attackSensor(HitSensor *sender, HitSensor *receiver) override {
            if (mSpinActive && sender == mSpinSensor) {
                // ACTMES_PLAYER_PUNCH is the original spin-attack message.
                // The reduced Game header does not yet expose its convenience
                // wrapper, so keep the missing surface in compatibility code.
                (void)MR::sendArbitraryMsg(PLAYER_PUNCH_MESSAGE, receiver, sender);
            }
        }

        void draw() const override {
            // RuntimeContext stores the exact pose passed into this draw
            // traversal before dispatching scheduler categories. Using it
            // keeps the player on the same frame/pose as the stage geometry.
            const auto &pose = mRuntime.last_camera_pose();
            if (!pose.has_value()) {
                return;
            }
            const_cast<StagePlayerActor *>(this)->drawModel(*pose, mRuntime.frame_index());
        }

        [[nodiscard]] const TVec3f &preferred_front() const {
            return mPreferredFront;
        }

    private:
        [[nodiscard]] bool apply_external_matrix() {
            auto &player = mRuntime.player_system();
            if (player.base_matrix_revision() == mObservedMatrixRevision) {
                return false;
            }
            mObservedMatrixRevision = player.base_matrix_revision();
            if (!player.has_forced_base_matrix()) {
                mForcedMatrix.reset();
                return false;
            }

            const auto matrix = player.base_matrix();
            mPosition.set(matrix[3U], matrix[7U], matrix[11U]);
            const auto front = TVec3f{matrix[2U], matrix[6U], matrix[10U]};
            mPreferredFront = calculate_stage_player_basis(mGravity, front).front;
            mForcedMatrix = smgpc::render::J3dMatrix3x4{std::array<float, 12U>{
                matrix[0U], matrix[1U], matrix[2U], matrix[3U],
                matrix[4U], matrix[5U], matrix[6U], matrix[7U],
                matrix[8U], matrix[9U], matrix[10U], matrix[11U],
            }};
            return true;
        }

        smgpc::runtime::RuntimeContext &mRuntime;
        smgpc::scene::StageStartInfo mStartInfo;
        TVec3f mPreferredFront{0.0F, 0.0F, 1.0F};
        HitSensor *mSpinSensor = nullptr;
        std::optional<smgpc::render::J3dMatrix3x4> mForcedMatrix;
        std::uint32_t mSpinFrames = 0U;
        float mVisualSpinRadians = 0.0F;
        bool mSpinActive = false;
        bool mExternalMatrixAppliedThisFrame = false;
        std::uint64_t mObservedMatrixRevision = 0U;
    };

    StagePlayerRuntime::StagePlayerRuntime(smgpc::runtime::RuntimeContext &runtime,
                                           const smgpc::scene::StageStartInfo &start_info)
        : _runtime(runtime), _start_info(start_info) {
        _runtime.player_system().reset_stage_state();
        _actor = std::make_unique<StagePlayerActor>(_runtime, _start_info);
        _runtime.player_system().attach_actor(*_actor);
    }

    StagePlayerRuntime::~StagePlayerRuntime() {
        _runtime.player_system().detach_actor(_actor.get());
        _actor.reset();
    }

    void StagePlayerRuntime::reset_to_start() {
        _actor->reset_to_start();
        _runtime.player_system().synchronize_attached_actor();
    }

    void StagePlayerRuntime::use_follow_camera_pose(const smgpc::camera::CameraPose &pose) {
        _follow_camera = make_stage_player_follow_camera(pose, _actor->mPosition, _actor->mGravity,
                                                         _actor->preferred_front());
        synchronize_after_movement();
    }

    void StagePlayerRuntime::use_fallback_follow_camera() {
        use_follow_camera_pose(make_stage_player_fallback_camera(_actor->mPosition, _actor->mGravity,
                                                                 _actor->preferred_front()));
    }

    void StagePlayerRuntime::synchronize_after_movement() {
        _runtime.player_system().synchronize_attached_actor();
        if (_follow_camera.has_value()) {
            _runtime.camera_system().set_game_camera_pose(calculate_stage_player_follow_camera_pose(
                *_follow_camera, _actor->mPosition, _actor->mGravity));
        }
    }

    LiveActor *StagePlayerRuntime::actor() const {
        return _actor.get();
    }

    const smgpc::scene::StageStartInfo &StagePlayerRuntime::start_info() const {
        return _start_info;
    }

}  // namespace smgpc::compat
