#include "OriginalMarioStateTests.hpp"
#include "OriginalPlayerUtilTests.hpp"
#include "OriginalProcessMarioCameraTests.hpp"
#include "MarioWalkParameterTests.hpp"

#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/allocation.hpp>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <unistd.h>

#ifndef NDEBUG
namespace {
    constexpr std::uint64_t frame_count = 360;

    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    struct Probe {
        bool stack_and_walk = false;
        bool camera = false;
        bool utilities = false;
        std::uint64_t normal_frames_after_restore = 0;
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
            const auto domain = smgpc::scene::current_scene_allocation_domain();
            require(domain != nullptr, "the actual original scene owns test allocations");
            const smgpc::compat::JkrAllocationScope allocation(domain);
            const smgpc::compat::J3dCommandScope commands;

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
                camera = true;
                std::fprintf(stderr, "[original-player-owner] PASS camera fields, actual gravity, bind and timer checks under the real demo; frame=%llu\n",
                             static_cast<unsigned long long>(frame));
            }

            if (frame == frame_count - 1) {
                require(stack_and_walk && camera && normal_frames_after_restore >= 100,
                        "restored test fields survive at least one hundred ordinary original scene frames");
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
                    probe.stack_and_walk && probe.camera && probe.utilities,
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
