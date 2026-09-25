#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/Effect/MultiEmitter.hpp"
#include "Game/Effect/SingleEmitter.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/Cp932Literal.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
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
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }

    struct Probe {
        bool exercised = false;
        bool followed = false;
        std::uint64_t ready_frame = 0;
        std::uint64_t action_frame = 0;
        const MarioActor* actor_identity = nullptr;
        const EffectSystem* effects_identity = nullptr;

        void after_frame(GameSystem& system, std::uint64_t frame) {
            if (exercised) {
                if (!followed && frame == action_frame + 1) {
                    auto* actor = MarioAccess::getPlayerActor();
                    require(actor == actor_identity && actor->mMarioAnim && actor->mEffectKeeper,
                            "The same real player and animation/effect owners survive the following normal frame");
                    std::fprintf(stderr, "[trample-action-probe] following frame=%llu combo=%u authored_animation=%u animation_frame=%g effect_valid=%u\n",
                                 static_cast<unsigned long long>(frame), actor->_988,
                                 actor->isAnimationRun(CP932("ジャンプふみ1")),
                                 actor->mMarioAnim->mXanimePlayer->tellAnimationFrame(),
                                 actor->mEffectKeeper->getEmitter(CP932("ふみつぶし"))->isValid());
                    followed = true;
                }
                return;
            }
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            if (!ready_frame) ready_frame = frame;
            if (frame < 1800) return;

            const aurora::allocation::HostAllocationScope host;
            auto* actor = MarioAccess::getPlayerActor();
            require(actor && actor->mMario && actor->mMarioAnim && actor->mModelManager && actor->mEffectKeeper,
                    "Actual original player, animator, model and EffectKeeper exist after normal scene initialization");
            auto* mario = actor->mMario;
            // Wait for ordinary action prerequisites; never alter the original
            // player mode, effect gates or movement flags to manufacture them.
            if (frame % 100 == 0)
                std::fprintf(stderr, "[trample-action-probe] eligibility frame=%llu animation_lock=%u demo=%u status=%u combo=%u\n",
                             static_cast<unsigned long long>(frame), actor->_B90, MR::isDemoActive(), mario->getCurrentStatus(), actor->_988);
            if (actor->_B90 || MR::isDemoActive() || actor->_482 || actor->_483 || mario->mMovementStates._A ||
                mario->getCurrentStatus() != MarioStatus_None) return;
            require(actor->mPlayerMode != PlayerMode_Hopper && actor->_988 == 0,
                    "Original opening provides the ordinary first-trample state");
            auto* effects = MR::getEffectSystem();
            require(effects && effects->mEmitterManager && effects->mEmitterHolder,
                    "Actual original effect system and JPA emitter owners exist");
            auto* keeper = actor->mEffectKeeper;
            constexpr auto& effect_name = CP932("ふみつぶし");
            require(keeper->isRegisteredEmitter(effect_name), "Retail trample effect exists in the real player resources");
            auto* emitter = keeper->getEmitter(effect_name);
            require(emitter && !emitter->isValid(), "Original trample emitter exists and is inactive before the injected action");

            const auto domain = smgpc::scene::current_scene_allocation_domain();
            require(domain != nullptr, "Actual scene owns allocations performed by the original action");
            {
                const smgpc::compat::JkrAllocationScope allocation(domain);
                const J3DSys::CommandScope commands;
                const auto* table = actor->getConst().getTable();
                // This intentionally injects one public action. It proves its
                // real animation/effect closure, not a sensor-driven stomp.
                actor->trampleJump(table->mTrampleNormal, table->mTrampleLong);
            }

            std::fprintf(stderr, "[trample-action-probe] action frame=%llu animation_lock=%u combo=%u authored_animation=%u effect_valid=%u bck=%s\n",
                         static_cast<unsigned long long>(frame), actor->_B90, actor->_988,
                         actor->isAnimationRun(CP932("ジャンプふみ1")), emitter->isValid(),
                         actor->mMarioAnim->mXanimePlayer->getCurrentBckName());
            require(actor->_988 == 1 && actor->isAnimationRun(CP932("ジャンプふみ1")),
                    "Original first trample selects its authored animation and combo state");
            require(keeper == actor->mEffectKeeper && emitter == keeper->getEmitter(effect_name) && emitter->isValid(),
                    "Original action activates the existing authored MultiEmitter");
            bool live_particle = false;
            for (s32 index = 0; index < emitter->mEmitters.size(); ++index) {
                auto* particle = emitter->getParticleEmitter(index);
                if (particle && particle->isValid()) live_particle = true;
            }
            require(live_particle, "Original action creates a real live JPA-backed particle emitter");
            require(!mario->mMovementStates._2F && !mario->mMovementStates._22 && mario->mMovementStates._3E == 0,
                    "Original action applies its final movement-state resets");
            actor_identity = actor;
            effects_identity = effects;
            action_frame = frame;
            exercised = true;
            std::fprintf(stderr, "[trample-action-probe] PASS injected public action with original Mario, animation, MultiEmitter and JPA resources; frame=%llu\n",
                         static_cast<unsigned long long>(frame));
        }
    };
}
#endif

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "This original-process diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-original-trample-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic starts with a fresh native console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        setenv("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "1400-1410:A;1500-1510:A;1600-1610:A;1700-1710:A;1800-1810:A;1900-1910:A;2000-2010:A", 1);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original trample action integration",
            .arguments = {"original-trample-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "2100"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct Disc { ~Disc() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised && probe.followed && probe.action_frame < 2090,
                "OriginalProcess completes the injected action and at least ten following normal frames");
        require(!smgpc::compat::has_actor_runtime_state(probe.actor_identity) &&
                    !smgpc::compat::has_name_obj_runtime_state(probe.effects_identity),
                "Normal original scene retirement releases the actual player and effect system owners");
        std::fprintf(stderr, "PASS original-process injected trample action: original animation, live particle emission, normal frames and owner retirement\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-process injected trample action: %s\n", error.what());
        return 1;
    }
#endif
}
