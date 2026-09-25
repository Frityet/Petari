#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Map/ScenarioSelectStar.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/ScenarioSelectScene.hpp"
#include "Game/Screen/ScenarioSelectLayout.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <unistd.h>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

#ifndef NDEBUG
struct Probe {
    std::uint64_t ready_frame = 0;
    bool requested = false;
    bool saw_selection = false;

    void after_frame(GameSystem& system, std::uint64_t frame) {
        auto* controller = system.mSceneController;
        if (!controller) return;
        if (!requested) {
            if (controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            if (!ready_frame) ready_frame = frame;
            if (frame < ready_frame + 30 || MR::isDemoActive()) return;
            // Exercise the same original sequence entry used by the dome after
            // confirmation. This is a diagnostic stimulus, not a played route.
            MR::requestStartScenarioSelect("EggStarGalaxy");
            requested = true;
        }
        auto* scene = controller->mScenarioSelectScene;
        if (!scene || !scene->isExecForeground()) return;
        auto* layout = scene->mScenarioLayout;
        if (!layout || !layout->mSpine) return;
        const auto* nerve = layout->mSpine->getCurrentNerve();
        if (!nerve || std::string_view(typeid(*nerve).name()).find("NrvWaitScenarioSelect") == std::string_view::npos) return;
        require(MR::getPane(layout, "NewStarGreen") && MR::getPane(layout, "ShaCoinNum"),
                "Original mission layout resolves the authored pane names");
        require(layout->mStar[0]->mScenarioNo == 1 && layout->mStar[0]->mStarCollectedStatus == 0,
                "The first uncollected mission uses the original new-star state");
        require(layout->mScenarioSky->mScale.x == 10.0f && layout->mScenarioSky->mScale.y == 10.0f &&
                    layout->mScenarioSky->mScale.z == 10.0f,
                "The original sky scale is independent of pane positioning");
        require(std::strcmp(layout->mMarioPaneName, "MarioPosition01") == 0,
                "Single-digit lives use the original one-digit position");
        if (!saw_selection)
            std::fprintf(stderr, "[scenario-select-probe] PASS original mission selection ready at frame %llu; saved stars=%d\n",
                         static_cast<unsigned long long>(frame), GameDataFunction::calcCurrentPowerStarNum());
        saw_selection = true;
    }
};
#endif
}

int main() {
#ifdef NDEBUG
    std::fprintf(stderr, "The mission-selection diagnostic requires a debug build.\n");
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        const char* source = std::getenv("SMGPC_TEST_SAVE_DIR");
        require(disc && *disc && source && *source, "Set SMGPC_REAL_DISC and SMGPC_TEST_SAVE_DIR to a save before the first Good Egg mission");
        const auto save = std::filesystem::temp_directory_path() / ("petari-scenario-select-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "Diagnostic save copy must be new");
        std::filesystem::copy(source, save, std::filesystem::copy_options::recursive);
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original mission selection diagnostic",
            .arguments = {"scenario-select-probe", "--stage", "AstroGalaxy", "--scenario", "1", "--save-slot", "1", "--max-frames", "600"},
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
        const int result = smgpc::app::run_original_game(configuration, *logger, observer);
        require(result == 0 && probe.saw_selection, "Original mission selection must become ready and retire cleanly");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original mission selection: %s\n", error.what());
        return 1;
    }
#endif
}
