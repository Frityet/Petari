#include "assets/MenuAssetPipeline.hpp"
#include "core/Logger.hpp"
#include "menu/MenuDirector.hpp"
#include "platform/WiiStubs.hpp"
#include "render/BgfxRenderer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

namespace pcport {
namespace {

struct CliConfig {
    std::filesystem::path gameRoot;
    std::string version = "RMGK01";
    std::string language = "KrKorean";
    bool headless = false;
    std::uint32_t autoAdvanceMs = 9000;
    std::optional<std::filesystem::path> dumpFrameDir;
    std::optional<std::filesystem::path> debugViewDir;
    std::uint32_t debugViewEvery = 1;
    std::uint32_t debugViewMaxFrames = 300;
    bool debugOverlay = false;
};

void PrintUsage() {
    std::cerr << "Usage: pc-port [--game-root <path>] [--version <id>] [--language <name>] [--headless] "
              << "[--auto-advance-ms <ms>] [--dump-frame-dir <path>] "
              << "[--debug-view-dir <path>] [--debug-view-every <n>] [--debug-view-max-frames <n>] [--debug-overlay]\n";
}

std::filesystem::path FindRepoRoot() {
    std::filesystem::path cursor = std::filesystem::current_path();

    for (int i = 0; i < 8; ++i) {
        if (std::filesystem::exists(cursor / "pc-port" / "tools" / "prepare_menu_assets.py") &&
            std::filesystem::exists(cursor / "build" / "tools" / "dtk")) {
            return cursor;
        }

        if (cursor.has_parent_path()) {
            cursor = cursor.parent_path();
        } else {
            break;
        }
    }

    throw std::runtime_error("Failed to infer repository root from current working directory");
}

CliConfig ParseArgs(int argc, char** argv) {
    CliConfig config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        auto requireValue = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("Missing value for ") + flag);
            }
            ++i;
            return argv[i];
        };

        if (arg == "--game-root") {
            config.gameRoot = requireValue("--game-root");
        } else if (arg == "--version") {
            config.version = requireValue("--version");
        } else if (arg == "--language") {
            config.language = requireValue("--language");
        } else if (arg == "--headless") {
            config.headless = true;
        } else if (arg == "--auto-advance-ms") {
            const std::string value = requireValue("--auto-advance-ms");
            const unsigned long parsed = std::stoul(value);
            if (parsed > 600000UL) {
                throw std::runtime_error("--auto-advance-ms is too large");
            }
            config.autoAdvanceMs = static_cast<std::uint32_t>(parsed);
        } else if (arg == "--dump-frame-dir") {
            config.dumpFrameDir = requireValue("--dump-frame-dir");
        } else if (arg == "--debug-view-dir") {
            config.debugViewDir = requireValue("--debug-view-dir");
        } else if (arg == "--debug-view-every") {
            const std::string value = requireValue("--debug-view-every");
            const unsigned long parsed = std::stoul(value);
            if (parsed == 0UL || parsed > 60000UL) {
                throw std::runtime_error("--debug-view-every must be in range [1, 60000]");
            }
            config.debugViewEvery = static_cast<std::uint32_t>(parsed);
        } else if (arg == "--debug-view-max-frames") {
            const std::string value = requireValue("--debug-view-max-frames");
            const unsigned long parsed = std::stoul(value);
            if (parsed == 0UL || parsed > 200000UL) {
                throw std::runtime_error("--debug-view-max-frames must be in range [1, 200000]");
            }
            config.debugViewMaxFrames = static_cast<std::uint32_t>(parsed);
        } else if (arg == "--debug-overlay") {
            config.debugOverlay = true;
        } else if (arg == "--help" || arg == "-h") {
            PrintUsage();
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    if (config.gameRoot.empty()) {
        config.gameRoot = FindRepoRoot();
    }

    return config;
}

void MaybeDumpFrame(BgfxRenderer& renderer, const std::optional<std::filesystem::path>& dumpDir, const char* label, bool* alreadyDumpedFlag) {
    if (!dumpDir.has_value() || *alreadyDumpedFlag) {
        return;
    }

    std::filesystem::create_directories(*dumpDir);

    const bool pngOk = renderer.SaveAllLayersPng(*dumpDir, label);
    const bool ppmOk = renderer.SaveAllLayersPpm(*dumpDir, label);
    const bool wroteAny = pngOk || ppmOk;

    if (!wroteAny) {
        throw std::runtime_error("Failed to write any frame dump for label " + std::string(label));
    }

    if (!pngOk) {
        Log(LogLevel::Warn, LogCategory::App, "PNG layer capture incomplete for " + std::string(label));
    }
    if (!ppmOk) {
        Log(LogLevel::Warn, LogCategory::App, "PPM layer capture incomplete for " + std::string(label));
    }

    Log(LogLevel::Info, LogCategory::App, "Frame dump attempt complete for " + std::string(label));
    *alreadyDumpedFlag = true;
}

class DebugFrameRecorder final {
public:
    explicit DebugFrameRecorder(const CliConfig& cli)
        : mDir(cli.debugViewDir), mEvery(cli.debugViewEvery), mMaxFrames(cli.debugViewMaxFrames), mCaptureEnabled(cli.debugViewDir.has_value()) {
        if (!mCaptureEnabled) {
            return;
        }

        std::filesystem::create_directories(*mDir);
        const std::filesystem::path manifestPath = *mDir / "frames.csv";
        mManifest.open(manifestPath);
        if (!mManifest) {
            throw std::runtime_error("Failed to open debug manifest for writing: " + manifestPath.string());
        }
        mManifest << "frame_index,state,presented_hash,layer_count,png_prefix,ppm_prefix,panes_file\n";

        Log(LogLevel::Info, LogCategory::App,
            "Debug frame recorder enabled. dir=" + mDir->string() + " every=" + std::to_string(mEvery) +
                " max=" + std::to_string(mMaxFrames));
    }

    void CaptureIfNeeded(BgfxRenderer& renderer, const MenuDirector& director, std::uint64_t frameIndex) {
        if (!mCaptureEnabled || mCaptured >= mMaxFrames || (frameIndex % mEvery) != 0U) {
            return;
        }

        const std::string stateName = ToString(director.GetState());
        const std::string frameTag = "frame_" + std::to_string(frameIndex) + "_" + stateName;
        const std::filesystem::path panesPath = *mDir / (frameTag + "_panes.csv");

        const bool pngOk = renderer.SaveAllLayersPng(*mDir, frameTag);
        if (!pngOk) {
            throw std::runtime_error("Debug recorder failed to capture PNG layers for frame tag: " + frameTag);
        }

        const bool ppmOk = renderer.SaveAllLayersPpm(*mDir, frameTag);
        if (!ppmOk && !mWarnedPpmUnavailable) {
            Log(LogLevel::Warn, LogCategory::App,
                "PPM layer capture incomplete on this backend; continuing with PNG + pane metadata");
            mWarnedPpmUnavailable = true;
        }

        const bool panesOk = renderer.SaveDebugPaneList(panesPath);
        if (!panesOk) {
            throw std::runtime_error("Debug recorder failed to write pane metadata: " + panesPath.string());
        }

        const std::uint64_t hash = renderer.ComputeHash(CaptureSurface::Presented);
        mManifest << frameIndex << ',' << stateName << ',' << hash << ',' << renderer.GetCapturedLayerCount() << ',' << frameTag << ','
                  << (ppmOk ? frameTag : "") << ',' << panesPath.filename().string() << '\n';
        ++mCaptured;
    }

private:
    std::optional<std::filesystem::path> mDir;
    std::uint32_t mEvery = 1;
    std::uint32_t mMaxFrames = 1;
    bool mCaptureEnabled = false;
    bool mWarnedPpmUnavailable = false;
    std::uint32_t mCaptured = 0;
    std::ofstream mManifest;
};

int RunHeadless(const CliConfig& cli, MenuDirector& director, BgfxRenderer& renderer) {
    constexpr float kFrameMs = 1000.0F / 60.0F;
    constexpr int kMaxFrames = 12000;

    renderer.SetDebugOverlayEnabled(cli.debugOverlay);
    DebugFrameRecorder debugRecorder(cli);

    bool dumpedTitleAppear = false;
    bool dumpedTitleLoop = false;
    bool dumpedTitleDecide = false;

    int titleDoneFrames = 0;

    for (int frame = 0; frame < kMaxFrames; ++frame) {
        director.SetButtonState(false, false);
        director.Update(kFrameMs);

        renderer.BeginFrame();
        renderer.Clear(0U, 0U, 0U, 255U);
        director.Render(renderer);
        renderer.EndFrame();
        debugRecorder.CaptureIfNeeded(renderer, director, static_cast<std::uint64_t>(frame));

        switch (director.GetState()) {
        case MenuState::TitleAppear:
            MaybeDumpFrame(renderer, cli.dumpFrameDir, "TitleAppear", &dumpedTitleAppear);
            break;
        case MenuState::TitleLoop:
            MaybeDumpFrame(renderer, cli.dumpFrameDir, "TitleLoop", &dumpedTitleLoop);
            break;
        case MenuState::TitleDecide:
            MaybeDumpFrame(renderer, cli.dumpFrameDir, "TitleDecide", &dumpedTitleDecide);
            break;
        case MenuState::Boot:
        case MenuState::TitleDone:
            break;
        }

        if (director.GetState() == MenuState::TitleDone) {
            ++titleDoneFrames;
            if (titleDoneFrames > 120) {
                Log(LogLevel::Info, LogCategory::App,
                    "Headless complete. Final frame hash=" + std::to_string(renderer.ComputeHash(CaptureSurface::Presented)) +
                        " state=TitleDone");
                return 0;
            }
        }
    }

    Log(LogLevel::Error, LogCategory::App, "Headless run did not reach stable TitleDone within max frame budget");
    return 1;
}

int RunWindowed(const CliConfig& cli, MenuDirector& director) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("SMG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1216, 912,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        const std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow failed: " + error);
    }

    bool dumpedTitleAppear = false;
    bool dumpedTitleLoop = false;
    bool dumpedTitleDecide = false;

    int result = 0;

    try {
        BgfxRenderer renderer(window, 608, 456);
        renderer.SetDebugOverlayEnabled(cli.debugOverlay);
        DebugFrameRecorder debugRecorder(cli);

        bool running = true;
        bool buttonA = false;
        bool buttonB = false;
        std::uint64_t lastTicks = SDL_GetTicks();
        std::uint64_t frameIndex = 0U;

        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else if (event.key.keysym.sym == SDLK_a) {
                        buttonA = true;
                    } else if (event.key.keysym.sym == SDLK_b) {
                        buttonB = true;
                    } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        director.RequestAdvance();
                    }
                } else if (event.type == SDL_KEYUP) {
                    if (event.key.keysym.sym == SDLK_a) {
                        buttonA = false;
                    } else if (event.key.keysym.sym == SDLK_b) {
                        buttonB = false;
                    }
                }
            }

            const std::uint64_t nowTicks = SDL_GetTicks();
            const std::uint64_t deltaTicks = (nowTicks >= lastTicks) ? (nowTicks - lastTicks) : 0U;
            lastTicks = nowTicks;

            const float deltaMs = std::clamp(static_cast<float>(deltaTicks), 1.0F, 50.0F);

            director.SetButtonState(buttonA, buttonB);
            director.Update(deltaMs);

            renderer.BeginFrame();
            renderer.Clear(0U, 0U, 0U, 255U);
            director.Render(renderer);
            renderer.EndFrame();
            debugRecorder.CaptureIfNeeded(renderer, director, frameIndex);
            ++frameIndex;

            switch (director.GetState()) {
            case MenuState::TitleAppear:
                MaybeDumpFrame(renderer, cli.dumpFrameDir, "TitleAppear", &dumpedTitleAppear);
                break;
            case MenuState::TitleLoop:
                MaybeDumpFrame(renderer, cli.dumpFrameDir, "TitleLoop", &dumpedTitleLoop);
                break;
            case MenuState::TitleDecide:
                MaybeDumpFrame(renderer, cli.dumpFrameDir, "TitleDecide", &dumpedTitleDecide);
                break;
            case MenuState::Boot:
            case MenuState::TitleDone:
                break;
            }
        }
    } catch (...) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}

}  // namespace
}  // namespace pcport

int main(int argc, char** argv) {
    using namespace pcport;

    SetLogLevel(LogLevel::Info);

    try {
        const CliConfig cli = ParseArgs(argc, argv);

        if (!std::filesystem::exists(cli.gameRoot)) {
            throw std::runtime_error("--game-root does not exist: " + cli.gameRoot.string());
        }

        const std::filesystem::path repoRoot = FindRepoRoot();
        const std::filesystem::path cacheRoot = repoRoot / "pc-port" / ".cache" / "assets";

        MenuAssetPipelineConfig pipelineConfig;
        pipelineConfig.locator.gameRoot = cli.gameRoot;
        pipelineConfig.locator.version = cli.version;
        pipelineConfig.locator.language = cli.language;
        pipelineConfig.repoRoot = repoRoot;
        pipelineConfig.cacheRoot = cacheRoot;
        pipelineConfig.forceRebuild = false;

        const PreparedMenuAssets assets = PrepareMenuAssets(pipelineConfig);

        (void)WiiStubs::GetMiiCount();
        (void)WiiStubs::IsNwc24Available();
        (void)WiiStubs::IsHomeButtonEnabled();

        MenuDirectorConfig menuConfig;
        menuConfig.autoAdvanceMs = cli.autoAdvanceMs;

        MenuDirector director = MenuDirector::Create(assets, menuConfig);

        if (cli.headless) {
            BgfxRenderer renderer(608, 456);
            return RunHeadless(cli, director, renderer);
        }

        return RunWindowed(cli, director);
    } catch (const std::exception& ex) {
        Log(LogLevel::Error, LogCategory::App, ex.what());
        PrintUsage();
        return 2;
    }
}
