#include "CameraTargetTestSupport.hpp"
#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "camera/CameraAnimation.hpp"
#include "camera/EventCamera.hpp"
#include "compat/CameraUtilCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/StageAuthoredData.hpp"
#include "scene/StageEventCameraBinding.hpp"

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
        if (std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) +
                                     ": actual=" + std::to_string(actual) +
                                     ";expected=" + std::to_string(expected));
        }
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
        TestCase{"linear CANM player target", test_linear_canm_player_target},
        TestCase{"uninitialized player event camera phase", test_uninitialized_player_event_waits_for_camera_phase},
        TestCase{"player camera capability and live sampling", test_player_camera_capability_and_live_sampling},
        TestCase{"failed start is transactional",
                 test_failed_start_is_transactional},
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
