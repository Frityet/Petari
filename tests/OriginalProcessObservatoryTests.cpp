#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Demo/GrandStarReturnDemoStarter.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/MapObj/AstroCore.hpp"
#include "Game/MapObj/AstroMapObj.hpp"
#include "Game/MapObj/PowerStar.hpp"
#include "Game/NPC/Rosetta.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SequenceUtil.hpp"

#include <aurora/allocation.hpp>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

#ifndef NDEBUG
struct Probe {
    std::uint64_t gateway_ready = 0;
    bool requested = false;
    bool observatory_ready = false;
    bool saw_return = false;
    bool saw_revival = false;
    std::string previous_nerve;

    void after_frame(GameSystem& system, std::uint64_t frame) {
        auto* controller = system.mSceneController;
        if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
            controller->getCurrentSceneForExecute() != controller->mScene ||
            !dynamic_cast<GameScene*>(controller->mScene)) return;
        if (!requested && std::strcmp(controller->mCurrSceneControlInfo.mStage, "HeavensDoorGalaxy") == 0) {
            if (!gateway_ready) gateway_ready = frame;
            if (frame < gateway_ready + 30) return;
            // Diagnostic stimulus at the actual stage-result API. This does
            // not represent playing or collecting the Grand Star in Gateway.
            GameSequenceFunction::sendStageResultSequenceParam("HeavensDoorGalaxy", 1, 0, 0);
            MR::requestChangeStageAfterStageClear();
            requested = true;
            std::fprintf(stderr, "[observatory-probe] Submitted diagnostic Grand Star result at frame %llu\n",
                         static_cast<unsigned long long>(frame));
        }
        if (std::strcmp(controller->mCurrSceneControlInfo.mStage, "AstroGalaxy") != 0) return;
        const aurora::allocation::HostAllocationScope host;
        GrandStarReturnDemoStarter* starter = nullptr;
        unsigned cores = 0, rosettas = 0, map_objects = 0;
        for (auto* object : NameObj::snapshotNativeObjects()) {
            cores += dynamic_cast<AstroCore*>(object) != nullptr;
            rosettas += dynamic_cast<Rosetta*>(object) != nullptr;
            map_objects += dynamic_cast<AstroMapObj*>(object) != nullptr;
            if (auto* cast = dynamic_cast<GrandStarReturnDemoStarter*>(object)) starter = cast;
        }
        require(requested && cores == 1 && rosettas == 1 && map_objects >= 20 && starter,
                "The original clear sequence must load the actual Observatory actor graph");
        if (!observatory_ready) {
            observatory_ready = true;
            std::fprintf(stderr, "[observatory-probe] Original Observatory ready at frame %llu: %u map objects\n",
                         static_cast<unsigned long long>(frame), map_objects);
        }
        if (starter->mFlag.mIsDead || !starter->mSpine) return;
        const auto* nerve = starter->mSpine->getCurrentNerve();
        const std::string current = nerve ? typeid(*nerve).name() : "";
        if (current.find("NrvMove") != std::string::npos) {
            require(starter->mPowerStar && !starter->mPowerStar->mFlag.mIsDead,
                    "The authored appear action must execute the actor's virtual lifecycle, including its Grand Star child");
        }
        for (const auto& row : starter->mPrevTransform.mMtx)
            for (const auto value : row)
                require(std::isfinite(value), "Original return-demo transforms must stay finite");
        if (current != previous_nerve) {
            std::fprintf(stderr, "[observatory-probe] frame=%llu return=%s step=%d\n",
                         static_cast<unsigned long long>(frame), current.c_str(), starter->mSpine->mStep);
            previous_nerve = current;
        }
        saw_return = true;
        saw_revival |= current.find("Revival") != std::string::npos || current.find("StageResult") != std::string::npos;
    }
};
#endif
}

int main(int argc, char** argv) {
#ifdef NDEBUG
    std::fprintf(stderr, "The Observatory diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const bool interactive = argc == 2 && std::strcmp(argv[1], "--interactive") == 0;
        require(argc == 1 || interactive, "Expected no arguments or --interactive");
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() / ("petari-observatory-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "The diagnostic requires a fresh isolated save");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original Comet Observatory arrival",
            .arguments = {"observatory-probe", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1"},
            .disc_image = disc,
        };
        if (!interactive) {
            configuration.arguments.push_back("--max-frames");
            configuration.arguments.push_back("2400");
        }
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
        const int result = smgpc::app::run_original_game(configuration, *logger, observer);
        require(result == 0 && probe.observatory_ready && probe.saw_return && probe.saw_revival,
                "Original stage handoff, Grand Star return and core revival must execute and retire normally");
        std::fprintf(stderr, "PASS original Observatory handoff and Grand Star arrival from diagnostic clear result\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL Observatory diagnostic: %s\n", error.what());
        return 1;
    }
#endif
}
