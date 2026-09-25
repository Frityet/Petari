#pragma once

#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include <aurora/allocation.hpp>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace smgpc::test {
// Execute resource assertions only after the actual original process finishes
// constructing its GameScene and StageDataHolder from the retail archive.
template<class Verify>
int run_stage_resource_process(const char* label, Verify verify) {
#ifdef NDEBUG
    return 1;
#else
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        if (!disc || !*disc) throw std::runtime_error("SMGPC_REAL_DISC must name the actual disc image");
        const auto save = std::filesystem::temp_directory_path() / (std::string("petari-") + label + "-" + std::to_string(getpid()));
        if (std::filesystem::exists(save)) throw std::runtime_error("Resource diagnostic requires a fresh save directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE"}) unsetenv(name);
        app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = label,
            .arguments = {label, "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
            .disc_image = disc,
        };
        auto logger = logging::create_default_logger();
        app::ensure_disc_image_open(configuration, *logger);
        struct Disc { ~Disc() { app::close_disc_image(); } } disc_lifetime;
        struct Probe { Verify& verify; bool exercised = false; } probe{verify};
        const app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t) {
                auto& probe = *static_cast<Probe*>(context);
                const auto* controller = system.mSceneController;
                if (probe.exercised || !controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                    controller->getCurrentSceneForExecute() != controller->mScene ||
                    !dynamic_cast<GameScene*>(controller->mScene)) return;
                const aurora::allocation::HostAllocationScope host;
                probe.verify();
                probe.exercised = true;
            },
        };
        if (app::run_original_game(configuration, *logger, observer) != 0 || !probe.exercised)
            throw std::runtime_error("Original process did not complete the resource assertions");
        std::fprintf(stderr, "PASS %s: original process resources and normal teardown\n", label);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL %s: %s\n", label, error.what());
        return 1;
    }
#endif
}
}
