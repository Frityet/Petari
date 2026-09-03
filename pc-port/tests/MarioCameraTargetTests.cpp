#include "MarioCameraTargetTests.hpp"

#include "Game/Camera/CameraTargetObj.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::tests {
    namespace {
        void require(bool condition, std::string_view message) {
            if (!condition) {
                throw std::runtime_error(std::string(message));
            }
        }

        bool near(const TVec3f& actual, const TVec3f& expected) {
            return std::fabs(actual.x - expected.x) < 0.0001F &&
                   std::fabs(actual.y - expected.y) < 0.0001F &&
                   std::fabs(actual.z - expected.z) < 0.0001F;
        }

        class ScopedMarioCameraFields final {
        public:
            explicit ScopedMarioCameraFields(MarioActor& actor)
                : actor(actor), mario(*actor.mMario), flags(actor.mFlag), mode(actor.mPlayerMode),
                  timer(actor._378), bound(actor._934), bound_sensor(actor._924), up(actor.mUpVec), camera_position(actor.mCameraTrans),
                  last_move(actor.mLastMove), front(mario.mFrontVec), side(mario.mSideVec),
                  air_gravity(mario.mAirGravityVec), shadow(mario.mShadowPos), ground(mario.mGroundPos),
                  states(mario.mMovementStates), status(mario._97C) {
            }

            ~ScopedMarioCameraFields() {
                actor.mFlag = flags;
                actor.mPlayerMode = mode;
                actor._378 = timer;
                actor._934 = bound;
                actor._924 = bound_sensor;
                actor.mUpVec = up;
                actor.mCameraTrans = camera_position;
                actor.mLastMove = last_move;
                mario.mFrontVec = front;
                mario.mSideVec = side;
                mario.mAirGravityVec = air_gravity;
                mario.mShadowPos = shadow;
                mario.mGroundPos = ground;
                mario.mMovementStates = states;
                mario._97C = status;
            }

        private:
            MarioActor& actor;
            Mario& mario;
            LiveActorFlag flags;
            u16 mode;
            u16 timer;
            bool bound;
            HitSensor* bound_sensor;
            TVec3f up;
            TVec3f camera_position;
            TVec3f last_move;
            TVec3f front;
            TVec3f side;
            TVec3f air_gravity;
            TVec3f shadow;
            TVec3f ground;
            Mario::MovementStates states;
            MarioState* status;
        };

        void write_be32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
            for (auto index = 0U; index < 4U; ++index) {
                bytes[offset + index] = static_cast<std::uint8_t>(value >> (24U - 8U * index));
            }
        }

        scene::StagePlacementObject make_demo_definition(std::string_view time_sheet) {
            constexpr auto field_count = 3U;
            constexpr auto entry_size = 12U;
            constexpr auto data_offset = 0x10U + field_count * 0x0cU;
            constexpr auto demo_name = std::string_view{"CameraTargetFixture"};
            auto strings = std::string(demo_name);
            strings.push_back('\0');
            const auto sheet_offset = static_cast<std::uint32_t>(strings.size());
            strings.append(time_sheet);
            strings.push_back('\0');
            auto bytes = std::vector<std::uint8_t>(data_offset + entry_size + strings.size());
            write_be32(bytes, 0U, 1U);
            write_be32(bytes, 4U, field_count);
            write_be32(bytes, 8U, data_offset);
            write_be32(bytes, 12U, entry_size);
            constexpr auto fields = std::array<std::string_view, field_count>{"DemoName", "TimeSheetName", "l_id"};
            for (auto index = 0U; index < field_count; ++index) {
                const auto descriptor = 0x10U + index * 0x0cU;
                write_be32(bytes, descriptor, resource::jmap_hash(fields[index]));
                write_be32(bytes, descriptor + 4U, 0xffffffffU);
                bytes[descriptor + 9U] = static_cast<std::uint8_t>(index * 4U);
                bytes[descriptor + 11U] = static_cast<std::uint8_t>(
                    index < 2U ? resource::BcsvFieldType::StringOffset : resource::BcsvFieldType::Int32);
            }
            write_be32(bytes, data_offset + 4U, sheet_offset);
            write_be32(bytes, data_offset + 8U, 1U);
            std::copy(strings.begin(), strings.end(), bytes.begin() + data_offset + entry_size);
            auto info = JMapInfo::from_bcsv(bytes);
            info.setPlacedZoneId(0);
            return {
                .object_name = "DemoGroup",
                .table_path = "jmp/placement/layera/DemoObjInfo",
                .l_id = 1,
                .zone_id = 0,
                .jmap_info = std::move(info),
                .jmap_entry_index = 0,
            };
        }
    }

    void verify_original_mario_camera_target(
        MarioActor& actor, runtime::DvdFileSystemService& dvd,
        const compat::DemoSceneRuntime& scene_demo) {
        require(actor.mMario != nullptr &&
                    MR::getMarioHolder() != nullptr && MR::getMarioHolder()->getMarioActor() == &actor,
                "original camera target checks require the initialized MarioHolder actor");
        require(!scene_demo.definitions().empty() && !scene_demo.is_active(),
                "original camera target checks require an idle scene demo definition");
        const auto restore = ScopedMarioCameraFields(actor);
        auto& mario = *actor.mMario;
        actor.mFlag.mIsDead = false;
        actor.mFlag.mIsClipped = false;
        actor.mPlayerMode = 0;
        actor._934 = false;
        actor._378 = 41U;
        mario._97C = nullptr;
        mario.mMovementStates = {};
        actor.mUpVec.set(0.0F, 4.0F, 0.0F);
        mario.mFrontVec.set(0.0F, 0.0F, 2.0F);
        mario.mSideVec.set(3.0F, 0.0F, 0.0F);
        actor.mCameraTrans.set(10.0F, 20.0F, 30.0F);
        actor.mLastMove.set(3.0F, 4.0F, 5.0F);
        mario.mShadowPos.set(100.0F, 200.0F, 300.0F);
        mario.mGroundPos.set(400.0F, 500.0F, 600.0F);
        mario.mAirGravityVec.set(0.25F, -0.5F, 0.75F);

        auto target = CameraTargetPlayer("Original player target regression");
        target.mActor = &actor;
        target.movement();
        require(near(target.getUpVec(), TVec3f(0.0F, 1.0F, 0.0F)) &&
                    near(target.getFrontVec(), mario.mFrontVec) && near(target.getSideVec(), mario.mSideVec) &&
                    near(target.getGroundPos(), mario.mShadowPos) && near(target.getGravityVector(), mario.mAirGravityVec),
                "original player target must normalize only up and cache shadow position and air gravity");
        require(target.mPlayerMovementTimer == 41U && target.mIsPlayerMoving,
                "the original first camera phase must observe Mario's changed movement timer");

        actor.mPlayerMode = 4;
        auto stage_gravity = TVec3f{};
        require(MR::calcGravityVector(&target, actor.mPosition, &stage_gravity, nullptr, 0),
                "the Bee target fixture requires the stage's original gravity manager");
        target.movement();
        require(near(target.getGravityVector(), stage_gravity) &&
                    !near(target.getGravityVector(), mario.mAirGravityVec),
                "Bee camera targets must sample stage gravity instead of Mario's cached air gravity");
        actor.mPlayerMode = 0;
        target.movement();
        require(near(target.getGravityVector(), mario.mAirGravityVec),
                "leaving Bee mode must restore the original air-gravity getter");

        const auto cached_up = target.getUpVec();
        const auto cached_front = target.getFrontVec();
        const auto cached_side = target.getSideVec();
        const auto cached_ground = target.getGroundPos();
        const auto cached_gravity = target.getGravityVector();
        actor.mFlag.mIsDead = true;
        actor._378 = 42U;
        actor.mUpVec.set(1.0F, 0.0F, 0.0F);
        mario.mFrontVec.set(0.0F, 1.0F, 0.0F);
        mario.mSideVec.set(0.0F, 0.0F, 1.0F);
        mario.mShadowPos.set(900.0F, 800.0F, 700.0F);
        actor.mCameraTrans.set(11.0F, 22.0F, 33.0F);
        target.movement();
        require(near(target.getUpVec(), cached_up) && near(target.getFrontVec(), cached_front) &&
                    near(target.getSideVec(), cached_side) && near(target.getGroundPos(), cached_ground) &&
                    near(target.getGravityVector(), cached_gravity) && target.mPlayerMovementTimer == 41U &&
                    near(target.getPosition(), actor.mCameraTrans),
                "dead Mario must retain cached target fields while the position getter remains live");
        actor.mFlag.mIsDead = false;
        actor.mFlag.mIsClipped = true;
        target.movement();
        require(near(target.getUpVec(), cached_up) && near(target.getFrontVec(), cached_front) &&
                    near(target.getSideVec(), cached_side) && target.mPlayerMovementTimer == 41U,
                "clipped Mario must also retain orientation and movement-timer state");

        actor.mFlag.mIsClipped = false;
        // The original _934 path requires a live rush host sensor. This
        // test host owns a real Binder with no ground contact; no fake Mario
        // movement state or unavailable MarioWait object is constructed.
        auto bound_host = LiveActor("Camera target bound host fixture");
        bound_host.initBinder(10.0F, 0.0F, 0U);
        auto bound_sensor = HitSensor(0U, 0U, 10.0F, &bound_host);
        auto* previous_bound_sensor = actor._924;
        actor._924 = &bound_sensor;
        actor._934 = true;
        require(MR::isPlayerInBind(), "the live rush-host relationship must expose original player binding");
        const auto matrix = MR::getPlayerBaseMtx();
        require(matrix != nullptr, "bound player target needs the original base-matrix provider");
        auto expected_up = TVec3f(matrix[0][1], matrix[1][1], matrix[2][1]);
        const auto length = std::sqrt(expected_up.x * expected_up.x + expected_up.y * expected_up.y + expected_up.z * expected_up.z);
        expected_up.x /= length;
        expected_up.y /= length;
        expected_up.z /= length;
        const auto expected_side = TVec3f(matrix[0][0], matrix[1][0], matrix[2][0]);
        const auto expected_front = TVec3f(matrix[0][2], matrix[1][2], matrix[2][2]);
        target.movement();
        require(near(target.getUpVec(), expected_up) && near(target.getSideVec(), expected_side) &&
                    near(target.getFrontVec(), expected_front),
                "bound orientation must use base-matrix columns rather than normal camera getter vectors");
        require(target.getGroundTriangle() == nullptr,
                "an airborne bound host must not fabricate a player grounding polygon");
        actor._934 = false;
        actor._924 = previous_bound_sensor;
        actor.mUpVec.zero();
        target.movement();
        require(near(target.getUpVec(), TVec3f(0.0F, 1.0F, 0.0F)),
                "zero unbound up must use the original target fallback");

        actor._378 = 0xffffU;
        target.movement();
        require(target.mIsPlayerMoving && near(target.getLastMove(), actor.mLastMove),
                "outside demos last move remains the live original getter");
        const auto placements = std::array{make_demo_definition(scene_demo.definitions().front().time_sheet_name)};
        const auto* previous_demo = compat::active_demo_scene_runtime();
        {
            auto demo = compat::DemoSceneRuntime(dvd, placements);
            auto starter = NameObj("Camera target timer regression");
            const auto started = demo.start_demo(&starter, "CameraTargetFixture", std::nullopt, compat::DemoPlayerMode::Normal);
            require(started == compat::DemoSheetStartResult::Started && MR::isDemoActive(),
                    "timer checks need a real active demo owner");
            target.movement();
            require(!target.mIsPlayerMoving && near(target.getLastMove(), TVec3f(0.0F, 0.0F, 0.0F)),
                    "a demo with unchanged Mario movement timer must suppress stale last move");
            actor._378 = 0U;
            target.movement();
            require(target.mIsPlayerMoving && near(target.getLastMove(), actor.mLastMove),
                    "u16 movement-timer wrap must still count as actual player movement");
            require(near(target.getLastMove(), actor.mLastMove) && target.mIsPlayerMoving,
                    "repeated target sampling must not mutate movement-phase state");
            target.movement();
            require(!target.mIsPlayerMoving && near(target.getLastMove(), TVec3f(0.0F, 0.0F, 0.0F)),
                    "a second movement in one camera phase demonstrates why the owner must advance once");
            require(demo.stop_active_demo(&starter, "CameraTargetFixture") &&
                        near(target.getLastMove(), actor.mLastMove),
                    "ending the demo must restore lastMove even while the movement flag stays false");
        }
        require(compat::active_demo_scene_runtime() == previous_demo && !scene_demo.is_active(),
                "the isolated demo fixture must restore the original scene owner without advancing its clock");
    }
}
