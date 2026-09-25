#pragma once

#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/FunctionAsyncExecutor.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include <aurora/allocation.hpp>
#include <aurora/guest_thread.hpp>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <exception>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <spawn.h>
#include <string_view>
#include <system_error>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <crt_externs.h>
#else
extern char** environ;
#endif

namespace smgpc::test {
// Uncached original manager loads originate on a loading worker and dispatch
// construction back to the actual main thread. Call from a process observer
// on main; this helper preserves that original queue/heap selection protocol.
template<class Verify> void on_resource_worker(Verify&& verify) {
    auto* executor = MR::getGameSystemObjHolder()->mFunctionAsyncExecutor;
    if (!executor || executor->mMainThreadExec->mThread != OSGetCurrentThread())
        throw std::logic_error("The actual main SDK thread must service original resource construction");
    struct Work {
        Verify& verify;
        OSThread* main;
        std::exception_ptr error;
        void execute() {
            const aurora::allocation::HostAllocationScope host;
            try {
                if (OSGetCurrentThread() == main)
                    throw std::logic_error("Uncached original resource queries must run on the actual worker");
                verify();
            } catch (...) {
                error = std::current_exception();
            }
        }
    } work{verify, OSGetCurrentThread(), {}};
    constexpr auto name = "OriginalStageResourceProcessFixture::resourceWorker";
    MR::startFunctionAsyncExecute(MR::Functor(&work, &Work::execute), 14, name);
    while (!MR::isEndFunctionAsyncExecute(name)) {
        // The original uncached manager path queues construction to main
        // and waits from its loading worker. Preserve both actual contexts.
        executor->update();
        const aurora::os::GuestThreadWaitScope wait;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    MR::waitForEndFunctionAsyncExecute(name);
    if (work.error) std::rethrow_exception(work.error);
}


// MEM1's OS allocator reservation lasts for the host process. Each complete
// original-game generation therefore starts in a fresh executable process.
// The child returns only after Verify performs its post-teardown assertions.
template<class Verify>
int run_stage_resource_generations(int argc, char** argv, const char* label,
                                   unsigned generations, Verify verify) {
    constexpr std::string_view child_argument = "--smgpc-resource-generation";
    try {
        if (argc == 3 && argv[1] == child_argument) {
            unsigned generation = 0;
            const std::string_view value(argv[2]);
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), generation);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || generation >= generations)
                throw std::invalid_argument("Invalid original resource generation");
            return verify(generation);
        }
        if (argc != 1 || generations == 0)
            throw std::invalid_argument("Unexpected original resource fixture arguments");
        for (unsigned generation = 0; generation < generations; ++generation) {
            auto number = std::to_string(generation);
            std::array<char*, 4> arguments{
                argv[0], const_cast<char*>(child_argument.data()), number.data(), nullptr};
            pid_t child = 0;
#if defined(__APPLE__)
            auto** environment = *_NSGetEnviron();
#else
            auto** environment = environ;
#endif
            const int error = posix_spawnp(&child, argv[0], nullptr, nullptr, arguments.data(), environment);
            if (error != 0) throw std::system_error(error, std::generic_category(), "posix_spawnp resource generation");
            int status = 0;
            pid_t waited;
            do {
                waited = waitpid(child, &status, 0);
            } while (waited < 0 && errno == EINTR);
            if (waited < 0) throw std::system_error(errno, std::generic_category(), "waitpid resource generation");
            if (!WIFEXITED(status)) {
                std::fprintf(stderr, "FAIL %s generation %u: child terminated by signal %d\n",
                             label, generation, WIFSIGNALED(status) ? WTERMSIG(status) : 0);
                return 1;
            }
            if (WEXITSTATUS(status) != 0) {
                std::fprintf(stderr, "FAIL %s generation %u: child exited %d\n",
                             label, generation, WEXITSTATUS(status));
                return WEXITSTATUS(status);
            }
        }
        std::fprintf(stderr, "PASS %s: %u independent original process generations\n", label, generations);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL %s generation runner: %s\n", label, error.what());
        return 1;
    }
}

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
