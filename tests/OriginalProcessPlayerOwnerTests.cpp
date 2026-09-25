#include "OriginalMarioStateTests.hpp"
#include "OriginalPlayerUtilTests.hpp"
#include "OriginalProcessMarioCameraTests.hpp"
#include "MarioWalkParameterTests.hpp"

#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/StageCollisionService.hpp"
#include "Game/Util/MtxUtil.hpp"

#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <unistd.h>

#ifndef NDEBUG
namespace {
    constexpr std::uint64_t frame_count = 360;

    void require(bool condition, const char* message) {
        if (!condition) aurora::throw_host_exception<std::runtime_error>(message);
    }

    template<class F>
    struct Restore final {
        F action;
        ~Restore() { action(); }
    };

    void verify_camera_head_timer(MarioActor& actor) {
        auto& mario = *actor.mMario;
        const auto head = mario.mHeadVec;
        const auto previous_head = actor._300;
        const auto up = actor.mUpVec;
        const auto front = actor._2DC;
        const auto side = actor._2E8;
        const auto timer = actor._330;
        const auto frozen = actor._EA5;
        const auto restore = Restore{[&] {
            mario.mHeadVec = head;
            actor._300 = previous_head;
            actor.mUpVec = up;
            actor._2DC = front;
            actor._2E8 = side;
            actor._330 = timer;
            actor._EA5 = frozen;
        }};
        const auto configured_timer = actor.mConst->getTable()->mCameraHeadRotationTimer;
        require(configured_timer > 2, "the actual Mario camera table has a multi-frame head rotation timer");
        actor._EA5 = false;
        mario.mHeadVec.set(0.0F, 1.0F, 0.0F);
        actor._300 = mario.mHeadVec;
        actor.mUpVec = mario.mHeadVec;
        actor._330 = 2;
        actor.updateForCamera();
        require(actor._330 == 1,
                "equal head values in distinct original vectors let the camera timer count down");
        actor.updateForCamera();
        require(actor._330 == 0, "a stable original head direction lets the camera timer expire");
        actor.updateForCamera();
        require(actor._330 == 0, "an unchanged original head direction does not restart an expired camera timer");

        mario.mHeadVec.set(0.0F, 0.0F, 1.0F);
        actor.updateForCamera();
        require(actor._330 == configured_timer - 1 && actor._300.x == 0.0F &&
                    actor._300.y == 0.0F && actor._300.z == 1.0F,
                "a changed original head direction copies the target and restarts the configured camera blend");
        actor.updateForCamera();
        require(actor._330 == configured_timer - 2,
                "the new stable camera head target continues counting down on the next update");
        std::fprintf(stderr, "[original-player-owner] PASS original camera stable-head timer expiry and changed-head reset; table=%u\n",
                     static_cast<unsigned>(configured_timer));
    }

    void verify_recovery_safety_position(MarioActor& actor) {
        auto& mario = *actor.mMario;
        auto* collision = smgpc::scene::StageCollisionService::active();
        require(collision != nullptr, "recovery safety test requires the actual stage collision owner");
        require(!mario.isStatusActive(8) && !mario.isStatusActive(19),
                "the ordinary opening player is outside fire damage and recovery states");
        auto down = actor.getGravityVec();
        const auto length = std::sqrt(down.dot(down));
        require(std::isfinite(length) && length > 0.0F, "original Mario gravity is finite");
        down.scale(1.0F / length);
        TVec3f hit_position;
        Triangle original;
        require(MR::getFirstPolyOnLineToMap(&hit_position, &original, actor.mPosition - down * 250.0F, down * 1000.0F),
                "recovery safety test finds an actual authored KCL face under Mario");
        require(original.mParts && original.getSensor() && !original.getSensor()->isType(0x48),
                "the tested face retains real collision parts and an eligible original sensor");
        require(original.isValid() && MR::isSameMtx(original.getBaseMtx()->toMtxPtr(),
                                                   original.getPrevBaseMtx()->toMtxPtr()),
                "recovery safety test borrows an actual stationary original Triangle identity");
        const auto normal = *original.getNormal(0);
        const auto center = (original.mPos[0] + original.mPos[1] + original.mPos[2]) * (1.0F / 3.0F);
        auto tangent = original.mPos[1] - original.mPos[0];
        const auto tangent_length = std::sqrt(tangent.dot(tangent));
        require(tangent_length > 0.01F, "the actual KCL face has a nondegenerate edge");
        tangent.scale(1.0F / tangent_length);

        const auto position = mario.mPosition;
        const auto gravity = mario.mAirGravityVec;
        auto* ground = mario.mGroundPolygon;
        auto* first_triangle = mario._7E0;
        auto* second_triangle = mario._820;
        const auto first_position = mario._7D4;
        const auto second_position = mario._814;
        const auto first_matrix = mario._7E4;
        const auto second_matrix = mario._824;
        const auto flags = mario._1C_WORD;
        const auto movement = mario.mMovementStates;
        const auto draw = mario.mDrawStates_WORD;
        const auto speed = mario.mVerticalSpeed;
        const auto floor = mario._960;
        const auto timer = mario._96A;
        const auto restore = Restore{[&] {
            mario.mPosition = position;
            mario.mAirGravityVec = gravity;
            mario.mGroundPolygon = ground;
            mario._7E0 = first_triangle;
            mario._820 = second_triangle;
            mario._7D4 = first_position;
            mario._814 = second_position;
            mario._7E4 = first_matrix;
            mario._824 = second_matrix;
            mario._1C_WORD = flags;
            mario.mMovementStates = movement;
            mario.mDrawStates_WORD = draw;
            mario.mVerticalSpeed = speed;
            mario._960 = floor;
            mario._96A = timer;
        }};
        Triangle candidate, first, second;
        mario.mGroundPolygon = &candidate;
        mario._7E0 = &first;
        mario._820 = &second;
        mario.mAirGravityVec = -normal;
        mario.mMovementStates._8 = false;
        mario.mMovementStates._32 = false;
        mario.mMovementStates._19 = false;
        mario.mMovementStates._1A = false;
        mario.mDrawStates._C = false;
        mario.mVerticalSpeed = 0.0F;
        mario._960 = 0;
        const auto near = [](const TVec3f& actual, const TVec3f& expected) {
            return (actual - expected).length() < 0.025F;
        };
        const auto prepare = [&](const TVec3f& translation, float offset) {
            candidate = original;
            for (auto& vertex : candidate.mPos) vertex += translation;
            first.mIdx = second.mIdx = 0xFFFFFFFFU;
            mario._7D4.set(-123.0F, 456.0F, -789.0F);
            mario._814.set(987.0F, -654.0F, 321.0F);
            mario.mPosition = center + translation + tangent * offset;
            mario._1C_WORD = 0x00004000U;
            mario._96A = 0;
        };

        // Test the original cached-world-vertex arithmetic with a real KCL
        // identity. Translated copies stay local to this call; the scene's
        // collision geometry and actor position are never republished.
        TVec3f first_target;
        const std::array<TVec3f, 2> translations = {TVec3f(0, 0, 0), TVec3f(12000, -10000, 8000)};
        for (std::size_t index = 0; index < translations.size(); ++index) {
            const auto& translation = translations[index];
            prepare(translation, 16.0F);
            mario.saveLastSafetyTrans();
            const auto expected = center + translation + tangent * 10.0F;
            std::fprintf(stderr, "[recovery-safety] translated=%zu expected=(%g,%g,%g) actual=(%g,%g,%g) error=%g\n",
                         index, expected.x, expected.y, expected.z, mario._7D4.x, mario._7D4.y, mario._7D4.z,
                         (mario._7D4 - expected).length());
            require(first.isValid() && near(mario._7D4, expected),
                    "original safety blend keeps five-eighths player offset from the actual face centroid");
            require((mario._1C_WORD & 0x00020000U) != 0,
                    "successful original safety save sets its availability flag");
            TVec3f returned_normal;
            const auto* saved = mario.getLastSafetyTrans(&returned_normal);
            require(saved == &mario._7D4 && near(*saved, expected) && near(returned_normal, normal),
                    "original recovery retrieves the saved point and actual KCL face normal");
            const auto target = *saved + returned_normal * 160.0F;
            require(near(target, expected + normal * 160.0F),
                    "original recovery target inputs retain the 160-unit face-normal lift");
            if (index == 0) first_target = target;
            else require(near(target - translation, first_target),
                         "translating player and cached world vertices translates the recovery target equally");
            prepare(translation, 160.0F);
            mario.saveLastSafetyTrans();
            require(near(mario._7D4, center + translation + tangent * 110.0F),
                    "original safety displacement toward the face centroid is capped at 50 units");
        }
        prepare(TVec3f(0, 0, 0), 16.0F);
        const auto sentinel = mario._7D4;
        mario._96A = 2;
        mario.saveLastSafetyTrans();
        require(mario._96A == 1 && !first.isValid() && near(mario._7D4, sentinel),
                "original safety delay decrements without publishing a new point");
        mario._96A = 0;
        mario._1C_WORD = 0;
        mario.saveLastSafetyTrans();
        require(!first.isValid() && near(mario._7D4, sentinel),
                "missing original ground eligibility leaves the safety point unchanged");
        std::fprintf(stderr, "[original-player-owner] PASS original safety blend, 50-unit clamp, translation invariance, real KCL recovery normal/target inputs and delay/eligibility gates\n");
    }

    struct Probe {
        bool stack_and_walk = false;
        bool camera = false;
        bool utilities = false;
        bool recovery_safety = false;
        std::uint64_t normal_frames_after_restore = 0;
        std::uint64_t cadence_samples = 0;
        std::uint64_t last_observed_frame = 0;
        u16 last_movement_counter = 0;
        bool cadence_started = false;
        const MarioActor* identity = nullptr;

        void after_frame(GameSystem& system, std::uint64_t frame) {
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;

            const aurora::allocation::HostAllocationScope host;
            auto* actor = MarioAccess::getPlayerActor();
            require(actor && actor->mMario && actor->mConst && actor->mMarioAnim && actor->mBinder &&
                        MR::getMarioHolder() && MR::getMarioHolder()->getMarioActor() == actor,
                    "normal original placement initializes the actual Mario, constants, animator, Binder and holder");
            require(!identity || identity == actor, "the original player owner survives every observed scene frame");
            identity = actor;
            const auto domain = MR::getSceneObjHolder()->nativeAllocationDomain();
            require(domain != nullptr, "the actual original scene owns test allocations");
            const smgpc::compat::JkrAllocationScope allocation(domain);
            const J3DSys::CommandScope commands;

            if (!stack_and_walk) {
                auto* active = actor->mMario->_97C;
                auto* noticed = actor->mMario->_980;
                smgpc::tests::verify_original_mario_state_lifecycle(*actor);
                smgpc::tests::verify_original_mario_walk_parameters(*actor);
                require(actor->mMario->_97C == active && actor->mMario->_980 == noticed,
                        "retained state-stack and walking checks restore the original live stack owner");
                stack_and_walk = true;
                std::fprintf(stderr, "[original-player-owner] PASS retained canonical MarioState stack and walking parameters; frame=%llu\n",
                             static_cast<unsigned long long>(frame));
            } else if (!utilities) {
                ++normal_frames_after_restore;
            }

            if (!camera && MR::isDemoActive()) {
                smgpc::tests::verify_original_process_mario_camera(*actor);
                verify_camera_head_timer(*actor);
                camera = true;
                std::fprintf(stderr, "[original-player-owner] PASS camera fields, actual gravity, bind and timer checks under the real demo; frame=%llu\n",
                             static_cast<unsigned long long>(frame));
            }

            // _378 is incremented at entry to the canonical MarioActor::movement
            // and otherwise only reset by construction/init. Observe it after
            // the retained early injected checks, without changing any state.
            // This checks actual process cadence, not host wall-clock FPS.
            if (cadence_started) {
                require(frame == last_observed_frame + 1 &&
                            static_cast<u16>(actor->_378 - last_movement_counter) == 1,
                        "each observed original process frame executes exactly one MarioActor movement");
                ++cadence_samples;
            }
            cadence_started = true;
            last_observed_frame = frame;
            last_movement_counter = actor->_378;

            if (frame == frame_count - 2) {
                verify_recovery_safety_position(*actor);
                recovery_safety = true;
            }

            if (frame == frame_count - 1) {
                require(stack_and_walk && camera && normal_frames_after_restore >= 100 && cadence_samples >= 100,
                        "restored test fields survive at least one hundred ordinary original scene frames");
                std::fprintf(stderr, "[original-player-owner] PASS actual process cadence: %llu consecutive frames with exactly one original movement each\n",
                             static_cast<unsigned long long>(cadence_samples));
                // This retained module ends with the original public control
                // reset, which legitimately changes animation and statuses.
                // Run only after the final game frame; ordinary teardown is
                // next. Do not fabricate restoration of those transitions.
                smgpc::tests::verify_original_player_util(*actor);
                utilities = true;
                std::fprintf(stderr, "[original-player-owner] PASS retained PlayerUtil owner/KCL/reset checks on terminal frame=%llu\n",
                             static_cast<unsigned long long>(frame));
            }
        }
    };
}
#endif

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "Original player owner diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-player-owner-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "diagnostic starts with a fresh console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE", "SMGPC_STRICT_PLACEMENT"})
            unsetenv(name);
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original player owner regression",
            .arguments = {"original-player-owner-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", std::to_string(frame_count)},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct DiscLifetime { ~DiscLifetime() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 &&
                    probe.stack_and_walk && probe.camera && probe.utilities && probe.recovery_safety,
                "actual original process reaches every retained player check and normal retirement");
        require(probe.identity && !smgpc::compat::has_actor_runtime_state(probe.identity),
                "ordinary process teardown retires the original player owner");
        std::fprintf(stderr, "PASS original player owner: retained state stack, walking parameters, player utilities, real-demo camera and retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original player owner: %s\n", error.what());
        return 1;
    }
#endif
}
