#include "CameraTargetTestSupport.hpp"
#include "Game/Camera/CameraAnim.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/Camera/CameraTargetObj.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PointGravity.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "camera/CameraAnimation.hpp"
#include "camera/EventCamera.hpp"
#include "camera/OriginalAnimationCamera.hpp"
#include "camera/OriginalGameCamera.hpp"
#include "compat/CameraLocalUtilRuntime.hpp"
#include "compat/CameraUtilCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/StageAuthoredData.hpp"
#include "scene/StageEventCameraBinding.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/dvd.h>
#include <aurora/wpad.hpp>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, float tolerance,
                      std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) +
                                     ": actual=" + std::to_string(actual) +
                                     ";expected=" + std::to_string(expected));
        }
    }

    void require_vector_near(const TVec3f& actual, const TVec3f& expected,
                             float tolerance, std::string_view message) {
        require_near(actual.x, expected.x, tolerance, message);
        require_near(actual.y, expected.y, tolerance, message);
        require_near(actual.z, expected.z, tolerance, message);
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset,
                    std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset,
                    std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t> &bytes, std::size_t offset,
                        float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    [[nodiscard]] JMapInfo make_actor_camera_row(s32 camera_set_id,
                                                 s32 zone_id) {
        constexpr auto cFieldCount = std::size_t{1U};
        constexpr auto cDataOffset = 0x10U + cFieldCount * 0x0cU;
        auto bytes = std::vector<std::uint8_t>(cDataOffset + 4U, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, cFieldCount);
        write_be32(bytes, 0x08U, cDataOffset);
        write_be32(bytes, 0x0cU, 4U);
        write_be32(bytes, 0x10U,
                   smgpc::resource::jmap_hash("CameraSetId"));
        write_be32(bytes, 0x14U, 0xffffffffU);
        write_be16(bytes, 0x18U, 0U);
        bytes[0x1aU] = 0U;
        bytes[0x1bU] = static_cast<std::uint8_t>(
            smgpc::resource::BcsvFieldType::Int32);
        write_be32(bytes, cDataOffset,
                   static_cast<std::uint32_t>(camera_set_id));
        auto info = JMapInfo::from_bcsv(bytes);
        info.setPlacedZoneId(zone_id);
        return info;
    }

    [[nodiscard]] std::vector<std::uint8_t> make_linear_canm() {
        constexpr auto cHeaderSize = std::size_t{0x20U};
        constexpr auto cComponentTableSize = std::size_t{8U * 8U};
        constexpr auto cValueOffset = cHeaderSize + cComponentTableSize;
        constexpr auto cValueCount = std::size_t{16U};
        auto bytes = std::vector<std::uint8_t>(
            cValueOffset + 4U + cValueCount * sizeof(float), 0U);
        std::copy_n("ANDO", 4U, bytes.begin());
        std::copy_n("CANM", 4U, bytes.begin() + 4U);
        write_be32(bytes, 0x08U, 1U);
        write_be32(bytes, 0x10U, 1U);
        write_be32(bytes, 0x18U, 2U);
        write_be32(bytes, 0x1cU, cComponentTableSize);
        for (auto component = std::size_t{}; component < 8U; ++component) {
            const auto offset = cHeaderSize + component * 8U;
            write_be32(bytes, offset, 2U);
            write_be32(bytes, offset + 4U,
                       static_cast<std::uint32_t>(component * 2U));
        }
        write_be32(bytes, cValueOffset,
                   static_cast<std::uint32_t>(cValueCount * sizeof(float)));
        const auto values = std::array<float, cValueCount>{
            0.0F,
            10.0F,
            100.0F,
            100.0F,
            500.0F,
            500.0F,
            0.0F,
            10.0F,
            100.0F,
            100.0F,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            45.0F,
            45.0F,
        };
        for (auto index = std::size_t{}; index < values.size(); ++index) {
            write_be_float(bytes, cValueOffset + 4U + index * 4U,
                           values[index]);
        }
        return bytes;
    }

    struct AnimationTrack {
        std::uint32_t count;
        std::uint32_t type;
        std::vector<float> values;
    };

    [[nodiscard]] std::vector<std::uint8_t> make_animation_tracks(
        bool keyed, std::uint32_t frame_count,
        const std::array<AnimationTrack, 8U>& tracks) {
        constexpr auto header_size = std::size_t{0x20U};
        const auto component_size = keyed ? std::size_t{12U} : std::size_t{8U};
        const auto table_size = tracks.size() * component_size;
        const auto value_offset = header_size + table_size;
        auto value_count = std::size_t{};
        for (const auto& track : tracks) {
            value_count += track.values.size();
        }
        auto bytes = std::vector<std::uint8_t>(value_offset + 4U + value_count * sizeof(float), 0U);
        std::copy_n("ANDO", 4U, bytes.begin());
        std::copy_n(keyed ? "CKAN" : "CANM", 4U, bytes.begin() + 4U);
        write_be32(bytes, 0x08U, 1U);
        write_be32(bytes, 0x10U, 1U);
        write_be32(bytes, 0x18U, frame_count);
        write_be32(bytes, 0x1cU, static_cast<std::uint32_t>(table_size));
        write_be32(bytes, value_offset, static_cast<std::uint32_t>(value_count * sizeof(float)));
        auto value_index = std::size_t{};
        for (auto index = std::size_t{}; index < tracks.size(); ++index) {
            const auto& track = tracks[index];
            const auto component_offset = header_size + index * component_size;
            write_be32(bytes, component_offset, track.count);
            write_be32(bytes, component_offset + 4U, static_cast<std::uint32_t>(value_index));
            if (keyed) {
                write_be32(bytes, component_offset + 8U, track.type);
            }
            for (const auto value : track.values) {
                write_be_float(bytes, value_offset + 4U + value_index * sizeof(float), value);
                ++value_index;
            }
        }
        return bytes;
    }

    [[nodiscard]] std::vector<std::uint8_t> make_four_frame_canm() {
        return make_animation_tracks(false, 4U, {
            AnimationTrack{4U, 0U, {0.0F, 10.0F, 30.0F, 60.0F}},
            AnimationTrack{1U, 0U, {100.0F}},
            AnimationTrack{1U, 0U, {500.0F}},
            AnimationTrack{4U, 0U, {0.0F, 10.0F, 30.0F, 60.0F}},
            AnimationTrack{1U, 0U, {100.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{4U, 0U, {0.0F, 20.0F, 40.0F, 60.0F}},
            AnimationTrack{4U, 0U, {45.0F, 55.0F, 65.0F, 75.0F}},
        });
    }

    [[nodiscard]] smgpc::camera::CameraAnimation make_target_basis_animation() {
        return smgpc::camera::CameraAnimation::from_bytes(make_animation_tracks(false, 32U, {
            AnimationTrack{1U, 0U, {10.0F}},
            AnimationTrack{1U, 0U, {20.0F}},
            AnimationTrack{1U, 0U, {100.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {20.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {45.0F}},
        }));
    }

    [[nodiscard]] std::vector<std::uint8_t> make_ckan_fovy_track(
        const AnimationTrack& fovy, std::uint32_t frame_count = 4U) {
        return make_animation_tracks(true, frame_count, {
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {500.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            fovy,
        });
    }

    class CameraPlayerFixture final : public LiveActor {
    public:
        explicit CameraPlayerFixture(smgpc::runtime::PlayerSystemService &player,
                                     bool initialize_target = true)
            : LiveActor("Camera state fixture"), _player(player) {
            _player.attach_actor(*this);
            _player.set_camera_target(std::make_unique<smgpc::tests::CameraTargetFixture>([this] {
                ++read_count;
                return camera_state;
            }));
            if (initialize_target) {
                _player.advance_camera_target(0U);
            }
        }

        ~CameraPlayerFixture() override { _player.detach_actor(this); }

        smgpc::camera::StageCameraTargetState camera_state{
            .ground_position = smgpc::camera::CameraParamVec3{},
            .gravity = smgpc::camera::CameraParamVec3{0.0F, -1.0F, 0.0F}};
        mutable unsigned read_count = 0U;

    private:
        smgpc::runtime::PlayerSystemService &_player;
    };

    class CameraTargetScene final {
    public:
        CameraTargetScene() : binding(holder) {
            require(holder.create(SceneObj_AreaObjContainer) != nullptr &&
                        holder.create(SceneObj_PlanetGravityManager) != nullptr,
                    "original camera targets require real area and gravity scene registries");
        }

        SceneObjHolder holder;
        smgpc::scene::SceneObjHolderBinding binding;
    };

    class CountedMatrixTarget final : public CameraTargetMtx {
    public:
        CountedMatrixTarget() : CameraTargetMtx("counted matrix camera target") {}

        void movement() override {
            ++movement_count;
            CameraTargetMtx::movement();
        }

        unsigned movement_count = 0U;
    };

    class ActorBaseMatrixFixture final : public LiveActor {
    public:
        ActorBaseMatrixFixture() : LiveActor("virtual base matrix camera target") { appear(); }

        MtxPtr getBaseMtx() const override {
            ++base_matrix_reads;
            return has_base_matrix ? matrix : nullptr;
        }

        mutable Mtx matrix{{0.0F, 0.0F, 4.0F, 999.0F},
                           {0.0F, 3.0F, 0.0F, 888.0F},
                           {-2.0F, 0.0F, 0.0F, 777.0F}};
        mutable unsigned base_matrix_reads = 0U;
        bool has_base_matrix = true;
    };

    class GravityInfoCameraTarget final : public CameraTargetActor {
    public:
        GravityInfoCameraTarget() : CameraTargetActor("gravity info camera target") {}

        GravityInfo* getGravityInfo() const override { return has_info ? &info : nullptr; }

        mutable GravityInfo info;
        bool has_info = false;
    };

    void set_player_matrix(smgpc::runtime::PlayerSystemService &player,
                           float x, float y, float z) {
        Mtx matrix{
            {1.0F, 0.0F, 0.0F, x},
            {0.0F, 1.0F, 0.0F, y},
            {0.0F, 0.0F, 1.0F, z},
        };
        player.set_base_matrix(matrix);
        static_cast<CameraPlayerFixture *>(player.attached_actor())->camera_state.position = {x, y, z};
    }

    [[nodiscard]] bool finite_pose(const smgpc::camera::CameraPose &pose) {
        return std::isfinite(pose.eye.x) && std::isfinite(pose.eye.y) &&
               std::isfinite(pose.eye.z) && std::isfinite(pose.watch.x) &&
               std::isfinite(pose.watch.y) &&
               std::isfinite(pose.watch.z) && std::isfinite(pose.up.x) &&
               std::isfinite(pose.up.y) && std::isfinite(pose.up.z) &&
               std::isfinite(pose.fovy_degrees);
    }

    [[nodiscard]] bool same_pose(const smgpc::camera::CameraPose &lhs,
                                 const smgpc::camera::CameraPose &rhs) {
        return lhs.eye.x == rhs.eye.x && lhs.eye.y == rhs.eye.y &&
               lhs.eye.z == rhs.eye.z && lhs.watch.x == rhs.watch.x &&
               lhs.watch.y == rhs.watch.y && lhs.watch.z == rhs.watch.z &&
               lhs.up.x == rhs.up.x && lhs.up.y == rhs.up.y &&
               lhs.up.z == rhs.up.z &&
               lhs.fovy_degrees == rhs.fovy_degrees &&
               lhs.aspect_ratio == rhs.aspect_ratio &&
               lhs.near_clip == rhs.near_clip &&
               lhs.far_clip == rhs.far_clip &&
               lhs.projection_offset_x == rhs.projection_offset_x &&
               lhs.projection_offset_y == rhs.projection_offset_y;
    }

    [[nodiscard]] smgpc::camera::CameraParamChunk make_fixed_point_param(std::int32_t up_mode) {
        auto param = smgpc::camera::CameraParamChunk{};
        param.camera_type = "CAM_TYPE_EYEPOS_FIX";
        param.general.num1 = up_mode;
        param.general.w_point = {0.0F, 0.0F, 600.0F};
        param.extra.w_offset = {};
        return param;
    }

    void test_fixed_point_offset_accumulation_and_safe_distance() {
        constexpr auto mode = smgpc::compat::OriginalCameraMode::Event;
        auto param = make_fixed_point_param(0);
        param.extra.l_offset = 100.0F;
        param.extra.l_offset_v = 60.0F;
        auto target = smgpc::camera::StageCameraTargetState{};
        target.last_move = {15.0F, 0.0F, 0.0F};
        auto camera = smgpc::camera::OriginalGameCamera({}, param, target, 45.0F, {}, mode);
        // Each original makeWatchPoint advances offset by 0.1. FixedPoint's
        // reset calls calc, followed by the normal frame's calc: 1 - 0.9^2.
        const auto first = camera.calc(target);
        require_near(first.state.local_offset.z, 19.0F, 0.0001F,
                     "FixedPoint reset and normal calc must accumulate the original offset twice without restoring its old value");
        require_near(first.pose.watch.y, 11.4F, 0.0001F,
                     "FixedPoint upper offset must use the same original accumulated interpolation");
        const auto second = camera.calc(target);
        require_near(second.pose.watch.z, 27.1F, 0.0001F,
                     "FixedPoint following frames must retain local offset rather than snap to the desired value");
        require_near(second.pose.watch.y, 16.26F, 0.0001F,
                     "FixedPoint vertical local offset must keep its preceding frame's state");
        camera.reset(target);
        const auto reset = camera.calc(target);
        require_near(reset.pose.watch.z, 40.951F, 0.0002F,
                     "resetting FixedPoint must continue from the retained manager offset through reset and normal calc");

        auto request_reset = smgpc::camera::OriginalGameCamera(
            {}, param, target, 45.0F, {}, mode, nullptr, true);
        const auto reset_offset = request_reset.calc(target);
        require_near(reset_offset.state.local_offset.z, 100.0F, 0.0001F,
                     "a zero-frame event's manager reset flag must initialize the full local offset");
        require_near(reset_offset.state.local_offset.y, 60.0F, 0.0001F,
                     "a manager local-offset reset must also initialize the full vertical offset");
        auto turned_target = target;
        turned_target.front = {1.0F, 0.0F, 0.0F};
        const auto after_request_reset = request_reset.calc(turned_target);
        require_near(after_request_reset.state.local_offset.x, 10.0F, 0.0001F,
                     "the manager local-offset reset flag must clear after the initial camera phase");
        require_near(after_request_reset.state.local_offset.z, 90.0F, 0.0001F,
                     "subsequent FixedPoint phases must return to the original movement-dependent interpolation");

        param.extra.flags |= 1U << 2U;
        target.last_move = {};
        auto instant = smgpc::camera::OriginalGameCamera({}, param, target, 45.0F, {}, mode);
        const auto instant_pose = instant.calc(target).pose;
        require_near(instant_pose.watch.z, 100.0F, 0.0001F,
                     "the authored local-offset interpolation-off flag must apply the full front offset while stationary");
        require_near(instant_pose.watch.y, 60.0F, 0.0001F,
                     "the authored interpolation-off flag must apply the full upper offset");

        param = make_fixed_point_param(0);
        param.general.w_point = {0.0F, 0.0F, 10.0F};
        auto short_view = smgpc::camera::OriginalGameCamera({}, param, target, 45.0F, {}, mode);
        const auto safe = short_view.calc(target).pose;
        require_near(safe.eye.z, 10.0F, 0.0001F,
                     "the safe-pose stage must preserve the authored FixedPoint eye");
        require_near(safe.watch.z, -290.0F, 0.0001F,
                     "the original event safe-pose rule must extend a short nonzero watch vector to 300 units");
    }

    void test_fixed_point_zone_and_transport_up() {
        constexpr auto mode = smgpc::compat::OriginalCameraMode::Event;
        auto zone = smgpc::scene::StageZoneTransform{};
        zone.matrix = {0.0F, -1.0F, 0.0F, 10.0F,
                       1.0F, 0.0F, 0.0F, 20.0F,
                       0.0F, 0.0F, 1.0F, 30.0F};
        auto param = make_fixed_point_param(0);
        param.general.w_point = {100.0F, 0.0F, -600.0F};
        auto target = smgpc::camera::StageCameraTargetState{};
        target.position = {10.0F, 120.0F, 30.0F};
        auto zone_up = smgpc::camera::OriginalGameCamera(zone, param, target, 45.0F, {}, mode);
        const auto zoned = zone_up.calc(target).pose;
        require_near(zoned.eye.x, 10.0F, 0.0001F, "FixedPoint eye must use zone rotation and translation");
        require_near(zoned.eye.y, 120.0F, 0.0001F, "FixedPoint eye must retain the rotated authored coordinate");
        require_near(zoned.eye.z, -570.0F, 0.0001F, "FixedPoint eye must retain the placed zone translation");
        require_near(zoned.up.x, -1.0F, 0.0001F,
                     "FixedPoint mode zero must rotate zone-up instead of borrowing target up");

        auto seed = CameraPoseParam{};
        seed.mWatchPos.set(0.0F, 0.0F, -600.0F);
        param = make_fixed_point_param(1);
        param.general.w_point = {};
        target.position = {0.0F, 0.0F, -600.0F};
        auto transported = smgpc::camera::OriginalGameCamera({}, param, target, 45.0F, {}, mode, &seed);
        (void)transported.calc(target);
        target.position = {0.0F, 600.0F, 0.0F};
        const auto north = transported.calc(target).pose;
        require_near(north.up.z, 1.0F, 0.0003F,
                     "mode one must transport the previous up vector when view direction turns from negative Z to positive Y");
        target.position = {600.0F, 0.0F, 0.0F};
        const auto east = transported.calc(target).pose;
        require_near(east.up.z, 1.0F, 0.0003F,
                     "mode one must preserve transported up through the following view rotation");
        target.position = {0.0F, 0.0F, -600.0F};
        const auto returned = transported.calc(target).pose;
        require_near(returned.up.x, 1.0F, 0.0003F,
                     "returning to the initial view direction must retain mode-one path-dependent orientation");
        require_near(returned.up.y, 0.0F, 0.0003F,
                     "mode one must not reset the transported orientation to zone-up each frame");
    }

    void test_fixed_point_mode_two_uses_global_player_up() {
        auto player = smgpc::runtime::PlayerSystemService{};
        auto global_actor = LiveActor("Explicit global camera player");
        player.attach_actor(global_actor, smgpc::runtime::PlayerActorBridge{
            .read_up_vector = [](const LiveActor&, TVec3f* destination) {
                destination->set(1.0F, 0.0F, 0.0F);
            }});
        struct DetachGuard {
            smgpc::runtime::PlayerSystemService& player;
            ~DetachGuard() { player.detach_actor(); }
        } detach{player};
        const auto player_binding = smgpc::compat::ScopedPlayerSystemServiceOverride(player);
        Mtx matrix{{1.0F, 0.0F, 0.0F, 0.0F},
                   {0.0F, 0.0F, -1.0F, 0.0F},
                   {0.0F, 1.0F, 0.0F, 0.0F}};
        player.set_base_matrix(matrix);
        const auto target = smgpc::camera::StageCameraTargetState{};
        auto camera = smgpc::camera::OriginalGameCamera({}, make_fixed_point_param(2), target,
            45.0F, {}, smgpc::compat::OriginalCameraMode::Event);
        const auto pose = camera.calc(target).pose;
        require_near(pose.up.x, 1.0F, 0.0001F,
                     "FixedPoint mode two must use the global player's original up getter independently of its render matrix and event target");
        require(camera.pose_param().mUpVec.x == 1.0F && camera.pose_param().mWatchUpVec.y == 1.0F,
                "mode two camera up must come from the player while watch-up remains the event target's up");
    }

    void test_actor_camera_info_pool_and_identity() {
        auto camera = smgpc::runtime::CameraSystemService{};
        auto first_catalog = smgpc::camera::EventCameraCatalog{};
        const auto baseline = camera.actor_camera_info_count();
        camera.attach_event_camera_catalog(first_catalog);
        {
            auto override =
                smgpc::compat::ScopedCameraSystemServiceOverride(camera);
            auto valid_row = make_actor_camera_row(16, 5);
            auto *first = MR::createActorCameraInfo(
                JMapInfoIter(&valid_row, 0));
            require(first != nullptr && first->mCameraSetID == 16 &&
                        first->mZoneID == 5,
                    "createActorCameraInfo must retain authored ID and placed zone");
            require(camera.actor_camera_info_count() == baseline + 1U,
                    "the stage event provider must own ActorCameraInfo storage");

            auto missing_row = make_actor_camera_row(-1, 5);
            auto *sentinel = first;
            require(!MR::createActorCameraInfoIfExist(
                        JMapInfoIter(&missing_row, 0), &sentinel) &&
                        sentinel == first &&
                        camera.actor_camera_info_count() == baseline + 1U,
                    "IfExist -1 must preserve the caller output and allocate nothing");

            auto rabbit = LiveActor("逃げウサギ集め");
            auto pipe = LiveActor("土管");
            require(MR::initMultiActorCameraNoInit(&rabbit, first, nullptr) &&
                        camera.is_event_camera_declared(
                            5, "逃げウサギ集め固有016"),
                    "unique actor-camera identity must include actor, 固有, and three-digit ID");
            require(MR::initMultiActorCameraNoInit(&pipe, first, "出現") &&
                        camera.is_event_camera_declared(5, "土管固有出現016"),
                    "multi-camera identity must preserve its authored suffix");
            auto common = ActorCameraInfo(0x8003, 7);
            require(MR::initMultiActorCameraNoInit(&pipe, &common, "出現") &&
                        camera.is_event_camera_declared(7, "土管共通出現003"),
                    "common camera identity must remove the 0x8000 flag before formatting");
        }
        camera.detach_event_camera_catalog(first_catalog);
        require(camera.actor_camera_info_count() == baseline,
                "first stage generation must release every owned ActorCameraInfo");

        auto second_catalog = smgpc::camera::EventCameraCatalog{};
        camera.attach_event_camera_catalog(second_catalog);
        {
            auto override =
                smgpc::compat::ScopedCameraSystemServiceOverride(camera);
            auto valid_row = make_actor_camera_row(54, 0);
            auto *second = MR::createActorCameraInfo(
                JMapInfoIter(&valid_row, 0));
            require(second != nullptr && second->mCameraSetID == 54 &&
                        camera.actor_camera_info_count() == baseline + 1U,
                    "second stage generation must receive fresh provider-owned storage");
        }
        camera.detach_event_camera_catalog(second_catalog);
        require(camera.actor_camera_info_count() == baseline,
                "second stage generation must also return to the allocation baseline");
    }

    void test_linear_canm_player_target() {
        const auto bytes = make_linear_canm();
        const auto animation =
            smgpc::camera::CameraAnimation::from_bytes(bytes);
        require(animation.format() ==
                        smgpc::camera::CameraAnimationFormat::Canm &&
                    animation.frame_count() == 2U,
                "synthetic CANM should retain format and frame count");

        auto player = smgpc::runtime::PlayerSystemService{};
        auto player_actor = CameraPlayerFixture(player);
        set_player_matrix(player, 100.0F, 0.0F, 0.0F);
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(5, "逃げチコDemoMeetTico",
                                              animation);
        camera.start_event_camera(
            5, "逃げチコDemoMeetTico",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 1.0F);
        require(!camera.active_event_camera_pose().has_value(),
                "an event request must defer its first pose until camera movement");
        camera.begin_frame(0U);
        const auto first = camera.active_event_camera_pose();
        require(first.has_value() && finite_pose(*first),
                "player-target CANM must produce a finite initial pose");
        require_near(first->eye.x, 100.0F, 0.001F,
                     "CANM eye must be transformed by the player matrix");

        set_player_matrix(player, 200.0F, 0.0F, 0.0F);
        camera.begin_frame(1U);
        const auto second = camera.active_event_camera_pose();
        require(second.has_value() &&
                    camera.event_camera_animation_frame(
                        5, "逃げチコDemoMeetTico") == 2 &&
                    second->eye.x > first->eye.x + 100.0F,
                "CANM must advance and continue following the player target");
        camera.end_event_camera(5, "逃げチコDemoMeetTico", true, -1);
        require(!camera.active_event_camera_key().has_value(),
                "ending the exact zone-qualified CANM must restore base-camera ownership");

        camera.declare_event_camera_animation(5, "actor lifetime", animation);
        auto actor = std::make_unique<LiveActor>("camera target");
        actor->appear();
        camera.start_event_camera(
            5, "actor lifetime",
            smgpc::camera::EventCameraTarget::target_actor(*actor), 0, 1.0F);
        actor.reset();
        auto rejected_destroyed_actor = false;
        try {
            camera.begin_frame(2U);
        } catch (const std::logic_error &) {
            rejected_destroyed_actor = true;
        }
        require(rejected_destroyed_actor,
                "event camera must reject a retired actor through registry membership before dereference");
    }

    void test_uninitialized_player_event_waits_for_camera_phase() {
        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player, false);
        auto* target = static_cast<smgpc::tests::CameraTargetFixture*>(player.camera_target());
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto animation = smgpc::camera::CameraAnimation::from_bytes(make_linear_canm());
        camera.declare_event_camera_animation(0, "first player event", animation);
        camera.declare_event_camera_animation(0, "replacement player event", animation);
        actor.camera_state.position.x = 100.0F;
        camera.start_event_camera(0, "first player event",
            smgpc::camera::EventCameraTarget::target_player(player), 0);
        require(camera.is_event_camera_active(0, "first player event") &&
                    !camera.active_event_camera_pose().has_value() &&
                    !player.camera_target_state().has_value() &&
                    target->movement_count == 0U && actor.read_count == 0U &&
                    camera.event_camera_animation_frame(0, "first player event") == 0,
                "an uninitialized player target must bind without movement or pose reads during the request");

        camera.pause_on_camera_director();
        camera.begin_frame(1U);
        require(!camera.active_event_camera_pose().has_value() &&
                    target->movement_count == 0U,
                "a paused director must leave the first event pending");
        actor.camera_state.position.x = 200.0F;
        camera.pause_off_camera_director();
        camera.begin_frame(2U);
        const auto first = camera.active_event_camera_pose();
        require(first.has_value() && target->movement_count == 1U &&
                    player.camera_target_state().has_value() &&
                    camera.event_camera_animation_frame(0, "first player event") == 1,
                "the first camera phase must initialize the target and then advance the sampled animation cursor");
        require_near(first->eye.x, 200.0F, 0.001F,
                     "the first pose must sample frame zero using the target state at camera movement");

        actor.camera_state.position.x = 400.0F;
        const auto reads_before_request = actor.read_count;
        camera.start_event_camera(0, "replacement player event",
            smgpc::camera::EventCameraTarget::retain(), 0);
        require(same_pose(*camera.active_event_camera_pose(), *first) &&
                    target->movement_count == 1U && actor.read_count == reads_before_request &&
                    camera.event_camera_animation_frame(0, "replacement player event") == 0,
                "a replacement request must preserve the visible pose and defer all target sampling");
        camera.pause_on_camera_director();
        camera.begin_frame(3U);
        require(same_pose(*camera.active_event_camera_pose(), *first) &&
                    target->movement_count == 1U,
                "a paused replacement event must retain the previous visible pose");
        camera.pause_off_camera_director();
        camera.begin_frame(4U);
        require_near(camera.active_event_camera_pose()->eye.x, 400.0F, 0.001F,
                     "the replacement event must first sample frame zero after unpausing");
        require(target->movement_count == 2U,
                "event replacement must advance the selected target once in its next camera phase");
    }

    void test_player_camera_capability_and_live_sampling() {
        auto player = smgpc::runtime::PlayerSystemService{};
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(0, "live player camera",
            smgpc::camera::CameraAnimation::from_bytes(make_linear_canm()));
        const auto target = smgpc::camera::EventCameraTarget::target_player(player);
        auto generic_actor = LiveActor("Player without camera capability");
        player.attach_actor(generic_actor);
        require(!player.camera_target_state().has_value(),
                "an attached generic actor must not fabricate player camera state");
        auto rejected = false;
        try {
            camera.start_event_camera(0, "live player camera", target, 0);
        } catch (const std::logic_error &) {
            rejected = true;
        }
        require(rejected && !camera.active_event_camera_pose().has_value(),
                "an available render matrix cannot substitute for player camera capability");

        auto actor = CameraPlayerFixture(player);
        actor.mPosition.set(-500.0F, -600.0F, -700.0F);
        actor.mVelocity.set(999.0F, 999.0F, 999.0F);
        actor.camera_state = {
            .position = {100.0F, 200.0F, 300.0F},
            .up = {0.0F, 1.0F, 0.0F},
            .front = {1.0F, 0.0F, 0.0F},
            .last_move = {4.0F, 5.0F, 6.0F},
            .ground_position = smgpc::camera::CameraParamVec3{7.0F, 8.0F, 9.0F},
            .gravity = smgpc::camera::CameraParamVec3{0.0F, -1.0F, 0.0F},
            .jumping = true,
            .fast_rise = true,
            .fast_drop = true,
            .side = smgpc::camera::CameraParamVec3{0.0F, 0.0F, -1.0F}};
        player.synchronize_attached_actor();
        const auto state = player.camera_target_state();
        require(state.has_value() && state->jumping && state->fast_rise && state->fast_drop &&
                    state->last_move.x == 4.0F && state->ground_position->y == 8.0F,
                "the typed bridge must preserve actual camera flags and geometry state");
        camera.start_event_camera(0, "live player camera", target, 0);
        camera.begin_frame(0U);
        const auto first = *camera.active_event_camera_pose();
        require_near(first.eye.x, 600.0F, 0.001F,
                     "CANM must use camera translation and front, not rendered player matrix");
        require_near(first.eye.y, 300.0F, 0.001F, "CANM must use camera up");
        require_near(first.eye.z, 300.0F, 0.001F, "CANM must use camera translation");

        const auto reads = actor.read_count;
        actor.camera_state.position.z = 900.0F;
        camera.begin_frame(1U);
        const auto moved = *camera.active_event_camera_pose();
        require(actor.read_count > reads, "event frames must sample the attached target object anew");
        require_near(moved.eye.z, 890.0F, 0.001F,
                     "live camera state and independent side axis must affect the next CANM sample");
        player.detach_actor(&actor);
        require(!player.camera_target_state().has_value(), "detachment must retire the camera capability");
        rejected = false;
        try {
            camera.begin_frame(2U);
        } catch (const std::logic_error &) {
            rejected = true;
        }
        require(rejected, "retained player targets must fail once their camera provider is detached");
    }

    void test_fractional_canm_camera_and_director_pause() {
        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player, false);
        actor.camera_state.position = {100.0F, 200.0F, 300.0F};
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(0, "fractional CANM",
            smgpc::camera::CameraAnimation::from_bytes(make_four_frame_canm()));
        camera.start_event_camera(0, "fractional CANM",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 0.5F);
        camera.begin_frame(1U);
        const auto first = *camera.active_event_camera_pose();
        require_near(first.eye.x, 100.0F, 0.001F, "CANM first phase must sample frame zero");
        require_near(first.fovy_degrees, 45.0F, 0.0001F, "CANM first phase must use its first FOV sample");
        camera.pause_on_camera_director();
        actor.camera_state.position.x = 300.0F;
        camera.begin_frame(2U);
        require(same_pose(*camera.active_event_camera_pose(), first),
                "director pause must freeze animation pose and target tracking together");
        camera.pause_off_camera_director();
        camera.begin_frame(3U);
        const auto fractional = *camera.active_event_camera_pose();
        require_near(fractional.eye.x, 305.0F, 0.001F,
                     "CANM must resume at frame one-half and use the latest target translation");
        require_near(fractional.watch.x, 305.0F, 0.001F,
                     "CANM watch coordinates must use the same fractional sample");
        require_near(fractional.fovy_degrees, 50.0F, 0.0001F,
                     "CANM fractional FOV must interpolate between authored samples");
        require_near(fractional.up.x, -0.17364818F, 0.0002F,
                     "CANM ten-degree twist must reach the original view-matrix roll");
        require_near(fractional.up.y, 0.98480775F, 0.0002F,
                     "CANM ten-degree twist must preserve the perpendicular up component");
        require(camera.event_camera_animation_frame(0, "fractional CANM") == 1 &&
                    !camera.is_event_camera_animation_end(0, "fractional CANM"),
                "director pause must not consume animation time");
    }

    void test_fractional_ckan_tangent_layouts() {
        // The two moving tracks have independent Hermite polynomials:
        // eyeX(t) = 2*t + 6.75*t^2 - 1.1875*t^3;
        // watchX(t) = 4*t + 12.5*t^2 - 2.125*t^3.
        // Nonzero type 7's unused first incoming and final outgoing tangents differ
        // deliberately, so swapping either tangent cannot satisfy the oracle.
        const auto bytes = make_animation_tracks(true, 4U, {
            AnimationTrack{2U, 0U, {0.0F, 0.0F, 60.0F, 4.0F, 40.0F, -30.0F}},
            AnimationTrack{1U, 0U, {100.0F}},
            AnimationTrack{1U, 0U, {500.0F}},
            AnimationTrack{2U, 7U, {0.0F, 0.0F, -900.0F, 120.0F, 4.0F, 80.0F, 60.0F, 999.0F}},
            AnimationTrack{1U, 0U, {100.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {50.0F}},
        });
        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player, false);
        actor.camera_state.position = {100.0F, 200.0F, 300.0F};
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(0, "fractional CKAN",
            smgpc::camera::CameraAnimation::from_bytes(bytes));
        camera.start_event_camera(0, "fractional CKAN",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 0.5F);
        camera.begin_frame(1U);
        camera.begin_frame(2U);
        const auto half = *camera.active_event_camera_pose();
        require_near(half.eye.x, 102.5390625F, 0.0001F,
                     "CKAN type-zero tangents must be converted from authored 30 Hz units");
        require_near(half.watch.x, 104.859375F, 0.0001F,
                     "CKAN nonzero type-seven interpolation must use outgoing then incoming tangents");
        require_near(half.eye.y, 300.0F, 0.0001F,
                     "single-value CKAN tracks must remain constant");
        camera.begin_frame(3U);
        const auto one = *camera.active_event_camera_pose();
        require_near(one.eye.x, 107.5625F, 0.0001F,
                     "CKAN type-zero polynomial must remain correct at a second sample");
        require_near(one.watch.x, 114.375F, 0.0001F,
                     "CKAN nonzero type-seven polynomial must remain correct at a second sample");
    }

    void test_ckan_duplicate_keys_select_last_equal_time() {
        const auto check_track = [](const AnimationTrack& track,
                                    const std::array<float, 4U>& expected) {
            const auto animation = smgpc::camera::CameraAnimation::from_bytes(make_ckan_fovy_track(track));
            const auto target = smgpc::camera::StageCameraTargetState{};
            auto camera = smgpc::camera::OriginalAnimationCamera(animation, target, 1.0F);
            for (const auto fovy : expected) {
                require_near(camera.calc(target).fovy_degrees, fovy, 0.0001F,
                             "the original CKAN upper-bound search must select the last key at an equal time");
            }
        };
        check_track(AnimationTrack{3U, 0U, {
                        0.0F, 10.0F, 0.0F,
                        0.0F, 40.0F, 0.0F,
                        4.0F, 80.0F, 0.0F}},
                    {40.0F, 46.25F, 60.0F, 73.75F});
        check_track(AnimationTrack{4U, 0U, {
                        0.0F, 40.0F, 0.0F,
                        2.0F, 50.0F, 0.0F,
                        2.0F, 70.0F, 0.0F,
                        4.0F, 90.0F, 0.0F}},
                    {40.0F, 45.0F, 70.0F, 80.0F});
    }

    void test_ckan_search_count_and_lookahead_validation() {
        const auto target = smgpc::camera::StageCameraTargetState{};
        const auto animation = smgpc::camera::CameraAnimation::from_bytes(make_ckan_fovy_track(
            AnimationTrack{2U, 0U, {
                0.0F, 40.0F, 0.0F,
                2.0F, 60.0F, 0.0F,
                4.0F, 80.0F, 0.0F}}));
        const auto native = animation.native_data();
        const auto* info = reinterpret_cast<const CanmKeyFrameInfo*>(
            native.bytes().data() + sizeof(CanmFileHeader));
        require(info->mFovy.mCount == 2U,
                "native decoding must preserve the declared search count when a readable following record exists");
        auto camera = smgpc::camera::OriginalAnimationCamera(animation, target, 1.0F);
        for (const auto expected : std::array{40.0F, 50.0F, 60.0F, 70.0F}) {
            require_near(camera.calc(target).fovy_degrees, expected, 0.0001F,
                         "the original accessor must interpolate through its physical lookahead beyond the search count");
        }
        require_near(camera.calc(target).fovy_degrees, 70.0F, 0.0001F,
                     "the terminal FOV lookup must remain valid through the same lookahead record");
        require(camera.current_frame() == 4.0F,
                "a lookahead record must not extend the animation's declared playback duration");

        const auto require_rejected = [](const AnimationTrack& track, std::string_view reason) {
            auto rejected = false;
            try {
                (void)smgpc::camera::CameraAnimation::from_bytes(make_ckan_fovy_track(track));
            } catch (const std::runtime_error&) {
                rejected = true;
            }
            require(rejected, reason);
        };
        // FOV is the final track in the global table: no other component can
        // accidentally supply a physically readable following record.
        require_rejected(AnimationTrack{2U, 0U, {
                             0.0F, 40.0F, 0.0F,
                             2.0F, 60.0F, 0.0F}},
                         "reachable final searched keys require a physically readable following record");
        require_rejected(AnimationTrack{2U, 0U, {
                             1.0F, 40.0F, 0.0F,
                             4.0F, 80.0F, 0.0F}},
                         "a first key after frame zero must be rejected before the original unsigned search underflows");
    }

    void test_native_animation_metadata_and_alignment() {
        auto retained = smgpc::camera::NativeCameraAnimationData{};
        constexpr auto native_value_offset = std::size_t{0x20U + 0x40U + 16U};
        {
            auto bytes = make_four_frame_canm();
            bytes.insert(bytes.begin() + 0x60U, 16U, 0xA5U);
            write_be32(bytes, 0x08U, 0x01020304U);
            write_be32(bytes, 0x0cU, static_cast<std::uint32_t>(-77));
            write_be32(bytes, 0x10U, 0x0A0B0C0DU);
            write_be32(bytes, 0x14U, 0x10203040U);
            write_be32(bytes, 0x1cU, 0x50U);
            bytes.push_back(0x5AU);
            bytes.push_back(0x6BU);
            const auto animation = smgpc::camera::CameraAnimation::from_bytes(bytes);
            retained = animation.native_data();
            std::ranges::fill(bytes, 0U);
        }
        const auto bytes = retained.bytes();
        require(reinterpret_cast<std::uintptr_t>(bytes.data()) % alignof(CanmFileHeader) == 0U,
                "native animation storage must satisfy original header and component alignment");
        const auto* header = reinterpret_cast<const CanmFileHeader*>(bytes.data());
        require(header->_8 == 0x01020304 && header->_C == -77 &&
                    header->_10 == 0x0A0B0C0D && header->_14 == 0x10203040 &&
                    header->mNrFrames == 4U && header->mValueOffset == 0x50U,
                "native animation ownership must preserve all decoded metadata after source destruction");
        const auto* info = reinterpret_cast<const CanmFrameInfo*>(bytes.data() + sizeof(CanmFileHeader));
        require(info->mPosX.mCount == 4U && info->mPosX.mOffset == 0U &&
                    info->mFovy.mCount == 4U && info->mFovy.mOffset == 16U,
                "native components must retain counts and offsets without rewriting tracks");
        require(*reinterpret_cast<const std::uint32_t*>(bytes.data() + native_value_offset) == 80U,
                "the relocated value byte count must be stored in host byte order");
        const auto* values = reinterpret_cast<const float*>(bytes.data() + native_value_offset + 4U);
        require(values[1U] == 10.0F && values[19U] == 75.0F &&
                    bytes[0x60U] == 0xA5U && bytes[bytes.size() - 2U] == 0x5AU && bytes.back() == 0x6BU,
                "native decoding must retain float values, table padding, and trailing resource bytes");
    }

    void test_animation_safe_pose_uses_copied_manager_seed() {
        const auto bytes = make_animation_tracks(false, 1U, {
            AnimationTrack{1U, 0U, {0.0F}}, AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}}, AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {0.0F}}, AnimationTrack{1U, 0U, {0.0F}},
            AnimationTrack{1U, 0U, {30.0F}}, AnimationTrack{1U, 0U, {47.0F}},
        });
        const auto animation = smgpc::camera::CameraAnimation::from_bytes(bytes);
        auto seed = CameraPoseParam{};
        seed.mPos.set(10.0F, 20.0F, 30.0F);
        seed.mWatchPos.set(410.0F, 20.0F, 30.0F);
        seed.mUpVec.set(0.0F, 0.0F, 1.0F);
        seed.mRoll = 0.4F;
        seed.mFovy = 52.0F;
        auto target = smgpc::camera::StageCameraTargetState{};
        target.position = {1000.0F, 2000.0F, 3000.0F};
        target.up = {1.0F, 0.0F, 0.0F};
        auto camera = smgpc::camera::OriginalAnimationCamera(animation, target, 1.0F, &seed);
        require(camera.pose_param().mRoll == 0.4F && camera.pose_param().mUpVec.z == 1.0F,
                "the manager seed must retain unrolled up and roll separately before animation calculation");
        seed.mWatchPos.set(10.0F, 20.0F, -470.0F);
        seed.mUpVec.set(0.0F, 1.0F, 0.0F);
        const auto pose = camera.calc(target);
        require(pose.eye.x == 1000.0F && pose.eye.y == 2000.0F && pose.eye.z == 3000.0F &&
                    pose.watch.x == 1400.0F && pose.watch.y == 2000.0F && pose.watch.z == 3000.0F,
                "coincident authored eye/watch must use the copied prior manager view direction");
        require_near(pose.up.y, 0.5F, 0.0002F,
                     "parallel target up must fall back to prior unrolled manager up before applying animation roll");
        require_near(pose.up.z, 0.86602540F, 0.0002F,
                     "the fallback must not apply the prior manager roll a second time");
        require_near(pose.fovy_degrees, 47.0F, 0.0001F,
                     "the first animation FOV must replace the seeded manager FOV");
    }

    void test_animation_terminal_pose_and_restart() {
        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player, false);
        actor.camera_state.position = {100.0F, 200.0F, 300.0F};
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto animation = smgpc::camera::CameraAnimation::from_bytes(make_four_frame_canm());
        camera.declare_event_camera_animation(0, "terminal CANM", animation);
        camera.declare_event_camera_animation(0, "restart CANM", animation);
        camera.start_event_camera(0, "terminal CANM",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 2.0F);
        camera.begin_frame(1U);
        camera.begin_frame(2U);
        const auto last_sample = *camera.active_event_camera_pose();
        require_near(last_sample.eye.x, 130.0F, 0.001F,
                     "speed two must leave the last sampled position at frame two");
        require_near(last_sample.fovy_degrees, 65.0F, 0.0001F,
                     "completion must initially retain the last sampled FOV");
        require(camera.event_camera_animation_frame(0, "terminal CANM") == 4 &&
                    camera.is_event_camera_animation_end(0, "terminal CANM"),
                "the original cursor must report completion after its last valid sample");

        actor.camera_state.position = {2000.0F, 3000.0F, 4000.0F};
        actor.camera_state.side = smgpc::camera::CameraParamVec3{0.0F, 1.0F, 0.0F};
        actor.camera_state.up = {0.0F, 0.0F, 1.0F};
        actor.camera_state.front = {1.0F, 0.0F, 0.0F};
        camera.begin_frame(3U);
        const auto terminal = *camera.active_event_camera_pose();
        require(terminal.eye.x == last_sample.eye.x && terminal.eye.y == last_sample.eye.y &&
                    terminal.eye.z == last_sample.eye.z && terminal.watch.x == last_sample.watch.x &&
                    terminal.watch.y == last_sample.watch.y && terminal.watch.z == last_sample.watch.z,
                "completed CameraAnim must retain eye and watch despite target translation and rotation");
        require_near(terminal.fovy_degrees, 75.0F, 0.0001F,
                     "the terminal branch must apply the final authored FOV after a skipped final position sample");
        require_near(terminal.up.x, -0.86602540F, 0.0002F,
                     "the terminal branch must apply final twist to the retained camera up vector");
        require_near(terminal.up.y, 0.5F, 0.0002F,
                     "terminal twist must not use the newly rotated target up vector");
        camera.begin_frame(4U);
        require(same_pose(*camera.active_event_camera_pose(), terminal) &&
                    camera.event_camera_animation_frame(0, "terminal CANM") == 4,
                "completed animation poses and cursors must stop advancing");

        actor.camera_state.side = smgpc::camera::CameraParamVec3{1.0F, 0.0F, 0.0F};
        actor.camera_state.up = {0.0F, 1.0F, 0.0F};
        actor.camera_state.front = {0.0F, 0.0F, 1.0F};
        camera.start_event_camera(0, "restart CANM",
            smgpc::camera::EventCameraTarget::retain(), 0, 1.0F);
        require(same_pose(*camera.active_event_camera_pose(), terminal),
                "a new animation chunk must preserve the prior visible pose during its request");
        camera.begin_frame(5U);
        const auto restarted = *camera.active_event_camera_pose();
        require_near(restarted.eye.x, 2000.0F, 0.001F,
                     "a different animation chunk must reset to frame zero and the current target");
        require_near(restarted.fovy_degrees, 45.0F, 0.0001F,
                     "a new animation chunk must restore its first FOV sample");
        require(camera.event_camera_animation_frame(0, "restart CANM") == 1 &&
                    !camera.is_event_camera_animation_end(0, "restart CANM"),
                "a new animation chunk must own a reset original cursor");
    }

    void test_animation_pause_and_resource_ownership() {
        auto target = smgpc::camera::StageCameraTargetState{};
        auto camera = std::unique_ptr<smgpc::camera::OriginalAnimationCamera>{};
        {
            auto bytes = make_four_frame_canm();
            const auto animation = smgpc::camera::CameraAnimation::from_bytes(bytes);
            camera = std::make_unique<smgpc::camera::OriginalAnimationCamera>(animation, target, 1.0F);
            std::ranges::fill(bytes, 0xEDU);
        }
        require(camera->current_frame() == 0.0F && !camera->is_end(),
                "constructing the original animation owner must not sample or advance the camera");
        const auto first = camera->calc(target);
        require_near(first.eye.z, 500.0F, 0.0001F,
                     "the original controller must retain decoded resource storage after its source owners are destroyed");
        require(camera->current_frame() == 1.0F,
                "the first original calc must advance the cursor after sampling");

        camera->set_paused(true);
        target.position.x = 200.0F;
        const auto paused_first = camera->calc(target);
        require_near(paused_first.eye.x, 210.0F, 0.0001F,
                     "animation-only pause must still calculate its current sample against the moving target");
        require(camera->current_frame() == 1.0F,
                "animation-only pause must hold the original cursor");
        target.position.x = 300.0F;
        const auto paused_second = camera->calc(target);
        require_near(paused_second.eye.x, 310.0F, 0.0001F,
                     "repeated paused calculations must keep following target motion at the same animation sample");
        require(camera->current_frame() == 1.0F,
                "repeated paused calculations must not advance animation time");
        camera->set_paused(false);
        const auto resumed = camera->calc(target);
        require(same_pose(resumed, paused_second) && camera->current_frame() == 2.0F,
                "unpausing must resample the held frame and then resume cursor advancement");
        const auto next = camera->calc(target);
        require_near(next.eye.x, 330.0F, 0.0001F,
                     "the next unpaused calculation must use the following authored sample");
    }

    void test_same_animation_request_preserves_cursor() {
        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player, false);
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(0, "same animation",
            smgpc::camera::CameraAnimation::from_bytes(make_four_frame_canm()));
        camera.start_event_camera(0, "same animation",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 1.0F);
        camera.begin_frame(1U);
        const auto before = *camera.active_event_camera_pose();
        camera.start_event_camera(0, "same animation",
            smgpc::camera::EventCameraTarget::retain(), 30, 0.5F);
        require(same_pose(*camera.active_event_camera_pose(), before) &&
                    camera.event_camera_animation_frame(0, "same animation") == 1,
                "same-chunk ANIM requests must preserve the original controller cursor and visible pose");
        camera.begin_frame(2U);
        require_near(camera.active_event_camera_pose()->eye.x, 10.0F, 0.0001F,
                     "same-chunk re-request must continue at the retained frame instead of restarting");
        camera.begin_frame(3U);
        require_near(camera.active_event_camera_pose()->eye.x, 20.0F, 0.0001F,
                     "same-chunk re-request must apply its updated playback speed without a reset");
        require(camera.event_camera_animation_frame(0, "same animation") == 2,
                "the reused controller must preserve the original frame plus its new playback increments");
    }

    void test_failed_start_is_transactional() {
        const auto animation = smgpc::camera::CameraAnimation::from_bytes(
            make_linear_canm());
        auto player = smgpc::runtime::PlayerSystemService{};
        auto player_actor = CameraPlayerFixture(player);
        set_player_matrix(player, 100.0F, 0.0F, 0.0F);
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(5, "prior", animation);
        camera.declare_event_camera_animation(5, "failed", animation);
        camera.declare_event_camera_animation(5, "retained", animation);
        camera.start_event_camera(
            5, "prior",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 1.0F);
        camera.begin_frame(0U);
        const auto prior_key = camera.active_event_camera_key();
        const auto prior_pose = camera.active_event_camera_pose();
        require(prior_key.has_value() && prior_pose.has_value(),
                "transactional start proof requires a valid prior event camera");

        auto actor = std::make_unique<LiveActor>("failed camera target");
        actor->appear();
        const auto failed_target =
            smgpc::camera::EventCameraTarget::target_actor(*actor);
        actor.reset();
        auto rejected = false;
        try {
            camera.start_event_camera(5, "failed", failed_target, 0, 1.0F);
        } catch (const std::logic_error &) {
            rejected = true;
        }
        const auto after_key = camera.active_event_camera_key();
        const auto after_pose = camera.active_event_camera_pose();
        require(rejected && after_key == prior_key && after_pose.has_value() &&
                    same_pose(*after_pose, *prior_pose),
                "a failed explicit-target start must preserve the prior active key and pose");

        set_player_matrix(player, 300.0F, 0.0F, 0.0F);
        camera.start_event_camera(
            5, "retained", smgpc::camera::EventCameraTarget::retain(), 0,
            1.0F);
        require(same_pose(*camera.active_event_camera_pose(), *prior_pose),
                "a valid replacement request must preserve the prior visible pose until camera movement");
        camera.begin_frame(1U);
        const auto retained_key = camera.active_event_camera_key();
        const auto retained_pose = camera.active_event_camera_pose();
        require(retained_key.has_value() && retained_pose.has_value() &&
                    retained_key->name == "retained" &&
                    retained_pose->eye.x == 300.0F,
                "a failed explicit-target start must not replace the retained target");
    }

    void test_original_matrix_target_movement_and_gravity() {
        auto scene = CameraTargetScene{};
        auto target = CameraTargetMtx("original matrix movement");
        Mtx matrix{{2.0F, 0.0F, 0.0F, 3.0F},
                   {0.0F, 3.0F, 0.0F, 4.0F},
                   {0.0F, 0.0F, 4.0F, 0.0F}};
        target.setMtx(matrix);
        target.movement();
        require_vector_near(target.getPosition(), {3.0F, 4.0F, 0.0F}, 0.0F,
                            "matrix target movement must publish the matrix translation");
        require_vector_near(target.getLastMove(), {3.0F, 4.0F, 0.0F}, 0.0F,
                            "the original matrix target must derive its first delta from its initial zero position");
        require_vector_near(target.getSideVec(), {2.0F, 0.0F, 0.0F}, 0.0F,
                            "matrix target side must preserve the raw matrix column magnitude");
        require_vector_near(target.getUpVec(), {0.0F, 3.0F, 0.0F}, 0.0F,
                            "matrix target up must preserve the raw matrix column magnitude");
        require_vector_near(target.getFrontVec(), {0.0F, 0.0F, 4.0F}, 0.0F,
                            "matrix target front must preserve the raw matrix column magnitude");
        require_vector_near(target.getGravityVector(), {}, 0.0F,
                            "a present empty gravity registry must publish zero gravity");
        require(&target.getGroundPos() == &target.getPosition() && target.getCubeCameraArea() == nullptr,
                "the matrix target must expose its actual position as ground and query the empty CubeCamera manager");

        auto gravity = PointGravity{};
        MR::registerGravity(&gravity);
        target.mMatrix.setTrans(TVec3f{6.0F, 8.0F, 0.0F});
        target.movement();
        require_vector_near(target.getLastMove(), {3.0F, 4.0F, 0.0F}, 0.0F,
                            "matrix target deltas must use consecutive published translations");
        require_vector_near(target.getGravityVector(), {-0.6F, -0.8F, 0.0F}, 0.0001F,
                            "matrix target gravity must come from the scene's actual point field at the latest position");
        target.invalidateLastMove();
        target.mMatrix.setTrans(TVec3f{9.0F, 12.0F, 0.0F});
        target.movement();
        require_vector_near(target.getLastMove(), {}, 0.0F,
                            "invalidateLastMove must suppress exactly the next matrix delta");
        target.mMatrix.setTrans(TVec3f{12.0F, 16.0F, 0.0F});
        target.movement();
        require_vector_near(target.getLastMove(), {3.0F, 4.0F, 0.0F}, 0.0F,
                            "movement after invalidation must start from the suppressed phase's published position");
        gravity.mActivated = false;
        target.movement();
        require_vector_near(target.getGravityVector(), {}, 0.0F,
                            "deactivating the real gravity field must clear the matrix target's prior gravity");
    }

    void test_matrix_and_object_target_camera_phases() {
        auto scene = CameraTargetScene{};
        auto target = CountedMatrixTarget{};
        Mtx matrix{{2.0F, 0.0F, 0.0F, 15.0F},
                   {0.0F, 3.0F, 0.0F, 0.0F},
                   {0.0F, 0.0F, 4.0F, 0.0F}};
        target.setMtx(matrix);
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto animation = make_target_basis_animation();
        camera.declare_event_camera_animation(5, "original matrix phases", animation);
        camera.start_event_camera(5, "original matrix phases",
            smgpc::camera::EventCameraTarget::target_matrix(target), 30);
        require(target.movement_count == 0U && !camera.active_event_camera_pose().has_value(),
                "a matrix event request must bind its target without moving it or calculating a camera");
        camera.begin_frame(1U);
        require(target.movement_count == 1U, "a camera phase must move the actual matrix target once");
        require_vector_near(target.getLastMove(), {}, 0.0F,
                            "an explicit matrix argument must invalidate the first published delta");
        const auto first = *camera.active_event_camera_pose();
        require_near(first.eye.x, 35.0F, 0.0001F,
                     "CameraAnim must receive the matrix translation and unnormalized side before calculation");
        require_near(first.eye.y, 60.0F, 0.0001F,
                     "CameraAnim must receive the original matrix target's raw up column");
        require_near(first.eye.z, 400.0F, 0.0001F,
                     "CameraAnim must receive the original matrix target's raw front column");

        target.mMatrix.setTrans(TVec3f{30.0F, 0.0F, 0.0F});
        camera.begin_frame(1U);
        require(target.movement_count == 1U && same_pose(*camera.active_event_camera_pose(), first),
                "repeating a camera phase index must skip target movement and controller calculation");
        camera.begin_frame(2U);
        require(target.movement_count == 2U, "a new camera phase must advance its matrix target once");
        require_vector_near(target.getLastMove(), {}, 0.0F,
                            "the first changed-chunk checkReset rebind must invalidate the second matrix delta");
        target.mMatrix.setTrans(TVec3f{45.0F, 0.0F, 0.0F});
        camera.begin_frame(3U);
        require_vector_near(target.getLastMove(), {15.0F, 0.0F, 0.0F}, 0.0F,
                            "matrix target motion must resume after the two original argument bindings");

        const auto before_pause = *camera.active_event_camera_pose();
        camera.pause_on_camera_director();
        target.mMatrix.setTrans(TVec3f{90.0F, 0.0F, 0.0F});
        camera.begin_frame(4U);
        require(target.movement_count == 3U && same_pose(*camera.active_event_camera_pose(), before_pause),
                "director pause must skip actual matrix movement and preserve the camera pose");
        camera.pause_off_camera_director();
        camera.begin_frame(4U);
        require(target.movement_count == 4U,
                "a skipped paused phase must remain available when the director resumes");
        require_vector_near(target.getLastMove(), {45.0F, 0.0F, 0.0F}, 0.0F,
                            "resuming must measure all translation since the last actual target movement");

        camera.start_event_camera(5, "original matrix phases",
            smgpc::camera::EventCameraTarget::retain(), 30);
        target.mMatrix.setTrans(TVec3f{105.0F, 0.0F, 0.0F});
        camera.begin_frame(5U);
        require_vector_near(target.getLastMove(), {15.0F, 0.0F, 0.0F}, 0.0F,
                            "a no-argument retained target request must not invalidate a matrix delta");
        camera.start_event_camera(5, "original matrix phases",
            smgpc::camera::EventCameraTarget::target_matrix(target), 30);
        target.mMatrix.setTrans(TVec3f{120.0F, 0.0F, 0.0F});
        camera.begin_frame(6U);
        require_vector_near(target.getLastMove(), {}, 0.0F,
                            "an explicit same-chunk matrix argument must invalidate its next delta once");
        target.mMatrix.setTrans(TVec3f{135.0F, 0.0F, 0.0F});
        camera.begin_frame(7U);
        require_vector_near(target.getLastMove(), {15.0F, 0.0F, 0.0F}, 0.0F,
                            "an unchanged chunk must not perform the second matrix invalidation");

        auto object_target = CountedMatrixTarget{};
        object_target.setMtx(matrix);
        camera.declare_event_camera_animation(5, "original object phases", animation);
        camera.start_event_camera(5, "original object phases",
            smgpc::camera::EventCameraTarget::target_object(object_target), 30);
        require(object_target.movement_count == 0U,
                "a generic original target object must also wait for camera movement");
        camera.begin_frame(8U);
        require(object_target.movement_count == 1U,
                "the generic target-object path must execute the object's original virtual movement");
        require_vector_near(object_target.getLastMove(), {15.0F, 0.0F, 0.0F}, 0.0F,
                            "a generic object argument must not inherit matrix-argument invalidation semantics");
        object_target.mMatrix.setTrans(TVec3f{30.0F, 0.0F, 0.0F});
        camera.begin_frame(9U);
        require_vector_near(object_target.getLastMove(), {15.0F, 0.0F, 0.0F}, 0.0F,
                            "a changed-chunk generic object rebind must leave the next delta intact");
        camera.end_event_camera(5, "original object phases", true, -1);

        auto pending_target = CountedMatrixTarget{};
        pending_target.setMtx(matrix);
        camera.start_event_camera(5, "original matrix phases",
            smgpc::camera::EventCameraTarget::target_matrix(pending_target), 30);
        camera.start_event_camera(5, "original matrix phases",
            smgpc::camera::EventCameraTarget::target_object(pending_target), 30);
        require(pending_target.movement_count == 0U,
                "switching a pending matrix argument to an object argument must not advance the target");
        camera.begin_frame(10U);
        require_vector_near(pending_target.getLastMove(), {}, 0.0F,
                            "the earlier explicit matrix request still invalidates the pending object's first delta");
        pending_target.mMatrix.setTrans(TVec3f{30.0F, 0.0F, 0.0F});
        camera.begin_frame(11U);
        require_vector_near(pending_target.getLastMove(), {15.0F, 0.0F, 0.0F}, 0.0F,
                            "a pending switch to a generic object must clear the obsolete second matrix invalidation");

        const auto preceding_pose = *camera.active_event_camera_pose();
        require(camera.event_camera_animation_frame(5, "original matrix phases") == 2,
                "the A-to-B-to-A fixture must begin with a calculated animation cursor");
        camera.start_event_camera(5, "original object phases",
            smgpc::camera::EventCameraTarget::target_matrix(pending_target), 30);
        camera.start_event_camera(5, "original matrix phases",
            smgpc::camera::EventCameraTarget::target_matrix(pending_target), 30);
        require(pending_target.movement_count == 2U &&
                    same_pose(*camera.active_event_camera_pose(), preceding_pose) &&
                    camera.event_camera_animation_frame(5, "original matrix phases") == 2,
                "requesting B then calculated A before movement must retain A's visible pose and original animation cursor");
        pending_target.mMatrix.setTrans(TVec3f{45.0F, 0.0F, 0.0F});
        camera.begin_frame(12U);
        require(camera.event_camera_animation_frame(5, "original matrix phases") == 3,
                "returning to calculated A before the camera phase must continue its existing controller");
        require_vector_near(pending_target.getLastMove(), {}, 0.0F,
                            "the final explicit matrix request must invalidate the next movement once");
        pending_target.mMatrix.setTrans(TVec3f{60.0F, 0.0F, 0.0F});
        camera.begin_frame(13U);
        require_vector_near(pending_target.getLastMove(), {15.0F, 0.0F, 0.0F}, 0.0F,
                            "A-to-B-to-A before movement must not perform a changed-chunk second matrix invalidation");
        require(camera.event_camera_animation_frame(5, "original matrix phases") == 4,
                "the retained A animation cursor must continue on subsequent phases");
        camera.start_event_camera(5, "original object phases",
            smgpc::camera::EventCameraTarget::target_object(pending_target), 30);
        camera.end_event_camera(5, "original object phases", true, -1);
        require(!camera.active_event_camera_key().has_value() &&
                    !camera.active_event_camera_pose().has_value(),
                "cancelling the pending chunk must clear both entries in its original event priority slot");
    }

    void test_original_actor_target_axes_and_state() {
        auto scene = CameraTargetScene{};
        auto actor = ActorBaseMatrixFixture{};
        actor.mPosition.set(100.0F, 200.0F, 300.0F);
        actor.mVelocity.set(7.0F, -8.0F, 9.0F);
        actor.mGravity.set(0.0F, -1.0F, 0.0F);
        auto target = GravityInfoCameraTarget{};
        target.mActor = &actor;
        target.movement();
        require(actor.base_matrix_reads > 0U,
                "the original actor target must dispatch the actor's virtual getBaseMtx");
        require_vector_near(target.getSideVec(), {0.0F, 0.0F, -2.0F}, 0.0F,
                            "actor target side must retain the virtual matrix's raw column");
        require_vector_near(target.getUpVec(), {0.0F, 3.0F, 0.0F}, 0.0F,
                            "actor target up must retain the virtual matrix's raw column");
        require_vector_near(target.getFrontVec(), {4.0F, 0.0F, 0.0F}, 0.0F,
                            "actor target front must retain the virtual matrix's raw column");
        require(&target.getPosition() == &actor.mPosition && &target.getGroundPos() == &actor.mPosition &&
                    &target.getLastMove() == &actor.mVelocity,
                "actor target position, ground, and last move must expose the original actor state");
        require_vector_near(target.getGravityVector(), {0.0F, 3.0F, 0.0F}, 0.0F,
                            "an actor target without GravityInfo must return its raw up vector");
        target.info.mGravityVector.set(6.0F, -7.0F, 8.0F);
        target.has_info = true;
        require_vector_near(target.getGravityVector(), {6.0F, -7.0F, 8.0F}, 0.0F,
                            "actor target gravity must honor the original virtual GravityInfo without normalizing it");

        const auto preceding_reads = actor.base_matrix_reads;
        actor.mFlag.mIsClipped = true;
        actor.has_base_matrix = false;
        actor.mRotation.set(30.0F, 45.0F, 0.0F);
        actor.mPosition.set(110.0F, 220.0F, 330.0F);
        target.movement();
        require(actor.base_matrix_reads == preceding_reads,
                "clipping must skip original actor-target orientation calculation");
        require_vector_near(target.getFrontVec(), {4.0F, 0.0F, 0.0F}, 0.0F,
                            "a clipped actor target must retain its preceding orientation");
        require_vector_near(target.getPosition(), {110.0F, 220.0F, 330.0F}, 0.0F,
                            "clipping must not replace the actor target's direct position getter with a cached snapshot");
        actor.mFlag.mIsClipped = false;
        actor.mFlag.mIsDead = true;
        target.movement();
        require(actor.base_matrix_reads == preceding_reads,
                "a dead but retained actor owner must skip target orientation movement");
        require_vector_near(target.getFrontVec(), {4.0F, 0.0F, 0.0F}, 0.0F,
                            "dead actor-target movement must preserve the preceding raw orientation");

        actor.mFlag.mIsDead = false;
        target.movement();
        // R_y(45) R_x(30), allowing the original signed-angle table's grid.
        require_vector_near(target.getSideVec(), {0.70710678F, 0.0F, -0.70710678F}, 0.0003F,
                            "an actor without a base matrix must derive side from its original Euler rotation");
        require_vector_near(target.getUpVec(), {0.35355339F, 0.86602540F, 0.35355339F}, 0.0003F,
                            "the Euler fallback must combine actor X and Y rotation in the original order");
        require_vector_near(target.getFrontVec(), {0.61237244F, -0.5F, 0.61237244F}, 0.0003F,
                            "the combined Euler front must include cosine X times sine Y");
    }

    void test_actor_target_camera_phases_and_retained_orientation() {
        auto scene = CameraTargetScene{};
        auto actor = ActorBaseMatrixFixture{};
        actor.mPosition.set(100.0F, 200.0F, 300.0F);
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(5, "original actor phases", make_target_basis_animation());
        camera.start_event_camera(5, "original actor phases",
            smgpc::camera::EventCameraTarget::target_actor(actor), 30);
        require(actor.base_matrix_reads == 0U,
                "an actor event request must defer original orientation movement to the camera phase");
        camera.begin_frame(1U);
        const auto first = *camera.active_event_camera_pose();
        require_near(first.eye.x, 500.0F, 0.0001F,
                     "an actor event must use the actor's virtual raw front and actual position");
        require_near(first.eye.y, 260.0F, 0.0001F,
                     "an actor event must preserve raw up magnitude through actual CameraAnim calculation");
        require_near(first.eye.z, 280.0F, 0.0001F,
                     "an actor event must use the virtual side instead of a native registry matrix");
        const auto first_reads = actor.base_matrix_reads;
        require(first_reads > 0U, "the first actor camera phase must have executed original target movement");
        actor.mPosition.set(110.0F, 220.0F, 330.0F);
        camera.begin_frame(1U);
        require(actor.base_matrix_reads == first_reads && same_pose(*camera.active_event_camera_pose(), first),
                "a repeated camera frame must not resample live actor position or orientation");
        actor.mFlag.mIsClipped = true;
        actor.has_base_matrix = false;
        camera.begin_frame(2U);
        require(actor.base_matrix_reads == first_reads,
                "a clipped event actor must use the original target's cached orientation");
        require_near(camera.active_event_camera_pose()->eye.x, 510.0F, 0.0001F,
                     "a clipped actor event must combine its current position with its retained orientation");
        actor.mFlag.mIsClipped = false;
        actor.mFlag.mIsDead = true;
        actor.mPosition.set(120.0F, 240.0F, 360.0F);
        camera.begin_frame(3U);
        require(actor.base_matrix_reads == first_reads,
                "a dead retained event actor must preserve the original target movement skip");
        require_near(camera.active_event_camera_pose()->eye.x, 520.0F, 0.0001F,
                     "a retained dead actor is still a live owner whose original position getter is readable");

        auto next_actor = ActorBaseMatrixFixture{};
        next_actor.mPosition.set(200.0F, 400.0F, 600.0F);
        next_actor.mFlag.mIsClipped = true;
        next_actor.has_base_matrix = false;
        camera.start_event_camera(5, "original actor phases",
            smgpc::camera::EventCameraTarget::target_actor(next_actor), 30);
        camera.begin_frame(4U);
        require(next_actor.base_matrix_reads == 0U,
                "changing the actor owner must still honor the replacement actor's clipping state");
        require_near(camera.active_event_camera_pose()->eye.x, 600.0F, 0.0001F,
                     "the reused original actor target must retain cached orientation across a clipped owner change");
        require_near(camera.active_event_camera_pose()->eye.z, 580.0F, 0.0001F,
                     "reusing the actor target must preserve its prior side vector as well as its front");
        camera.end_event_camera(5, "original actor phases", true, -1);
    }

    void test_matrix_target_lifetime_and_aba() {
        const auto animation = smgpc::camera::CameraAnimation::from_bytes(
            make_linear_canm());
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.declare_event_camera_animation(5, "matrix lifetime", animation);
        auto matrix = std::make_unique<CameraTargetMtx>("matrix target");
        camera.start_event_camera(
            5, "matrix lifetime",
            smgpc::camera::EventCameraTarget::target_matrix(*matrix), 0,
            1.0F);
        matrix.reset();
        auto rejected_destroyed_matrix = false;
        try {
            camera.begin_frame(3U);
        } catch (const std::logic_error &) {
            rejected_destroyed_matrix = true;
        }
        require(rejected_destroyed_matrix,
                "event camera must reject a retired CameraTargetMtx before dereference");

        auto aba_camera = smgpc::runtime::CameraSystemService{};
        aba_camera.declare_event_camera_animation(5, "matrix ABA", animation);
        alignas(CameraTargetMtx) std::byte storage[sizeof(CameraTargetMtx)]{};
        auto *first = std::construct_at(
            reinterpret_cast<CameraTargetMtx *>(storage), "first matrix");
        const auto captured =
            smgpc::camera::EventCameraTarget::target_matrix(*first);
        aba_camera.start_event_camera(5, "matrix ABA", captured, 0, 1.0F);
        std::destroy_at(first);
        auto *replacement = std::construct_at(
            reinterpret_cast<CameraTargetMtx *>(storage),
            "replacement matrix");
        const auto replacement_target =
            smgpc::camera::EventCameraTarget::target_matrix(*replacement);
        auto rejected_reused_address = false;
        try {
            aba_camera.begin_frame(4U);
        } catch (const std::logic_error &) {
            rejected_reused_address = true;
        }
        std::destroy_at(replacement);
        require(captured.matrix == replacement_target.matrix &&
                    captured.name_obj_generation !=
                        replacement_target.name_obj_generation &&
                    rejected_reused_address,
                "event-camera target generations must reject same-address NameObj reuse");
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto result = std::string(value);
        std::ranges::transform(result, result.begin(), [](char character) {
            return static_cast<char>(std::tolower(
                static_cast<unsigned char>(character)));
        });
        return result;
    }

    void test_same_xz_request_preserves_original_state(
        const smgpc::camera::EventCameraCatalog &retail_catalog,
        smgpc::runtime::DvdFileSystemService &dvd) {
        // Control the temporal parameters in a private copy of a valid loaded
        // catalog. The original catalog and its retail resource remain intact.
        auto catalog = retail_catalog;
        auto *definition = const_cast<smgpc::camera::StaticEventCameraDefinition *>(
            catalog.find(0, "土管固有出現054"));
        require(definition != nullptr, "the controlled XZ fixture needs a catalog entry");
        definition->zone_transform = {};
        definition->camera_param = {};
        auto &param = definition->camera_param;
        param.camera_type = "CAM_TYPE_XZ_PARA";
        param.general.num1 = 1;
        param.general.dist = 600.0F;
        param.general.angle_b = 0.0F;
        param.extra.w_offset = {};
        param.extra.v_pan_use = 1;
        param.extra.upper = 1.0F;
        param.extra.lower = 1.0F;

        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(scheduler);
        auto demo = smgpc::compat::DemoSceneRuntime(dvd, {});
        auto &wpad = aurora::wpad_service();
        wpad.clear();
        struct WpadClearGuard {
            ~WpadClearGuard() { aurora::wpad_service().clear(); }
        } clear_guard;
        wpad.set_connected(WPAD_CHAN0, true);
        wpad.begin_frame();
        wpad.set_button_mask(WPAD_CHAN0, 0U);

        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player);
        const auto target = smgpc::camera::EventCameraTarget::target_player(player);
        auto requested = smgpc::runtime::CameraSystemService{};
        auto uninterrupted = smgpc::runtime::CameraSystemService{};
        requested.attach_event_camera_catalog(catalog);
        uninterrupted.attach_event_camera_catalog(catalog);
        requested.declare_event_camera(0, "土管固有出現054");
        uninterrupted.declare_event_camera(0, "土管固有出現054");
        requested.start_event_camera(0, "土管固有出現054", target, 0);
        uninterrupted.start_event_camera(0, "土管固有出現054", target, 0);
        requested.begin_frame(0U);
        uninterrupted.begin_frame(0U);
        const auto initial_pose = *requested.active_event_camera_pose();
        actor.camera_state.position.y = 100.0F;
        actor.camera_state.last_move.y = 100.0F;
        for (auto frame = 1U; frame <= 4U; ++frame) {
            wpad.begin_frame();
            wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_RIGHT);
            requested.begin_frame(frame);
            uninterrupted.begin_frame(frame);
        }
        const auto before = *requested.active_event_camera_pose();
        require(before.watch.y > 0.0F && before.watch.y < 100.0F &&
                    std::fabs((before.eye.z - before.watch.z) -
                              (initial_pose.eye.z - initial_pose.watch.z)) > 20.0F,
                "the fixture must have nontrivial original height chase and round state");
        requested.start_event_camera(0, "土管固有出現054", target, 30);
        require(same_pose(before, *requested.active_event_camera_pose()),
                "same-XZ re-request must preserve the current pose until movement");

        auto unavailable = smgpc::runtime::PlayerSystemService{};
        auto rejected = false;
        try {
            requested.start_event_camera(0, "土管固有出現054",
                smgpc::camera::EventCameraTarget::target_player(unavailable), 0);
        } catch (const std::logic_error &) {
            rejected = true;
        }
        require(rejected && same_pose(before, *requested.active_event_camera_pose()),
                "failed same-XZ request must preserve the controller and prior target");
        requested.start_event_camera(0, "土管固有出現054",
            smgpc::camera::EventCameraTarget::retain(), 0);
        for (auto frame = 5U; frame <= 8U; ++frame) {
            wpad.begin_frame();
            wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_RIGHT);
            requested.begin_frame(frame);
            uninterrupted.begin_frame(frame);
            require(same_pose(*requested.active_event_camera_pose(),
                              *uninterrupted.active_event_camera_pose()),
                    "re-requested XZ must follow the uninterrupted original round/height trajectory");
        }
    }

    void test_fixed_point_event_lifecycle(smgpc::camera::EventCameraCatalog catalog) {
        constexpr auto first_name = "逃げウサギ集め固有016";
        constexpr auto next_name = "土管固有出現017";
        for (const auto* name : {first_name, next_name}) {
            auto* definition = const_cast<smgpc::camera::StaticEventCameraDefinition*>(catalog.find(5, name));
            require(definition != nullptr, "the FixedPoint lifecycle fixture needs retained real catalog identities");
            definition->zone_transform = {};
            definition->camera_param = make_fixed_point_param(0);
            definition->camera_param.extra.l_offset = 100.0F;
            definition->camera_param.extra.l_offset_v = 60.0F;
        }
        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player, false);
        actor.camera_state.last_move = {15.0F, 0.0F, 0.0F};
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.attach_event_camera_catalog(catalog);
        camera.declare_event_camera(5, first_name);
        camera.declare_event_camera(5, next_name);
        camera.start_event_camera(5, first_name,
            smgpc::camera::EventCameraTarget::target_player(player), 30);
        require(!camera.active_event_camera_pose().has_value(),
                "a FixedPoint event request must remain deferred until the target's camera phase");
        camera.begin_frame(1U);
        const auto first = *camera.active_event_camera_pose();
        require_near(first.watch.z, 19.0F, 0.0001F,
                     "a FixedPoint event must run original reset and normal calc without pre-filling its local offset");
        camera.pause_on_camera_director();
        actor.camera_state.position.x = 100.0F;
        camera.begin_frame(2U);
        require(same_pose(*camera.active_event_camera_pose(), first),
                "director pause must preserve the FixedPoint controller pose and local offset");
        camera.pause_off_camera_director();
        camera.begin_frame(3U);
        const auto resumed = *camera.active_event_camera_pose();
        require_near(resumed.watch.x, 100.0F, 0.0001F,
                     "FixedPoint resume must consume the newest target position");
        require_near(resumed.watch.z, 27.1F, 0.0001F,
                     "FixedPoint resume must advance its retained local offset once");
        camera.start_event_camera(5, first_name, smgpc::camera::EventCameraTarget::retain(), 30);
        require(same_pose(*camera.active_event_camera_pose(), resumed),
                "same-chunk FixedPoint requests must preserve the visible pose until movement");
        camera.begin_frame(4U);
        const auto retained = *camera.active_event_camera_pose();
        require_near(retained.watch.z, 34.39F, 0.0002F,
                     "same-chunk FixedPoint requests must reuse the original controller without a reset calculation");
        camera.start_event_camera(5, next_name, smgpc::camera::EventCameraTarget::retain(), 30);
        require(same_pose(*camera.active_event_camera_pose(), retained),
                "a new FixedPoint chunk must preserve the prior manager's visible pose during the request");
        camera.begin_frame(5U);
        const auto restarted = *camera.active_event_camera_pose();
        require_near(restarted.watch.z, 46.8559F, 0.0003F,
                     "a different FixedPoint chunk must reset and calculate from the preceding original manager offset");
        require_near(restarted.watch.y, 28.11354F, 0.0003F,
                     "the new event manager must inherit and continue the prior vertical offset");
        camera.start_event_camera(5, next_name, smgpc::camera::EventCameraTarget::retain(), 0);
        require(same_pose(*camera.active_event_camera_pose(), restarted),
                "a same-chunk zero-frame request must preserve the current visible FixedPoint pose");
        camera.begin_frame(6U);
        require_near(camera.active_event_camera_pose()->watch.z, 52.17031F, 0.0003F,
                     "CameraManEvent checkReset must skip the zero-frame local-offset reset for an unchanged chunk");
        camera.end_event_camera(5, next_name, true, -1);
        require(!camera.active_event_camera_key().has_value(),
                "ending FixedPoint must release the active event camera ownership");
        camera.detach_event_camera_catalog(catalog);

        for (const auto* name : {first_name, next_name}) {
            auto* definition = const_cast<smgpc::camera::StaticEventCameraDefinition*>(catalog.find(5, name));
            definition->camera_param.extra.roll = 0.2F;
        }
        auto pending_player = smgpc::runtime::PlayerSystemService{};
        auto pending_actor = CameraPlayerFixture(pending_player, false);
        pending_actor.camera_state.last_move = {15.0F, 0.0F, 0.0F};
        auto pending_camera = smgpc::runtime::CameraSystemService{};
        pending_camera.attach_event_camera_catalog(catalog);
        pending_camera.declare_event_camera(5, first_name);
        pending_camera.declare_event_camera(5, next_name);
        pending_camera.start_event_camera(5, first_name,
            smgpc::camera::EventCameraTarget::target_player(pending_player), 30);
        pending_camera.begin_frame(1U);
        const auto preceding_pose = *pending_camera.active_event_camera_pose();
        require_near(preceding_pose.watch.z, 19.0F, 0.0001F,
                     "the pending-request fixture must begin with an accumulated original manager offset");
        pending_camera.start_event_camera(5, next_name, smgpc::camera::EventCameraTarget::retain(), 30);
        pending_camera.start_event_camera(5, next_name, smgpc::camera::EventCameraTarget::retain(), 30);
        require(same_pose(*pending_camera.active_event_camera_pose(), preceding_pose),
                "re-requesting a pending FixedPoint event must retain the preceding visible pose without calculation");
        pending_camera.begin_frame(2U);
        require_near(pending_camera.active_event_camera_pose()->watch.z, 34.39F, 0.0002F,
                     "re-requesting a pending event must preserve its raw manager seed and accumulated front offset");
        require_near(pending_camera.active_event_camera_pose()->watch.y, 20.634F, 0.0002F,
                     "a pending event must preserve the preceding raw vertical offset through reset and normal calc");
        pending_camera.end_event_camera(5, next_name, true, -1);
        pending_camera.detach_event_camera_catalog(catalog);
    }

    void test_fixed_point_event_interpolation_precedence(smgpc::camera::EventCameraCatalog catalog) {
        constexpr auto name = "逃げウサギ集め固有016";
        auto* definition = const_cast<smgpc::camera::StaticEventCameraDefinition*>(catalog.find(5, name));
        require(definition != nullptr, "the interpolation fixture needs a retained real catalog identity");
        definition->zone_transform = {};
        definition->camera_param = make_fixed_point_param(0);
        definition->camera_param.extra.l_offset = 100.0F;

        struct FrameCase {
            std::int32_t authored_enabled;
            std::int32_t authored_frames;
            std::int32_t requested_frames;
            float expected_offset;
            std::string_view failure;
        };
        // CameraManEvent::getInterpolateFrame chooses a nonnegative authored
        // override, then a nonnegative request, then 60. Only zero sets the
        // manager's local-offset reset flag before the new chunk's reset.
        constexpr auto cases = std::array{
            FrameCase{0, 30, 0, 100.0F, "a zero-frame request must reset offsets when the authored override is disabled"},
            FrameCase{1, 30, 0, 19.0F, "a positive authored interpolation override must win over a zero-frame request"},
            FrameCase{1, 0, 30, 100.0F, "an authored zero-frame override must win over a positive request"},
            FrameCase{0, 0, 30, 19.0F, "a disabled authored zero-frame value must not reset local offsets"},
            FrameCase{1, -1, 0, 100.0F, "a negative authored override must fall through to a zero-frame request"},
            FrameCase{1, -1, 30, 19.0F, "a negative authored override must fall through to a positive request"},
            FrameCase{1, -1, -1, 19.0F, "negative authored and requested frames must select the positive retail default"},
        };
        for (const auto& frame_case : cases) {
            definition->camera_param.event_enable_erp_frame = frame_case.authored_enabled;
            definition->camera_param.extra.cam_int = frame_case.authored_frames;
            auto player = smgpc::runtime::PlayerSystemService{};
            auto actor = CameraPlayerFixture(player, false);
            actor.camera_state.last_move = {15.0F, 0.0F, 0.0F};
            auto camera = smgpc::runtime::CameraSystemService{};
            camera.attach_event_camera_catalog(catalog);
            camera.declare_event_camera(5, name);
            camera.start_event_camera(5, name,
                smgpc::camera::EventCameraTarget::target_player(player), frame_case.requested_frames);
            require(!camera.active_event_camera_pose().has_value(),
                    "interpolation-frame selection must not calculate a pose during the request");
            camera.begin_frame(1U);
            require_near(camera.active_event_camera_pose()->watch.z,
                         frame_case.expected_offset, 0.0001F, frame_case.failure);
            camera.end_event_camera(5, name, true, -1);
            camera.detach_event_camera_catalog(catalog);
        }
    }

    void test_optional_real_disc_event_camera_resources() {
        const auto *disc_path = std::getenv("SMGPC_REAL_DISC");
        if (disc_path == nullptr || disc_path[0] == '\0') {
            std::cout
                << "[skip] real-disc actor event-camera test (set SMGPC_REAL_DISC)\n";
            return;
        }

        aurora_dvd_close();
        require(aurora_dvd_open(disc_path),
                "SMGPC_REAL_DISC should point to a readable SMG image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        auto authored = smgpc::scene::StageAuthoredData::resolve(
            dvd, "HeavensDoorGalaxy", 1, 0, 0);
        auto catalog = smgpc::camera::EventCameraCatalog::from_stage_tables(
            dvd, authored.tables());
        const auto *rabbit = catalog.find(5, "逃げウサギ集め固有016");
        const auto *child_pipe = catalog.find(5, "土管固有出現017");
        const auto *root_pipe = catalog.find(0, "土管固有出現054");
        require(rabbit != nullptr &&
                    rabbit->camera_param.id ==
                        "e:逃げウサギ集め固有016" &&
                    rabbit->camera_param.camera_type ==
                        "CAM_TYPE_EYEPOS_FIX" &&
                    child_pipe != nullptr &&
                    child_pipe->camera_param.id ==
                        "e:土管固有出現017" &&
                    child_pipe->camera_param.camera_type ==
                        "CAM_TYPE_XZ_PARA" &&
                    root_pipe != nullptr &&
                    root_pipe->camera_param.id ==
                        "e:土管固有出現054" &&
                    root_pipe->camera_param.camera_type ==
                        "CAM_TYPE_XZ_PARA",
                "real catalog must contain rabbit and root/child pipe event chunks");
        require(catalog.find(0, "逃げウサギ集め固有016") == nullptr &&
                    catalog.find(5, "土管固有出現054") == nullptr,
                "event-camera lookup must not cross a zone boundary");

        test_same_xz_request_preserves_original_state(catalog, dvd);
        test_fixed_point_event_lifecycle(catalog);
        test_fixed_point_event_interpolation_precedence(catalog);

        const auto collector = std::ranges::find_if(
            authored.placements(), [](const auto &placement) {
                return placement.creator_identifier == "RunawayRabbitCollect";
            });
        require(collector != authored.placements().end(),
                "real scenario must retain the collector placement");
        auto collector_parent_id = s32{-1};
        require(collector->jmap_info.getValue(
                    collector->jmap_entry_index, "l_id",
                    &collector_parent_id) &&
                    collector_parent_id >= 0,
                "real collector must retain its child-parent identity");
        const auto *child_info = collector->jmap_info.getChildObjInfo();
        require(child_info != nullptr,
                "real collector must retain its child-object JMap");
        auto tico_index = s32{-1};
        for (auto index = s32{}; index < child_info->getNumEntries(); ++index) {
            const char *name = nullptr;
            auto parent_id = s32{-1};
            if (child_info->getValue(index, "name", &name) &&
                name != nullptr && std::string_view(name) == "RunawayTico" &&
                child_info->getValue(index, "ParentID", &parent_id) &&
                parent_id == collector_parent_id) {
                tico_index = index;
                break;
            }
        }
        require(tico_index >= 0,
                "real collector must retain a matching RunawayTico child row");
        const auto collector_info = ActorCameraInfo(JMapInfoIter(
            &collector->jmap_info, collector->jmap_entry_index));
        const auto tico_info =
            ActorCameraInfo(JMapInfoIter(child_info, tico_index));
        require(collector_info.mCameraSetID == 16 &&
                    collector_info.mZoneID == 5 &&
                    tico_info.mCameraSetID == -1 && tico_info.mZoneID == 5,
                "ActorCameraInfo must restore retail ID and placed-zone fields");

        const auto tico_archive_path = dvd.find_object_archive("TicoBaby");
        require(tico_archive_path.has_value(),
                "TicoBaby.arc must exist for DemoMeetTico CANM");
        const auto &tico_archive = dvd.archive_for_path(*tico_archive_path);
        const auto animation_entry = std::ranges::find_if(
            tico_archive.entries(), [](const auto &entry) {
                const auto name = lower_copy(entry.name);
                return name == "demomeettico.camn" ||
                       name == "demomeettico.canm";
            });
        require(animation_entry != tico_archive.entries().end(),
                "TicoBaby.arc must contain the real DemoMeetTico camera animation");
        const auto animation = smgpc::camera::CameraAnimation::from_bytes(
            tico_archive.file_data(*animation_entry));
        require(animation.format() ==
                        smgpc::camera::CameraAnimationFormat::Ckan &&
                    animation.frame_count() == 1199U,
                "DemoMeetTico must parse as the retail 1199-frame CKAN");
        const auto sample0 = animation.sample(0.0F);
        const auto sample300 = animation.sample(300.0F);
        require(std::isfinite(sample0.eye.x) &&
                    std::isfinite(sample300.eye.x) &&
                    (sample0.eye.x != sample300.eye.x ||
                     sample0.watch.x != sample300.watch.x ||
                     sample0.fovy_degrees != sample300.fovy_degrees),
                "real CKAN sampling must produce finite authored motion");

        auto camera = smgpc::runtime::CameraSystemService{};
        auto binding = smgpc::scene::StageEventCameraBinding(
            camera, dvd, authored.tables());
        auto player = smgpc::runtime::PlayerSystemService{};
        auto player_actor = CameraPlayerFixture(player);
        set_player_matrix(player, 13000.0F, -10000.0F, 6000.0F);
        camera.declare_event_camera(5, "逃げウサギ集め固有016");
        camera.start_event_camera(
            5, "逃げウサギ集め固有016",
            smgpc::camera::EventCameraTarget::target_player(player), 0);
        camera.begin_frame(1U);
        const auto rabbit_pose = camera.active_event_camera_pose();
        require(rabbit_pose.has_value() && finite_pose(*rabbit_pose),
                "real EYEPOS_FIX collector camera must calculate a pose");

        camera.declare_event_camera(5, "土管固有出現017");
        camera.start_event_camera(
            5, "土管固有出現017",
            smgpc::camera::EventCameraTarget::target_player(player), 0);
        camera.begin_frame(2U);
        require(camera.active_event_camera_pose().has_value() &&
                    finite_pose(*camera.active_event_camera_pose()),
                "real XZ_PARA pipe camera must calculate a pose");

        camera.declare_event_camera_animation(5, "逃げチコDemoMeetTico",
                                              animation);
        camera.start_event_camera(
            5, "逃げチコDemoMeetTico",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 1.0F);
        camera.begin_frame(3U);
        require(camera.active_event_camera_key().has_value() &&
                    camera.active_event_camera_key()->zone_id == 5 &&
                    camera.active_event_camera_key()->name ==
                        "逃げチコDemoMeetTico" &&
                    camera.active_event_camera_pose().has_value() &&
                    finite_pose(*camera.active_event_camera_pose()),
                "real DemoMeetTico CKAN must preserve zone identity and target-player pose");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"ActorCameraInfo pool and identities",
                 test_actor_camera_info_pool_and_identity},
        TestCase{"FixedPoint offsets and safe distance", test_fixed_point_offset_accumulation_and_safe_distance},
        TestCase{"FixedPoint zone and transported up", test_fixed_point_zone_and_transport_up},
        TestCase{"FixedPoint global player up", test_fixed_point_mode_two_uses_global_player_up},
        TestCase{"linear CANM player target", test_linear_canm_player_target},
        TestCase{"uninitialized player event camera phase", test_uninitialized_player_event_waits_for_camera_phase},
        TestCase{"player camera capability and live sampling", test_player_camera_capability_and_live_sampling},
        TestCase{"fractional CANM and director pause", test_fractional_canm_camera_and_director_pause},
        TestCase{"fractional CKAN tangent layouts", test_fractional_ckan_tangent_layouts},
        TestCase{"CKAN duplicate key times", test_ckan_duplicate_keys_select_last_equal_time},
        TestCase{"CKAN search count and lookahead", test_ckan_search_count_and_lookahead_validation},
        TestCase{"native animation metadata and alignment", test_native_animation_metadata_and_alignment},
        TestCase{"animation copied manager fallback", test_animation_safe_pose_uses_copied_manager_seed},
        TestCase{"animation terminal pose and restart", test_animation_terminal_pose_and_restart},
        TestCase{"animation pause and resource ownership", test_animation_pause_and_resource_ownership},
        TestCase{"same animation request preserves cursor", test_same_animation_request_preserves_cursor},
        TestCase{"failed start is transactional",
                 test_failed_start_is_transactional},
        TestCase{"original matrix target movement and gravity", test_original_matrix_target_movement_and_gravity},
        TestCase{"matrix and object target camera phases", test_matrix_and_object_target_camera_phases},
        TestCase{"original actor target axes and state", test_original_actor_target_axes_and_state},
        TestCase{"actor target phases and retained orientation", test_actor_target_camera_phases_and_retained_orientation},
        TestCase{"matrix target lifetime and ABA",
                 test_matrix_target_lifetime_and_aba},
        TestCase{"optional real-disc event camera resources",
                 test_optional_real_disc_event_camera_resources},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what()
                      << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " actor event-camera test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " actor event-camera test(s) passed\n";
    return 0;
}
